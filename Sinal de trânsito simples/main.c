#include <stdio.h>  // Biblioteca para funções de entrada e saída (como printf)
#include "pico/stdlib.h"  // Biblioteca padrão para o Raspberry Pi Pico, para manipulação dos pinos GPIO e delay
#include "pico/cyw43_arch.h"  // Biblioteca para o gerenciamento da rede sem fio (Wi-Fi, Bluetooth)
#include "hardware/pwm.h"  // Biblioteca para controle de PWM (modulação por largura de pulso)
#include "hardware/clocks.h"  // Biblioteca para controle de clocks do sistema, útil para ajustar frequências

// Definição dos pinos GPIO para LEDs e buzzer
#define LED_VERMELHO 0  // LED vermelho no pino GPIO 0
#define LED_VERDE 3  // LED verde no pino GPIO 3
#define LED_AMARELO 4  // LED amarelo no pino GPIO 4
#define LED_VERDE_PEDESTRE 7  // LED verde para pedestre no pino GPIO 7
#define BUZZER 15  // Buzzer no pino GPIO 15
#define BOTAO 10 // Botão no pino GPIO 10

// Definições relacionadas ao buzzer (PWM)
#define BUZZER_FREQ_HZ 3200  // Frequência do buzzer em Hz (quanto maior, mais agudo)
#define HIGH 4096  // Valor máximo para o PWM (nível alto)
#define LOW 0  // Valor mínimo para o PWM (nível baixo)

// Função para inicializar o buzzer
void inicializarBuzzer() {
    // Configura o pino do BUZZER para funcionar como PWM (modulação por largura de pulso)
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);

    // Obtém o número do slice de PWM correspondente ao pino do buzzer
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);

    // Obtém a configuração padrão do PWM
    pwm_config cfg = pwm_get_default_config();

    // Define o divisor do clock para ajustar a frequência do buzzer
    pwm_config_set_clkdiv(&cfg, clock_get_hz(clk_sys) / (BUZZER_FREQ_HZ * HIGH));

    // Inicializa o PWM no slice correspondente com a configuração definida
    pwm_init(slice_num, &cfg, true);

    // Inicializa o buzzer com o nível baixo (sem som)
    pwm_set_gpio_level(BUZZER, LOW);
}

// Função de inicialização dos pinos GPIO para LEDs e buzzer
void inicializar() {
    // Inicializa os pinos de LEDs e buzzer como saída, e configura o pino do botão como entrada com pull-up
    stdio_init_all();  // Inicializa as funções de entrada e saída para permitir uso do printf e outras funções de I/O

    gpio_init(LED_VERMELHO);  // Inicializa o pino do LED vermelho
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);  // Configura o pino do LED vermelho como saída

    gpio_init(LED_VERDE);  // Inicializa o pino do LED verde
    gpio_set_dir(LED_VERDE, GPIO_OUT);  // Configura o pino do LED verde como saída

    gpio_init(LED_AMARELO);  // Inicializa o pino do LED amarelo
    gpio_set_dir(LED_AMARELO, GPIO_OUT);  // Configura o pino do LED amarelo como saída

    gpio_init(LED_VERDE_PEDESTRE);  // Inicializa o pino do LED verde para pedestre
    gpio_set_dir(LED_VERDE_PEDESTRE, GPIO_OUT);  // Configura o pino do LED verde para pedestre como saída

    gpio_init(BUZZER);  // Inicializa o pino do buzzer
    gpio_set_dir(BUZZER, GPIO_OUT);  // Configura o pino do buzzer como saída

    inicializarBuzzer();  // Chama a função para configurar o buzzer com PWM

    gpio_init(BOTAO);  // Inicializa o pino do botão
    gpio_set_dir(BOTAO, GPIO_IN);  // Configura o pino do botão como entrada
    gpio_pull_up(BOTAO);  // Ativa o resistor pull-up no pino do botão, garantindo um valor lógico "alto" quando o botão não é pressionado
}

// Função que verifica se o botão foi pressionado
bool pressionarBotao() {
    // Verifica se o botão foi pressionado (valor lógico baixo)
    if (gpio_get(BOTAO) == 0) {
        // Desliga os LEDs de semáforo e acende o LED amarelo para indicar transição
        gpio_put(LED_VERMELHO, 0);
        gpio_put(LED_VERDE, 0);
        gpio_put(LED_VERDE_PEDESTRE, 0);
        gpio_put(LED_AMARELO, 1);
        
        // Aguarda 5 segundos com o LED amarelo aceso
        sleep_ms(5000);

        // Desliga o LED amarelo e acende o LED vermelho e o verde para pedestre
        gpio_put(LED_AMARELO, 0);
        gpio_put(LED_VERMELHO, 1);
        gpio_put(LED_VERDE_PEDESTRE, 1);

        // Aguarda 15 segundos com o buzzer ativo(tempo para o pedestre atravessar)
        pwm_set_gpio_level(BUZZER, HIGH);
        sleep_ms(15000);

        // Desliga o LED verde para pedestre e o LED vermelho, desliga o buzzer
        gpio_put(LED_VERDE_PEDESTRE, 0);
        gpio_put(LED_VERMELHO, 0);
        pwm_set_gpio_level(BUZZER, LOW);

        // Retorna ao controle normal do fluxo do semáforo
        fluxo();
        return true;
    }
    return false;  // Retorna falso caso o botão não tenha sido pressionado
}

// Função para controlar o fluxo dos semáforos
void fluxo() {
    // Acende o LED verde para veículos e apaga o LED vermelho
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 0);

    // Aguarda 8 segundos para o fluxo de veículos, enquanto verifica se o botão foi pressionado
    for (int i = 0; i < 8000; i++) {
        sleep_ms(1);
        if (pressionarBotao() == true) {
            break;  // Se o botão for pressionado, interrompe o ciclo
        }
    }

    // Acende o LED amarelo, indicando que a troca de sinal está prestes a acontecer
    gpio_put(LED_AMARELO, 1);
    gpio_put(LED_VERDE, 0);

    // Aguarda 2 segundos (tempo de transição entre os semáforos)
    for (int i = 0; i < 2000; i++) {
        sleep_ms(1);
        if (pressionarBotao() == true) {
            break;  // Se o botão for pressionado, interrompe o ciclo
        }
    }

    // Acende o LED vermelho e apaga o LED amarelo
    gpio_put(LED_VERMELHO, 1);
    gpio_put(LED_AMARELO, 0);

    // Aguarda 10 segundos (tempo de semáforo vermelho)
    for (int i = 0; i < 10000; i++) {
        sleep_ms(1);
        if (pressionarBotao() == true) {
            break;  // Se o botão for pressionado, interrompe o ciclo
        }
    }
}

// Função principal
int main() {

    inicializar();  // Chama a função de inicialização dos pinos GPIO

    // Loop principal, onde o fluxo do semáforo é constantemente verificado e atualizado
    while (true) {
        fluxo();  // Controla o fluxo dos semáforos
    }
}
