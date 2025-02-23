#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h" // Adicionado para controle PWM dos buzzers
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"

// Definições de constantes para LEDs WS2812
#define LED_COUNT 25
#define LED_PIN 7

// Definições de pinos I2C para OLED e botões
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
const uint BUTTON_A = 5; // Pino do botão A (assumido como GP5)
const uint BUTTON_B = 6; // Pino do botão B (assumido como GP6)
const uint BUZZER1_PIN = 21; // Pino do buzzer 1 (assumido GP21)
const uint BUZZER2_PIN = 10; // Pino do buzzer 2 (assumido GP10)

// Estrutura para pixel RGB (formato GRB usado pelos WS2812)
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

// Buffer global para os 25 LEDs
npLED_t leds[LED_COUNT];

// Variáveis globais para controle do PIO
PIO np_pio;
uint sm;

/**
 * Define a cor RGB de um LED específico no buffer
 * @param index: Índice do LED (0 a 24)
 * @param r: Vermelho (0-255)
 * @param g: Verde (0-255)
 * @param b: Azul (0-255)
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de LEDs, apagando todos (0,0,0)
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Inicializa o PIO para controlar os LEDs WS2812
 * @param pin: Pino GPIO para os LEDs
 */
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    npClear();
}

/**
 * Escreve os dados do buffer nos LEDs físicos
 */
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

/**
 * Mapeia coordenadas 2D para índice linear na matriz de LEDs
 * @param x: Coluna (0 a 4)
 * @param y: Linha (0 a 4)
 * @return: Índice linear (0 a 24)
 */
int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return 24 - (y * 5 + x);
    } else {
        return 24 - (y * 5 + (4 - x));
    }
}

/**
 * Exibe segundos em binário nos LEDs com cor verde
 * @param num: Segundos (0 a 59)
 */
void secToLed(int num) {
    int positions[] = {0, 9, 10, 19, 20, 1};
    for (int i = 0; i < 6; i++) {
        if (num & (1 << i)) {
            npSetLED(positions[i], 0, 25, 0);
        }
    }
}

/**
 * Exibe minutos em binário nos LEDs com cor azul
 * @param num: Minutos (0 a 59)
 */
void minToLed(int num) {
    int positions[] = {2, 7, 12, 17, 22, 3};
    for (int i = 0; i < 6; i++) {
        if (num & (1 << i)) {
            npSetLED(positions[i], 0, 0, 25);
        }
    }
}

/**
 * Exibe horas em binário nos LEDs com cor vermelha
 * @param num: Horas (0 a 31)
 */
void hourToLed(int num) {
    int positions[] = {4, 5, 14, 15, 24};
    for (int i = 0; i < 5; i++) {
        if (num & (1 << i)) {
            npSetLED(positions[i], 25, 0, 0);
        }
    }
}

/**
 * Emite um som no buzzer 1 (1000 Hz)
 * @param duration_ms: Duração do som em milissegundos
 */
void buzzer1_beep(uint32_t duration_ms) {
    pwm_set_gpio_level(BUZZER1_PIN, 125); // Duty cycle ~50%
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER1_PIN, 0); // Desliga o buzzer
}

/**
 * Emite um som no buzzer 2 (500 Hz)
 * @param duration_ms: Duração do som em milissegundos
 */
void buzzer2_beep(uint32_t duration_ms) {
    pwm_set_gpio_level(BUZZER2_PIN, 125); // Duty cycle ~50%
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER2_PIN, 0); // Desliga o buzzer
}

/**
 * Função principal: Cronômetro com início/pausa via botão A, reset via botão B com LEDs apagados e sons nos buzzers
 */
