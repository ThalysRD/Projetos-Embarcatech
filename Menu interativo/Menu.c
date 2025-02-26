#include <stdio.h>
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

const int VRY = 26;
const int ADC_CHANNEL_0 = 0;
const int SW = 22;

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

ssd1306_t display;
int selected_option = 0;

void setup_joystick()
{
    adc_init();
    adc_gpio_init(VRY);
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
}

void setup_display()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT);
}

void display_menu()
{
    ssd1306_clear(&display);
    switch (selected_option)
    {
    case 0:
        ssd1306_clear(&display);
        ssd1306_draw_empty_square(&display, 0, 0, 127, 16);
        ssd1306_draw_string(&display, 2, 4, 1, "1 - Joystick LED");
        ssd1306_draw_string(&display, 0, 24, 1, "2 - Buzzer");
        ssd1306_draw_string(&display, 0, 44, 1, "3 - LED RGB");
        break;
    case 1:
        ssd1306_clear(&display);
        ssd1306_draw_empty_square(&display, 0, 20, 127, 16);
        ssd1306_draw_string(&display, 0, 4, 1, "1 - Joystick LED");
        ssd1306_draw_string(&display, 2, 24, 1, "2 - Buzzer");
        ssd1306_draw_string(&display, 0, 44, 1, "3 - LED RGB");
        break;
    case 2:
        ssd1306_clear(&display);
        ssd1306_draw_empty_square(&display, 0, 40, 127, 16);
        ssd1306_draw_string(&display, 0, 4, 1, "1 - Joystick LED");
        ssd1306_draw_string(&display, 0, 24, 1, "2 - Buzzer");
        ssd1306_draw_string(&display, 2, 44, 1, "3 - LED RGB");
        break;
    }

    ssd1306_show(&display);
}

void joystick_read_axis(uint16_t *vry_value)
{
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vry_value = adc_read();
}

void setup()
{
    stdio_init_all();
    setup_joystick();
    setup_display();
}

void programa1()
{
    sleep_ms(200);
    const int VRX = 27;
    const int ADC_CHANNEL_1 = 1;
    const int LED_B = 13;
    const int LED_R = 11;
    const float DIVIDER_PWM = 16.0;
    const uint16_t PERIOD = 4096;
    uint16_t led_b_level, led_r_level = 100;
    uint slice_led_b, slice_led_r;

    void setup_pwm_led(uint led, uint *slice, uint16_t level)
    {
        gpio_set_function(led, GPIO_FUNC_PWM);
        *slice = pwm_gpio_to_slice_num(led);
        pwm_set_clkdiv(*slice, DIVIDER_PWM);
        pwm_set_wrap(*slice, PERIOD);
        pwm_set_gpio_level(led, level);
        pwm_set_enabled(*slice, true);
    }

    void setup_programa1()
    {
        adc_gpio_init(VRX);
        setup_pwm_led(LED_B, &slice_led_b, led_b_level);
        setup_pwm_led(LED_R, &slice_led_r, led_r_level);
    }

    void joystick_read_axis_programa1(uint16_t *vrx_value, uint16_t *vry_value)
    {
        adc_select_input(ADC_CHANNEL_0);
        sleep_us(2);
        *vrx_value = adc_read();

        adc_select_input(ADC_CHANNEL_1);
        sleep_us(2);
        *vry_value = adc_read();
    }

    setup_programa1();
    uint16_t vrx_value, vry_value, sw_value;
    while (1)
    {
        joystick_read_axis_programa1(&vrx_value, &vry_value);
        pwm_set_gpio_level(LED_B, vrx_value);
        pwm_set_gpio_level(LED_R, vry_value);

        sleep_ms(100);
        if (gpio_get(SW) == 0)
        {
            setup_pwm_led(LED_B, &slice_led_b, 0);
            setup_pwm_led(LED_R, &slice_led_r, 0);
            return;
        }
    }
}

