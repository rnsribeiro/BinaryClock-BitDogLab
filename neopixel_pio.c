#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h" // Biblioteca para controle de PIO (Programmable I/O)
#include "hardware/clocks.h" // Biblioteca para acesso ao clock do sistema
#include "ws2818b.pio.h" // Biblioteca gerada pelo arquivo .pio para protocolo WS2818B

// Definições de constantes
#define LED_COUNT 25 // Número total de LEDs na matriz (5x5)
#define LED_PIN 7 // Pino GPIO usado para controlar os LEDs WS2812

// Estrutura para representar um pixel RGB (formato GRB usado pelos WS2812)
struct pixel_t {
    uint8_t G, R, B; // Valores de 8 bits para verde (G), vermelho (R) e azul (B)
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Renomeação para clareza (npLED_t = NeoPixel LED)

// Buffer global para armazenar as cores dos 25 LEDs
npLED_t leds[LED_COUNT];

// Variáveis globais para controle do PIO
PIO np_pio; // Instância do PIO usada (pio0 ou pio1)
uint sm; // Máquina de estado (state machine) do PIO

/**
 * Define a cor RGB de um LED específico no buffer
 * @param index: Índice do LED na cadeia (0 a 24)
 * @param r: Valor de vermelho (0-255)
 * @param g: Valor de verde (0-255)
 * @param b: Valor de azul (0-255)
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de LEDs, definindo todos como apagados (0,0,0)
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Inicializa o PIO para controlar os LEDs WS2812
 * @param pin: Pino GPIO usado para enviar dados aos LEDs
 */
void npInit(uint pin) {
    // Carrega o programa PIO para o bloco pio0
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Reivindica uma máquina de estado livre no PIO
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) { // Se não houver máquina livre em pio0, tenta pio1
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Panic se não houver máquina disponível
    }

    // Configura e inicia o programa PIO na máquina de estado escolhida
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // 800kHz para protocolo WS2812

    // Inicializa o buffer com todos os LEDs apagados
    npClear();
}

/**
 * Escreve os dados do buffer nos LEDs físicos
 */
void npWrite() {
    // Envia os valores GRB de cada LED para o PIO em sequência
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Azul
    }
    sleep_us(100); // Aguarda 100us para o sinal de RESET conforme datasheet WS2812
}

/**
 * Mapeia coordenadas 2D (x, y) para um índice linear na cadeia de LEDs
 * Considera layout em zigue-zague: linhas pares da esquerda para direita, ímpares da direita para esquerda
 * @param x: Coluna (0 a 4)
 * @param y: Linha (0 a 4)
 * @return: Índice linear correspondente (0 a 24)
 */
int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return 24 - (y * 5 + x); // Linhas pares: ordem direta
    } else {
        return 24 - (y * 5 + (4 - x)); // Linhas ímpares: ordem invertida
    }
}

/**
 * Exibe segundos em binário na matriz de LEDs com cor verde
 * @param num: Valor dos segundos (0 a 59)
 */
void secToLed(int num) {
    int positions[] = {0, 9, 10, 19, 20, 1}; // Posições dos LEDs para bits b0 a b5
    for (int i = 0; i < 6; i++) {
        if (num & (1 << i)) { // Verifica se o bit i está ativo
            npSetLED(positions[i], 0, 25, 0); // Acende o LED em verde para bit 1
        }
    }
}

/**
 * Exibe minutos em binário na matriz de LEDs com cor azul
 * @param num: Valor dos minutos (0 a 59)
 */
void minToLed(int num) {
    int positions[] = {2, 7, 12, 17, 22, 3}; // Posições dos LEDs para bits b0 a b5
    for (int i = 0; i < 6; i++) {
        if (num & (1 << i)) { // Verifica se o bit i está ativo
            npSetLED(positions[i], 0, 0, 25); // Acende o LED em azul para bit 1
        }
    }
}

/**
 * Exibe horas em binário na matriz de LEDs com cor vermelha
 * @param num: Valor das horas (0 a 31)
 */
void hourToLed(int num) {
    int positions[] = {4, 5, 14, 15, 24}; // Posições dos LEDs para bits b0 a b4
    for (int i = 0; i < 5; i++) {
        if (num & (1 << i)) { // Verifica se o bit i está ativo
            npSetLED(positions[i], 25, 0, 0); // Acende o LED em vermelho para bit 1
        }
    }
}

/**
 * Função principal: Implementa um cronômetro que conta segundos, minutos e horas,
 * reiniciando após 32 horas, e exibe os valores na matriz de LEDs
 */
int main() {
    int second = 0, minute = 0, hour = 0; // Contadores de segundos, minutos e horas
    uint64_t last_time = 0; // Último tempo registrado em milissegundos

    stdio_init_all(); // Inicializa comunicação serial via USB
    npInit(LED_PIN); // Inicializa a matriz de LEDs no pino especificado

    while (true) { // Loop infinito do cronômetro
        uint64_t current_time = time_us_64() / 1000; // Obtém tempo atual em milissegundos
        if (current_time - last_time >= 1000) { // Verifica se passou 1 segundo
            last_time = current_time; // Atualiza o último tempo
            second++; // Incrementa segundos
            if (second >= 60) { // Se atingiu 60 segundos
                second = 0; // Zera segundos
                minute++; // Incrementa minutos
                if (minute >= 60) { // Se atingiu 60 minutos
                    minute = 0; // Zera minutos
                    hour++; // Incrementa horas
                    if (hour >= 32) { // Se atingiu 32 horas
                        second = 0; // Reinicia segundos
                        minute = 0; // Reinicia minutos
                        hour = 0; // Reinicia horas
                    }
                }
            }
        }
        npClear(); // Limpa o buffer de LEDs antes de atualizar
        secToLed(second); // Exibe os segundos em verde
        minToLed(minute); // Exibe os minutos em azul
        hourToLed(hour); // Exibe as horas em vermelho
        npWrite(); // Envia os dados do buffer para os LEDs
        sleep_ms(10); // Pequeno delay para evitar sobrecarga da CPU
    }
}