int main() {
    int second = 0, minute = 0, hour = 0; // Contadores de tempo
    uint64_t last_time = 0; // Último tempo registrado em milissegundos
    uint64_t last_toggle_time = 0; // Último tempo de alternância do texto no OLED
    bool is_running = false; // Estado do cronômetro: false = parado, true = rodando
    bool display_toggle = false; // Controla a alternância do texto no OLED
    bool is_reset_prompt = false; // Estado para exibir mensagem de reset

    stdio_init_all(); // Inicializa comunicação serial via USB

    // Inicializa I2C para o OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED SSD1306
    ssd1306_init();

    // Define a área de renderização do OLED (128x64 pixels, 8 páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // Buffer para o OLED
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    npInit(LED_PIN); // Inicializa os LEDs WS2812

    // Configura o botão A (GP5) como entrada com pull-up
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Configura o botão B (GP6) como entrada com pull-up
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Configura PWM para buzzer 1 (1000 Hz)
    gpio_set_function(BUZZER1_PIN, GPIO_FUNC_PWM);
    uint slice_num1 = pwm_gpio_to_slice_num(BUZZER1_PIN);
    pwm_set_wrap(slice_num1, 2499); // Frequência base ajustada
    pwm_set_clkdiv(slice_num1, 50.0f); // Divide para ~1000 Hz (125MHz / (2500 * 50) = 1000 Hz)
    pwm_set_enabled(slice_num1, true);

    // Configura PWM para buzzer 2 (500 Hz)
    gpio_set_function(BUZZER2_PIN, GPIO_FUNC_PWM);
    uint slice_num2 = pwm_gpio_to_slice_num(BUZZER2_PIN);
    pwm_set_wrap(slice_num2, 4999); // Frequência base ajustada
    pwm_set_clkdiv(slice_num2, 50.0f); // Divide para ~500 Hz (125MHz / (5000 * 50) = 500 Hz)
    pwm_set_enabled(slice_num2, true);

    while (true) {
        // Verifica os botões
        if (!gpio_get(BUTTON_A) && !is_reset_prompt) { // Botão A inicia/pausa
            is_running = !is_running;
            buzzer1_beep(100); // Som de 1000 Hz por 100 ms
            sleep_ms(200); // Debounce
        } else if (!gpio_get(BUTTON_B) && !is_reset_prompt) { // Botão B inicia prompt de reset
            is_reset_prompt = true;
            is_running = false;
            buzzer2_beep(100); // Som de 500 Hz por 100 ms
            sleep_ms(200); // Debounce
        } else if (is_reset_prompt) { // Estado de espera por confirmação de reset
            if (!gpio_get(BUTTON_B)) { // Botão B confirma reset
                second = 0;
                minute = 0;
                hour = 0;
                npClear(); // Apaga todos os LEDs
                npWrite(); // Atualiza os LEDs para refletir o estado apagado
                buzzer2_beep(100); // Som de 500 Hz por 100 ms
                is_reset_prompt = false;
                sleep_ms(200); // Debounce
            } else if (!gpio_get(BUTTON_A)) { // Botão A cancela reset e continua
                is_reset_prompt = false;
                is_running = true;
                buzzer1_beep(100); // Som de 1000 Hz por 100 ms
                sleep_ms(200); // Debounce
            }
        }

        uint64_t current_time = time_us_64() / 1000; // Tempo atual em milissegundos

        if (!is_running && !is_reset_prompt) { // Estado pausado normal
            if (current_time - last_toggle_time >= 1000) { // Alterna texto a cada 1s
                display_toggle = !display_toggle;
                last_toggle_time = current_time;
            }
            memset(ssd, 0, ssd1306_buffer_length); // Limpa o buffer do OLED
            if (display_toggle) {
                char time_str[9];
                sprintf(time_str, "%02d:%02d:%02d", hour, minute, second);
                ssd1306_draw_string(ssd, 0, 0, time_str);
            } else {
                ssd1306_draw_string(ssd, 0, 0, (second == 0 && minute == 0 && hour == 0) ? "Press A to start" : "Paused");
            }
            render_on_display(ssd, &frame_area); // Atualiza o OLED
            // LEDs permanecem como estavam
        } else if (is_reset_prompt) { // Estado de espera por confirmação de reset
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 0, 0, "Press B to reset"); // Mensagem de reset
            render_on_display(ssd, &frame_area);
            // LEDs permanecem como estavam até confirmação
        } else { // Estado rodando
            if (current_time - last_time >= 1000) { // Passou 1 segundo?
                last_time = current_time;
                second++;
                if (second >= 60) {
                    second = 0;
                    minute++;
                    if (minute >= 60) {
                        minute = 0;
                        hour++;
                        if (hour >= 32) {
                            second = 0;
                            minute = 0;
                            hour = 0;
                        }
                    }
                }
            }
            // Formata e exibe o tempo no OLED
            char time_str[9];
            sprintf(time_str, "%02d:%02d:%02d", hour, minute, second);
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 0, 0, time_str);
            render_on_display(ssd, &frame_area);

            // Atualiza os LEDs WS2812 com valores binários
            npClear();
            secToLed(second);
            minToLed(minute);
            hourToLed(hour);
            npWrite();
        }
        sleep_ms(10); // Pequeno delay para evitar sobrecarga da CPU
    }
    return 0;
}