#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ADRESS 0x3C
#define ADC_PIN 28
#define BUTTON_B 6
#define REFERENCE 10000
#define ADC_VREF 3.31
#define ADC_RESOLUTION 4095

volatile uint32_t button_b_time = 0;
ssd1306_t ssd;
float resistance = 0.0;
float mean = 0.0;
int first_band = 0;
int second_band = 0;
int multiplier = 0;
float margin_error = 0.0;

const float E24[] = {
    10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30,
    33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91
  };
  
const char* colors[] = {
    "Black", "Brown", "Red", "Orange", "Yellow",
    "Green", "Blue", "Purple", "Gray", "White",
};

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

    int commercialResistance = (int)(resistance + 0.5);
    first_band = commercialResistance / 10;
    second_band = commercialResistance % 10;
    margin_error = (nearest_value - resistance) / nearest_value * 100.0f;
    margin_error = fabs(margin_error);

    return nearest_value * pow(10, multiplier);
}

bool debounce(volatile uint32_t *last_time){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time > 250){ 
        *last_time = current_time;
        return true;
    }
    return false;
}

void gpio_irq_handler(uint gpio, uint32_t events){
  if(gpio == BUTTON_B) {
    static volatile uint32_t last_time = 0;
    if (debounce(&last_time)) {
        printf("Button pressed\n");
        ssd1306_fill(&ssd, false);
        ssd1306_send_data(&ssd);
        reset_usb_boot(0, 0);
    }
  }
}

void button_init() {
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

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
  
int main(){
    stdio_init_all();
    button_init();
    display_init();
    adc_init();
    adc_gpio_init(ADC_PIN);

    while (true) {
        read_adc();
        show_display();
        sleep_ms(1000);
    }
}
