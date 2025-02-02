#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "ws2818b.pio.h"

// Definições de pinos e constantes
#define LED_COUNT 25  // Número de LEDs na matriz
#define LED_PIN 7  // Pino onde o LED está conectado
#define LED_RED 13 // Pino para o LED vermelho
#define LED_GREEN 12  // Pino para o LED verde
#define LED_BLUE 11  // Pino para o LED azul
#define BUTTON_A 5 // Pino para o botão A (incremento)
#define BUTTON_B 6 // Pino para o botão B (decremento)

// Variáveis globais
volatile int number = 0;  // Número atual a ser exibido na matriz
volatile bool led_state = false;  // Estado do LED vermelho (pisca-pisca)

// Estrutura que define a cor de cada LED (RGB)
typedef struct {
    uint8_t G, R, B;
} pixel_t;

// Matriz que armazena os LEDs
pixel_t leds[LED_COUNT];

// PIO e estado da máquina de estado (sm) para controle dos LEDs WS2812
PIO np_pio;
uint sm;

// Matriz que representa números de 0 a 9 para a exibição
const int numbers[10][5][5] = {
    {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},  // 0
    {{0,0,1,0,0},{0,0,1,1,0},{1,0,1,0,0},{0,0,1,0,0},{1,1,1,1,1}},  // 1
    {{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,1},{0,0,0,0,1},{1,1,1,1,1}}, // 2
    {{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,1,1,1,1}}, // 3
    {{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,0},{0,0,0,0,1}},  // 4
    {{1,1,1,1,1},{0,0,0,0,1},{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,1}},  // 5
    {{1,1,1,1,1},{0,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,1,1,1,1}},  // 6
    {{1,1,1,1,1},{1,0,0,0,0},{0,1,0,0,0},{0,0,1,0,0},{0,0,1,0,0}},  // 7
    {{1,1,1,1,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,1,1,1,1}},  // 8
    {{1,1,1,1,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,1}}   // 9
};


// Função que inicializa a comunicação PIO para controlar os LEDs WS2812
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);  // Adiciona o programa PIO
    np_pio = pio0;  // Seleciona o PIO 0
    sm = pio_claim_unused_sm(np_pio, false);  // Requisita uma máquina de estado
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);  // Inicializa o programa para controle do LED
}

// Função que envia dados para os LEDs
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);  // Envia o valor do LED verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R);  // Envia o valor do LED vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B);  // Envia o valor do LED azul
    }
    sleep_us(100);  // Aguarda um curto período
}

// Função que desenha o número na matriz de LEDs
void drawNumber(int num) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;  // Calcula o índice do LED
            leds[index].R = numbers[num][y][x] ? 255 : 0;  // Define a cor do LED vermelho
            leds[index].G = 0;  // Desliga o LED verde
            leds[index].B = numbers[num][y][x] ? 255 : 0;  // Define a cor do LED azul
        }
    }
    npWrite();  // Atualiza os LEDs
}

// Função de interrupção para os botões (para incremento ou decremento)
void gpio_callback(uint gpio, uint32_t events) {
    sleep_ms(500);  // Debounce para evitar múltiplas leituras do botão
    if (gpio_get(gpio) == 0) {  // Verifica se o botão foi pressionado (nível baixo)
        if (gpio == BUTTON_A && number < 9) {  // Se for o botão A e o número for menor que 9
            number++;  // Incrementa o número
        }
        if (gpio == BUTTON_B && number > 0) {  // Se for o botão B e o número for maior que 0
            number--;  // Decrementa o número
        }
        drawNumber(number);  // Atualiza a exibição do número
    }
}

// Função que faz o LED vermelho piscar
bool blink_red_led(struct repeating_timer *t) {
    led_state = !led_state;  // Alterna o estado do LED
    gpio_put(LED_RED, led_state);  // Atualiza o LED vermelho
    return true;  // Continua chamando a função repetidamente
}

int main() {
    stdio_init_all();  // Inicializa a comunicação serial
    npInit(LED_PIN);  // Inicializa os LEDs

    // Configura o LED vermelho
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Configura o botão A (incremento)
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Ativa o resistor pull-up para manter o botão em nível alto
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  // Configura interrupção para o botão A

    // Configura o botão B (decremento)
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);  // Ativa o resistor pull-up
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  // Configura interrupção para o botão B

    // Configura o temporizador para piscar o LED vermelho
    struct repeating_timer timer;
    add_repeating_timer_ms(-200, blink_red_led, NULL, &timer);

    // Exibe os números de 0 a 9 na matriz de LEDs
    for (int i = 0; i < 10; i++) {
        drawNumber(i);  // Desenha o número
        sleep_ms(1000);  // Espera 1 segundo antes de mudar para o próximo número
    };

    drawNumber(number);  // Desenha o número inicial

    while (true) {
        tight_loop_contents();  // Loop principal (aguarda interrupções)
    }
}