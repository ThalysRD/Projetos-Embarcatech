#include <stdio.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

ssd1306_t display;

#define BTN_A_PIN 5

char *msg1 = "";
char *msg2 = "";
char *msg3 = "";

int A_state = 0;

#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7

struct pixel_t {
  uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

void inicializar(){
  stdio_init_all();
  gpio_init(LED_R_PIN);
  gpio_set_dir(LED_R_PIN, GPIO_OUT);
  gpio_init(LED_G_PIN);
  gpio_set_dir(LED_G_PIN, GPIO_OUT);
  gpio_init(LED_B_PIN);
  gpio_set_dir(LED_B_PIN, GPIO_OUT);

  gpio_init(BTN_A_PIN);
  gpio_set_dir(BTN_A_PIN, GPIO_IN);
  gpio_pull_up(BTN_A_PIN);

  i2c_init(I2C_PORT, 400*1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  
  ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT );

  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }

  ws2818b_program_init(np_pio, sm, offset, LED_PIN, 800000.f);

  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

void npWrite() {
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100);
}

int WaitWithRead(int timeMS){
    for(int i = 0; i < timeMS; i = i+100){
        A_state = !gpio_get(BTN_A_PIN);
        if(A_state == 1){
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
}

void SinalAberto(){
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);   
    msg1 = "SINAL ABERTO";
    msg2 = "ATRAVESSAR";
    msg3 = "COM CUIDADO";
    
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, msg1);
    ssd1306_draw_string(&display, 0, 10, 1, msg2);
    ssd1306_draw_string(&display, 0, 20, 1, msg3);
    ssd1306_show(&display);

    npClear();
    for(int i = 0; i < 25; i++){
        npSetLED(i, 255, 0, 0);
    }
    npWrite();
}

void SinalAtencao(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);
    msg1 = "SINAL DE ATENCAO";
    msg2 = "PREPARE-SE";
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, msg1);
    ssd1306_draw_string(&display, 0, 10, 1, msg2);
    ssd1306_show(&display);
    npClear();
    for(int i = 0; i < 25; i++){
        npSetLED(i, 255, 255, 0);
    }
    npWrite();
}

void SinalFechado(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
    msg1 = "SINAL FECHADO";
    msg2 = "AGUARDE ";
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, msg1);
    ssd1306_draw_string(&display, 0, 10, 1, msg2);
    ssd1306_show(&display);
    npClear();
    for(int i = 0; i < 25; i++){
        npSetLED(i, 0, 255, 0);
    }
    npWrite();
}

int main(){
    inicializar();

    while(true){
        SinalFechado();
        A_state = WaitWithRead(8000);   
        SinalAtencao();

        if(A_state){               
            sleep_ms(5000);
            SinalAberto();
            sleep_ms(10000);
        }else{                          
            SinalAtencao();
            sleep_ms(2000);
            SinalAberto();
            sleep_ms(8000);
        }
    }

    return 0;
}
