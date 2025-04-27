#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "ws2812.pio.h"

//Constantes
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ADRESS 0x3C
#define ADC_PIN 28
#define BUTTON_B 6
#define WS2812_PIN 7
#define REFERENCE 10000
#define ADC_VREF 3.31
#define ADC_RESOLUTION 4095
#define NUM_PIXELS 25

//Estrutura para armazenar os valores RGB
typedef struct {
    double R; 
    double G; 
    double B; 
} RGB;

//Variaveis globais
volatile uint32_t button_b_time = 0;
ssd1306_t ssd;
PIO pio; 
uint sm; 
float resistance = 0.0;
float mean = 0.0;
int first_band = 0;
int second_band = 0;
int multiplier = 0;
float margin_error = 0.0;

//Valores E24
const float E24[] = {
    10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30,
    33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91
};

//Definindo os valores de cor para cada faixa
const char* colors[] = {
    "Black", "Brown", "Red", "Orange", "Yellow",
    "Green", "Blue", "Purple", "Gray", "White",
};

//Função para encontrar o valor E24 mais próximo
float find_e24_nearest(float resistance) {
    multiplier = 0;
    while (resistance >= 100) {
        resistance /= 10;
        multiplier++;
    }
    while (resistance < 10) {
        resistance *= 10;
        multiplier--;
    }

    float nearest_value = E24[0];
    float smallest_error = fabs(resistance - E24[0]);

    for (int i = 1; i < 24; i++) {
        float error = fabs(resistance - E24[i]);
        if (error < smallest_error) {
            smallest_error = error;
            nearest_value = E24[i];
        }
    }

    int commercialResistance = nearest_value;
    first_band = commercialResistance / 10;
    second_band = commercialResistance % 10;
    margin_error = (nearest_value - resistance) / nearest_value * 100.0f;
    margin_error = fabs(margin_error);

    printf("Resistance: %.2f Ohm\n", resistance);
    printf("Nearest E24: %.2f Ohm\n", nearest_value);
    printf("First Band: %d\n", first_band);
    printf("Second Band: %d\n", second_band);
    printf("Multiplier: %d\n", multiplier);
    printf("Margin of Error: %.2f%%\n", margin_error);

    return nearest_value * pow(10, multiplier);
}

//Função de debounce para evitar múltiplas leituras do botão
bool debounce(volatile uint32_t *last_time){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time > 250){ 
        *last_time = current_time;
        return true;
    }
    return false;
}

//Função de inicialização do botão
void button_init() {
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

//Função de inicialização do display OLED SSD1306
void display_init() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ADRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

//Função para inicializar a matriz de LEDs WS2812
uint matrix_init() {
    pio = pio0; 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
}

//Função para obter o valor RGB em formato uint32_t
uint32_t matrix_rgb(double r, double g, double b){
   unsigned char R, G, B;
   R = r * 255;
   G = g * 255;
   B = b * 255;
   return (G << 24) | (R << 16) | (B << 8);
}

//Função para obter o índice pixel na matriz de LEDs
int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return 24-(y * 5 + x); 
    } else {
        return 24-(y * 5 + (4 - x)); 
    }
}

//Função para desenhar os pixels na matriz de LEDs
void desenho_pio(RGB pixels[NUM_PIXELS], PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        int x = i % 5;
        int y = i / 5;
        int index = getIndex(x, y);
        pio_sm_put_blocking(pio, sm, matrix_rgb(pixels[index].R, pixels[index].G, pixels[index].B));
    }
}

//Função para selecionar a cor da faixa com base no número da faixa
RGB select_band_color(int band) {
    switch (band) {
        case 0: return (RGB){0, 0, 0}; // Black
        case 1: return (RGB){0.07, 0.025, 0}; // Brown
        case 2: return (RGB){0.1, 0, 0}; // Red
        case 3: return (RGB){0.05, 0.005, 0}; // Orange
        case 4: return (RGB){0.1, 0.1, 0}; // Yellow
        case 5: return (RGB){0, 0.1, 0}; // Green
        case 6: return (RGB){0, 0, 0.1}; // Blue
        case 7: return (RGB){0.05, 0, 0.05}; // Purple
        case 8: return (RGB){0.005, 0.005, 0.005}; // Gray
        case 9: return (RGB){0.1, 0.1, 0.1}; // White
        default: return (RGB){0.1, 0.1, 0.1}; // Default to white if invalid band
    }
}

//Função para mostrar a matriz de LEDs com as cores das faixas
void show_matrix() {
    RGB pixels[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++) {
        int x = i % 5;
        int y = i / 5;
        int index = getIndex(x, y);
        if(i < 5) {
            pixels[index] = select_band_color(multiplier);
        } else if(i < 15) {
            pixels[index] = select_band_color(second_band);
        } else { 
            pixels[index] = select_band_color(first_band);
        }
    }
    desenho_pio(pixels, pio, sm);
}

//Função para limpar a matriz de LEDs
void clear_matrix(){
    RGB BLACK = {0, 0, 0}; 
    RGB pixels[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++) {
        pixels[i] = BLACK;
    }
    desenho_pio(pixels, pio0, 0);
}

//Função para realizar a leitura do ADC
void read_adc() {
    adc_select_input(2);
    float sum = 0.0f;
    for (int i = 0; i < 500; i++){
        sum += adc_read();
        sleep_ms(1);
    }
    mean = sum / 500.0f;
    resistance = (REFERENCE * mean) / (ADC_RESOLUTION - mean);
}

//Função para mostrar os dados no display OLED
void show_display() {
    char str_adc_value[5], str_resistance[5], str_margin_error[5];
    char str_first_band[10], str_second_band[10], str_multiplier[10];

    sprintf(str_adc_value, "%1.0f", mean); 
    sprintf(str_resistance, "%1.0f", find_e24_nearest(resistance));
    sprintf(str_margin_error, "%1.2f%%", margin_error);  
    sprintf(str_first_band, "1: %s", colors[first_band]);
    sprintf(str_second_band, "2: %s", colors[second_band]);
    sprintf(str_multiplier, "3: %s", colors[multiplier]);

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
    ssd1306_line(&ssd, 3, 37, 123, 37, true);
    ssd1306_line(&ssd, 48, 3, 48, 60, true);

    ssd1306_draw_string(&ssd, "Error", 6, 6);
    ssd1306_draw_string(&ssd, str_margin_error, 6, 20);

    ssd1306_draw_string(&ssd, str_first_band, 52, 6);
    ssd1306_draw_string(&ssd, str_second_band, 52, 18);
    ssd1306_draw_string(&ssd, str_multiplier, 52, 28);
    
    ssd1306_draw_string(&ssd, "ADC", 13, 41);
    ssd1306_draw_string(&ssd, str_adc_value, 8, 52);

    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
    ssd1306_draw_string(&ssd, str_resistance, 59, 52);
    
    ssd1306_send_data(&ssd);
}

//Função de interrupção para o botão
void gpio_irq_handler(uint gpio, uint32_t events){
    if(gpio == BUTTON_B) {
        static volatile uint32_t last_time = 0;
        if (debounce(&last_time)) {
            printf("Button pressed\n");
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            clear_matrix();
            reset_usb_boot(0, 0);
        }
    }
}
  
int main(){
    stdio_init_all();
    button_init();
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    display_init();
    matrix_init();
    adc_init();
    adc_gpio_init(ADC_PIN);

    while (true) {
        read_adc();
        show_display();
        show_matrix();
        sleep_ms(1000);
    }
}
