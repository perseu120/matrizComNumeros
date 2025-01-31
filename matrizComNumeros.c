#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7
#define LED_RED 11
#define LED_GREEN 12
#define LED_BLUE 13
#define BUTTON_A 5
#define BUTTON_B 6

volatile int number = 0; // Número exibido na matriz
volatile bool led_state = false;

typedef struct {
    uint8_t G, R, B;
} pixel_t;

pixel_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Matrizes para representar números de 0 a 9
const int numbers[10][5][5] = {
    {{1,1,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 0
    {{0,0,1,0,0}, {0,1,1,0,0}, {1,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}}, // 1
    {{1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}}, // 2
    {{1,1,1,1,1}, {0,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}, // 3
    {{1,0,0,1,0}, {1,0,0,1,0}, {1,1,1,1,1}, {0,0,0,1,0}, {0,0,0,1,0}}, // 4
    {{1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}, // 5
    {{1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 6
    {{1,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,0,1,0,0}}, // 7
    {{1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 8
    {{1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}  // 9
};

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

void drawNumber(int num) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;
            leds[index].R = numbers[num][y][x] ? 255 : 0;
            leds[index].G = 0;
            leds[index].B = 255;
        }
    }
    npWrite();
}

void gpio_callback(uint gpio, uint32_t events) {
    sleep_ms(50); // Debounce
    if (gpio_get(gpio) == 0) {
        if (gpio == BUTTON_A && number < 9) number++;
        if (gpio == BUTTON_B && number > 0) number--;
        drawNumber(number);
    }
}

bool blink_red_led(struct repeating_timer *t) {
    led_state = !led_state;
    gpio_put(LED_RED, led_state);
    return true;
}

int main() {
    stdio_init_all();
    npInit(LED_PIN);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    struct repeating_timer timer;
    add_repeating_timer_ms(-200, blink_red_led, NULL, &timer);

    drawNumber(number);
    while (true) { tight_loop_contents(); }
}
