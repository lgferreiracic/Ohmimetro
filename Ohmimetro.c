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
#define endereco 0x3C
#define ADC_PIN 28 
#define Botao_B 6  

int R_conhecido = 10000;   
float R_x = 0.0;           
float ADC_VREF = 3.31;     
int ADC_RESOLUTION = 4095; 

const float E24[] = {
    10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30,
    33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91
  };
  
const char* cores[] = {
    "Preto", "Marrom", "Vermelho", "Laranja", "Amarelo",
    "Verde", "Azul", "Violeta", "Cinza", "Branco"
};

float encontrar_e24_mais_proximo(float valor) {
    int potencia = 0;
    while (valor >= 100) {
        valor /= 10;
        potencia++;
    }
    while (valor < 10) {
        valor *= 10;
        potencia--;
    }

    float mais_proximo = E24[0];
    float menor_erro = fabs(valor - E24[0]);

    for (int i = 1; i < 24; i++) {
        float erro = fabs(valor - E24[i]);
        if (erro < menor_erro) {
        menor_erro = erro;
        mais_proximo = E24[i];
        }
    }

    return mais_proximo * pow(10, potencia);
}

void calcular_faixas(float valor, int *d1, int *d2, int *mult) {
    int pot = 0;

    while (valor >= 100) {
        valor /= 10;
        pot++;
    }
    while (valor < 10) {
        valor *= 10;
        pot--;
    }

    int inteiro = (int)(valor + 0.5); 

    *d1 = inteiro / 10;
    *d2 = inteiro % 10;
    *mult = pot;
}

void gpio_irq_handler(uint gpio, uint32_t events){
  reset_usb_boot(0, 0);
}
  

int main()
{
    stdio_init_all();
    gpio_init(Botao_B);
    gpio_set_dir(Botao_B, GPIO_IN);
    gpio_pull_up(Botao_B);
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(ADC_PIN);
    
    float tensao;
    char str_x[5];
    char str_y[5];
    char faixa1[10], faixa2[10], faixa3[10];

    while (true) {
        adc_select_input(2);

        float soma = 0.0f;
        for (int i = 0; i < 500; i++){
            soma += adc_read();
            sleep_ms(1);
        }
        float media = soma / 500.0f;

        R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

        float R_comercial = encontrar_e24_mais_proximo(R_x);
        int d1, d2, mult;
        calcular_faixas(R_comercial, &d1, &d2, &mult);

        sprintf(str_x, "%1.0f", media); 
        sprintf(str_y, "%1.0f", R_comercial);  
        sprintf(faixa1, "%s", cores[d1]);
        sprintf(faixa2, "%s", cores[d2]);
        sprintf(faixa3, "%s", cores[mult]);

        ssd1306_fill(&ssd, true);
        ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
        ssd1306_line(&ssd, 3, 25, 123, 25, true);
        ssd1306_line(&ssd, 3, 37, 123, 37, true);
        ssd1306_draw_string(&ssd, faixa1, 8, 6);
        ssd1306_draw_string(&ssd, faixa2, 20, 16);
        ssd1306_draw_string(&ssd, faixa3, 10, 28);
        ssd1306_draw_string(&ssd, "ADC", 13, 41);
        ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
        ssd1306_line(&ssd, 44, 37, 44, 60, true);
        ssd1306_draw_string(&ssd, str_x, 8, 52);
        ssd1306_draw_string(&ssd, str_y, 59, 52);
        ssd1306_send_data(&ssd);

        sleep_ms(1000);
    }
}