void programa2()
{
    sleep_ms(200);
    #define BUZZER_PIN 21

    const uint star_wars_notes[] = {
        330, 330, 330, 262, 392, 523, 330, 262,
        392, 523, 330, 659, 659, 659, 698, 523,
        415, 349, 330, 262, 392, 523, 330, 262,
        392, 523, 330, 659, 659, 659, 698, 523,
        415, 349, 330, 523, 494, 440, 392, 330,
        659, 784, 659, 523, 494, 440, 392, 330,
        659, 659, 330, 784, 880, 698, 784, 659,
        523, 494, 440, 392, 659, 784, 659, 523,
        494, 440, 392, 330, 659, 523, 659, 262,
        330, 294, 247, 262, 220, 262, 330, 262,
        330, 294, 247, 262, 330, 392, 523, 440,
        349, 330, 659, 784, 659, 523, 494, 440,
        392, 659, 784, 659, 523, 494, 440, 392
    };

    const uint note_duration[] = {
        500, 500, 500, 350, 150, 300, 500, 350,
        150, 300, 500, 500, 500, 500, 350, 150,
        300, 500, 500, 350, 150, 300, 500, 350,
        150, 300, 650, 500, 150, 300, 500, 350,
        150, 300, 500, 150, 300, 500, 350, 150,
        300, 650, 500, 350, 150, 300, 500, 350,
        150, 300, 500, 500, 500, 500, 350, 150,
        300, 500, 500, 350, 150, 300, 500, 350,
        150, 300, 500, 350, 150, 300, 500, 500,
        350, 150, 300, 500, 500, 350, 150, 300,
    };

    void setup_programa2(uint pin)
    {
        gpio_set_function(pin, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(pin);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 4.0f);
        pwm_init(slice_num, &config, true);
        pwm_set_gpio_level(pin, 0);
    }

    void play_tone(uint pin, uint frequency, uint duration_ms)
    {
        uint slice_num = pwm_gpio_to_slice_num(pin);
        uint32_t clock_freq = clock_get_hz(clk_sys);
        uint32_t top = clock_freq / frequency - 1;

        pwm_set_wrap(slice_num, top);
        pwm_set_gpio_level(pin, top / 2);

        sleep_ms(duration_ms);

        pwm_set_gpio_level(pin, 0);
        sleep_ms(50);
    }

    void play_star_wars(uint pin)
    {
        for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++)
        {
            if (gpio_get(SW) == 0)
            {
                return;
            }
            if (star_wars_notes[i] == 0)
            {
                sleep_ms(note_duration[i]);
            }
            else
            {
                if (gpio_get(SW) == 0)
                {
                    return;
                }
                play_tone(pin, star_wars_notes[i], note_duration[i]);
            }
        }
    }

    setup_programa2(BUZZER_PIN);
    while (1)
    {
        play_star_wars(BUZZER_PIN);
        if (gpio_get(SW) == 0)
        {
            return;
        }
    }
}

void programa3()
{
    sleep_ms(200);
    const uint LED = 12;
    const uint16_t PERIOD = 2000;
    const float DIVIDER_PWM = 16.0;
    const uint16_t LED_STEP = 100;
    uint16_t led_level = 100;

    void setup_programa3()
    {
        uint slice;
        gpio_set_function(LED, GPIO_FUNC_PWM);
        slice = pwm_gpio_to_slice_num(LED);
        pwm_set_clkdiv(slice, DIVIDER_PWM);
        pwm_set_wrap(slice, PERIOD);
        pwm_set_gpio_level(LED, led_level);
        pwm_set_enabled(slice, true);
    }

    stdio_init_all();
    setup_programa3();
    uint up_down = 1;

    while (1)
    {
        pwm_set_gpio_level(LED, led_level);
        sleep_ms(1000);
        for (int i = 0; i < 10; i++)
        {
            sleep_ms(100);
            if (gpio_get(SW) == 0)
            {
                pwm_set_gpio_level(LED, 0);
                return;
            }
        }
        if (up_down)
        {
            led_level += LED_STEP;
            if (led_level >= PERIOD)
                up_down = 0;
        }
        else
        {
            led_level -= LED_STEP;
            if (led_level <= LED_STEP)
                up_down = 1;
        }
    }
}

int main()
{
    uint16_t vry_value;
    bool button_pressed = false;

    setup();
    display_menu();

    while (1)
    {
        joystick_read_axis(&vry_value);

        if (vry_value < 1000)
        {
            selected_option = (selected_option + 1) % 3;
            display_menu();
            sleep_ms(200);
        }
        else if (vry_value > 3000)
        {
            selected_option = (selected_option - 1 + 3) % 3;
            display_menu();
            sleep_ms(200);
        }

        if (gpio_get(SW) == 0)
        {
            if (!button_pressed)
            {
                button_pressed = true;
                printf("Opcao selecionada: %d\n", selected_option + 1);
                switch (selected_option)
                {
                case 0:
                    programa1();
                    break;
                case 1:
                    programa2();
                    break;
                case 2:
                    programa3();
                    break;
                }
            }
        }
        else
        {
            button_pressed = false;
        }

        sleep_ms(50);
    }
}
