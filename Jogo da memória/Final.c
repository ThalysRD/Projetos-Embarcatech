#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "ws2818b.pio.h"
#include "hardware/adc.h"
#include "pico/rand.h"
#include "pico/time.h"

// Definições de I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
ssd1306_t display;

// Configurações de Wi-Fi
#define REDE "brisa-2238553"
#define SENHA "j9v745gq"

//Configurações de ThingSpeak
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "3FPDUB3BWLYSB0DL"
struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

char http_response[1024];

// Definições de botões
#define BUTTON_A 5
#define BUTTON_B 6

//Definições para a matriz de LEDs RGB
#define LED_COUNT 25
#define LED_PIN 7
typedef struct {
    uint8_t G, R, B;
} npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Definições de joystick
const int VRY = 26; // Eixo Y do joystick
const int VRX = 27; // Eixo X do joystick
const int ADC_CHANNEL_0 = 0; // Canal ADC para VRY
const int ADC_CHANNEL_1 = 1; // Canal ADC para VRX
const int SW = 22; // Botão do joystick

int numeros[2]; // Número dos LEDs que acenderam
int numeros_selecionados[2] = { -1, -1 }; // Números selecionados pelo usuário
int indice_selecionado = 0; // Índice do número selecionado
bool start_game = false; // Indica se o jogo deve começar

int tentativas = 0; // Contador de tentativas
int acertos = 0; // Contador de acertos
int speed = 3000; // Tempo que os leds ficam na tela para o usuário memorizar
bool jogo_ativo = false; // Indica se o jogo está ativo

// Limiares para leitura do joystick
#define LIMIAR_SUP 3000
#define LIMIAR_INF 1000
int indice_led_verde = 12; // Índice do LED verde

// Função para inicializar o joystick
void inicializar_joystick() {
    adc_init();
    adc_gpio_init(VRY);
    adc_gpio_init(VRX);
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
}

// Função para ler os eixos do joystick
void joystick_read_axis(uint16_t *vry_value, uint16_t *vrx_value) {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vry_value = adc_read();

    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vrx_value = adc_read();
}

// Função para inicializar a matriz de LEDs
void inicializar_matriz() {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, sm, offset, LED_PIN, 800000.f);

    // Inicializa todos os LEDs como apagados
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Função para limpar todos os LEDs
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

// Função para escrever as cores dos LEDs na matriz
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

// Função para imprimir mensagens no display
void print_msg(int x, int y, int tam, const char *msg, bool clear) {
    if (clear) {
        ssd1306_clear(&display);
    }
    ssd1306_draw_string(&display, x, y, tam, msg);
    ssd1306_show(&display);
}

// Função para inicializar o display OLED
void inicializar_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT);
}

// Função para inicializar os botões
void inicializar_botao() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

// Callback para receber dados da conexão HTTP
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

// Callback para conexão HTTP
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return err;
    }

    printf("Conectado ao ThingSpeak!\n");

    // Monta a requisição HTTP
    char request[256];
    snprintf(request, sizeof(request),
        "GET /update?api_key=%s&field1=%d&field2=%d HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        API_KEY, acertos, tentativas, THINGSPEAK_HOST);

    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_recv_callback);

    return ERR_OK;
}

// Callback para resolução de DNS
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        printf("Endereço IP do ThingSpeak: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    } else {
        printf("Falha na resolução de DNS\n");
    }
}

// Função para inicializar a conexão Wi-Fi
void inicializar_conexao() {
    if (cyw43_arch_init()) {
        print_msg(1, 50, 1, "Falha ao iniciar Wi-Fi", true);
        return;
    }

    cyw43_arch_enable_sta_mode();
    print_msg(1, 50, 1, "Conectando ao Wi-Fi...", true);

    if (cyw43_arch_wifi_connect_blocking(REDE, SENHA, CYW43_AUTH_WPA2_MIXED_PSK)) {
        print_msg(1, 50, 1, "Falha ao conectar ao Wi-Fi", true);
        return;
    }

    print_msg(1, 50, 1, "Wi-Fi conectado!", true);
}

// Função para inicializar todos os componentes
void inicializar() {
    stdio_init_all();
    inicializar_display();
    inicializar_matriz();
    inicializar_joystick();
    inicializar_botao();
    inicializar_conexao();
}

// Função para mover o LED verde
void mover_led_verde(int deslocamento) {
    int novo_indice = indice_led_verde + deslocamento;

    if (novo_indice >= 0 && novo_indice < LED_COUNT) {
        indice_led_verde = novo_indice;
        npClear();
        npSetLED(indice_led_verde, 0, 255, 0);
        npWrite();
    }
}

