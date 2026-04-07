#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

// definindo GPIO dos botões
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_BOTAO_J 22

// definindo GPIO dos LEDs
#define PINO_LED_G 11
#define PINO_LED_B 12
#define PINO_LED_R 13


// definindo GPIO dos eixos X e Y
#define PINO_X 26
#define PINO_Y 27


volatile bool azul_ligado = false;
volatile bool verde_ligado = false;
volatile bool vermelho_ligado = false;

void configurar_botoes(){
    // Configurando botão A
    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);
    // Configurando botão B
    gpio_init(PINO_BOTAO_B);
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_B);
    // Configurando botão do Joystick
    gpio_init(PINO_BOTAO_J);
    gpio_set_dir(PINO_BOTAO_J, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_J);

}

void configurar_eixoXY(){
    adc_gpio_init(PINO_X);
    adc_gpio_init(PINO_Y);
}

void configurar_leds(){
    gpio_init(PINO_LED_B);
    gpio_set_dir(PINO_LED_B, GPIO_OUT);
    gpio_init(PINO_LED_G);
    gpio_set_dir(PINO_LED_G, GPIO_OUT);
    gpio_init(PINO_LED_R);
    gpio_set_dir(PINO_LED_R, GPIO_OUT);
}

void ligar_pinos_callback(uint gpio, uint32_t events){
    if(gpio == PINO_BOTAO_A){
        bool estado_atual = gpio_get(PINO_LED_G);
        gpio_put(PINO_LED_G, !estado_atual);

        verde_ligado = true;
    }else if(gpio == PINO_BOTAO_B){
        bool estado_atual = gpio_get(PINO_LED_B);
        gpio_put(PINO_LED_B, !estado_atual);

        azul_ligado = true;
    }else if(gpio == PINO_BOTAO_J){
        bool estado_atual = gpio_get(PINO_LED_R);
        gpio_put(PINO_LED_R, !estado_atual);

        vermelho_ligado = true;
    }
}



int main()
{

    adc_init();

    stdio_init_all();

    configurar_eixoXY();

    configurar_botoes();

    configurar_leds();

    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &ligar_pinos_callback);

    gpio_set_irq_enabled(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true);

    gpio_set_irq_enabled(PINO_BOTAO_J, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        adc_select_input(0); 
        uint16_t vrx_value = adc_read(); 

        adc_select_input(1); 
        uint16_t vry_value = adc_read();

        if (vrx_value > 2100) {
            gpio_put(PINO_LED_G, true); 
        } else {
            gpio_put(PINO_LED_G, false); 
        }


        if (vry_value > 2100) {
            gpio_put(PINO_LED_B, true); 
        } else {
            gpio_put(PINO_LED_B, false); 
        }
        printf("VRX: %u, VRY: %u\n", vrx_value, vry_value);

        sleep_ms(500);
    }

    return 0;
}