// Função chamada em caso de acerto
void acerto() {
    acertos += 1; 
    if (speed > 500) {
        speed -= 250; // Aumenta a dificuldade reduzindo a tempo que os LEDs ficam na tela
    }

    // Pisca os LEDs verdes 10 vezes
    for (int i = 0; i < 10; i++) {
        npClear(); // Limpa os LEDs
        for (int index = 0; index < LED_COUNT; index++) {
            npSetLED(index, 0, 255, 0); // Define todos os LEDs como verde
        }
        npWrite(); // Atualiza a matriz de LEDs
        absolute_time_t next_wake_time = delayed_by_ms(get_absolute_time(), 200);
        while (true) {
            if (time_reached(next_wake_time)) {
                break;
            }
        }

        npClear(); // Limpa os LEDs
        npWrite(); // Atualiza a matriz de LEDs
        next_wake_time = delayed_by_ms(get_absolute_time(), 200);
        while (true) {
            if (time_reached(next_wake_time)) {
                break;
            }
        }
    }
}

// Função chamada em caso de erro
void erro() {
    // Pisca os LEDs vermelhos 10 vezes
    for (int i = 0; i < 10; i++) {
        npClear(); // Limpa os LEDs
        for (int index = 0; index < LED_COUNT; index++) {
            npSetLED(index, 255, 0, 0); // Define todos os LEDs como vermelho
        }
        npWrite(); // Atualiza a matriz de LEDs
        absolute_time_t next_wake_time = delayed_by_ms(get_absolute_time(), 200);
        while (true) {
            if (time_reached(next_wake_time)) {
                break;
            }
        }

        npClear(); // Limpa os LEDs
        npWrite(); // Atualiza a matriz de LEDs
        next_wake_time = delayed_by_ms(get_absolute_time(), 200);
        while (true) {
            if (time_reached(next_wake_time)) {
                break;
            }
        }
    }
}

// Função para o jogo da memória
void jogo_da_memoria() {
    npClear();
    npWrite();
    
    // Gera dois índices aleatórios diferentes
    uint32_t indice1 = get_rand_32() % LED_COUNT;
    uint32_t indice2 = get_rand_32() % LED_COUNT;
    
    while (indice1 == indice2) {
        indice2 = get_rand_32() % LED_COUNT;
    }

    numeros[0] = indice1;
    numeros[1] = indice2;
    
    // Acende os LEDs que devem ser memorizados
    npSetLED(indice1, 123, 104, 238);
    npSetLED(indice2, 123, 104, 238);
    
    npWrite(); // Atualiza a matriz de LEDs para mostrar os LEDs que devem ser memorizados

    // Aguarda o tempo de memorização
    absolute_time_t next_wake_time = delayed_by_ms(get_absolute_time(), speed);
    while (true) {
        if (time_reached(next_wake_time)) {
            break;
        }
    }
    
    npClear(); // Limpa os LEDs após o tempo de memorização
    npWrite(); // Atualiza a matriz de LEDs
}

// Função para reiniciar o jogo
void reiniciar_jogo() {
    jogo_da_memoria(); // Inicia um novo jogo
    indice_selecionado = 0;
    numeros_selecionados[0] = -1;
    numeros_selecionados[1] = -1;
    indice_led_verde = 12; // Reseta o índice do LED verde
}

// Função para verificar se o usuário acertou
void verifica_acerto() {
    if (numeros_selecionados[0] != -1 && numeros_selecionados[1] != -1) {
        bool acertou1 = (numeros_selecionados[0] == numeros[0]) || (numeros_selecionados[0] == numeros[1]);
        bool acertou2 = (numeros_selecionados[1] == numeros[0]) || (numeros_selecionados[1] == numeros[1]);

        if (acertou1 && acertou2) {
            acerto(); // Chama a função de acerto
        } else {
            erro(); // Chama a função de erro
        }

        // Atualiza a pontuação no display
        char score[10];
        snprintf(score, sizeof(score), "%d/%d", acertos, tentativas);
        print_msg(20, 1, 1, "Acertou:", true);
        print_msg(70, 1, 1, score, false);

        // Reseta a seleção
        numeros_selecionados[0] = -1;
        numeros_selecionados[1] = -1;
        indice_selecionado = 0;
    }
    //Inicia novamente o jogo com um tempo reduzido em relação a tentativa anterior
    reiniciar_jogo();
}


// Função para mover o LED com o joystick
void mover_led_com_joystick() {
    uint16_t vry_value, vrx_value;
    joystick_read_axis(&vry_value, &vrx_value);

    // Matriz que representa a posição dos LEDs
    int matriz[5][5] = {
        {24, 23, 22, 21, 20}, 
        {15, 16, 17, 18, 19},
        {14, 13, 12, 11, 10}, 
        { 5,  6,  7,  8,  9}, 
        { 4,  3,  2,  1,  0}
    };

    int linha_atual = -1, coluna_atual = -1;
    // Encontra a posição atual do LED verde na matriz
    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            if (matriz[linha][coluna] == indice_led_verde) {
                linha_atual = linha;
                coluna_atual = coluna;
                break;
            }
        }
    }

    if (linha_atual == -1 || coluna_atual == -1) return; // Se não encontrar, sai da função

    // Move o LED verde com base na leitura do joystick
    if (vry_value > LIMIAR_SUP && linha_atual > 0) {
        linha_atual--;
    }
    
    if (vry_value < LIMIAR_INF && linha_atual < 4) {
        linha_atual++;
    }

    if (vrx_value > LIMIAR_SUP && coluna_atual < 4) {
        coluna_atual++;
    }

    if (vrx_value < LIMIAR_INF && coluna_atual > 0) {
        coluna_atual--;
    }

    indice_led_verde = matriz[linha_atual][coluna_atual]; // Atualiza o índice do LED verde
    if (gpio_get(SW) == 0) { // Se o botão do joystick for pressionado
        absolute_time_t next_wake_time = delayed_by_ms(get_absolute_time(), 200);
        while (true) {
            if (time_reached(next_wake_time)) {
                break;
            }
        }
        if (indice_selecionado < 2) { // Limita a seleção a dois LEDs
            numeros_selecionados[indice_selecionado] = indice_led_verde; // Armazena a seleção
            indice_selecionado += 1; // Avança para o próximo índice
        }
    }

    // Atualiza a mensagem no display
    char indice[10];
    snprintf(indice, sizeof(indice), "(%d, %d)", numeros_selecionados[0], numeros_selecionados[1]);
    
    print_msg(20, 15, 1, "Selecione os", false);
    print_msg(20, 25, 1, "LEDs iguais", false);
    print_msg(20, 45, 1, "Para encerrar", false);
    print_msg(20, 55, 1, "Pressione B", false);
    
    // Verifica acertos após selecionar dois LEDs
    if (indice_selecionado >= 2) {
        tentativas += 1;
        verifica_acerto(); // Verifica acertos
        // Não reinicie o jogo aqui, faça isso na função verifica_acerto
    }
    
    npClear();
    npSetLED(indice_led_verde, 0, 255, 0); // Acende o LED verde
    npWrite();
    
    // Se o botão B for pressionado, envia os dados para o ThingSpeak
    if (gpio_get(BUTTON_B) == 0) {
        npClear();
        npWrite();
        print_msg(20, 30, 1, "Aguarde...", true);
        for (int i = 0; i < 3; i++) {
            dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
            absolute_time_t next_wake_time = delayed_by_ms(get_absolute_time(), 2000);
            while (true ) {
                if (time_reached(next_wake_time)) {
                    break;
                }
            }
        }
        speed = 3000; // Reseta a velocidade do jogo
        ssd1306_clear(&display); // Limpa o display
        tentativas = 0; // Reseta o contador de tentativas
        acertos = 0; // Reseta o contador de acertos
        jogo_ativo = false; // O jogo não está mais ativo
    }
}
// Função principal
int main() {
    inicializar(); // Inicializa todos os componentes
    while (true) {
        // Verifica se o botão A foi pressionado para iniciar o jogo
        if (gpio_get(BUTTON_A) == 0 && !jogo_ativo) {
            start_game = true;
            jogo_ativo = true; // O jogo começa
            ssd1306_clear(&display);
            jogo_da_memoria(); // Inicia o jogo
        }

        // Se o jogo estiver ativo, move o LED com o joystick
        if (jogo_ativo) {
            mover_led_com_joystick(); // Chama a função para mover o LED com o joystick
        } else {
            //Limpa a matriz de leds
            npClear();
            npWrite(); 
            // Mensagens para instruir o usuário a iniciar o jogo
            print_msg(20, 10, 1, "Para iniciar", false);
            print_msg(20, 20, 1, "Pressione A", false);
        }

        sleep_ms(1); // Aguarda um milissegundo antes da próxima iteração
    }

    return 0; // Retorna 0 ao final do programa
}