#include <stdio.h>
#include "pico/stdlib.h"

#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_LED_G 12
#define PINO_LED_B 13


volatile bool azul_ligado = false;
volatile bool verde_ligado = false;

void configurar_botoes(){
    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);
    gpio_init(PINO_BOTAO_B);
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_B);
}

void configurar_leds(){
    gpio_init(PINO_LED_B);
    gpio_set_dir(PINO_LED_B, GPIO_OUT);
    gpio_init(PINO_LED_G);
    gpio_set_dir(PINO_LED_G, GPIO_OUT);
}

void ligar_pinos_callback(uint gpio, uint32_t events){
    if(gpio == PINO_BOTAO_A){
        bool estado_atual = gpio_get(PINO_LED_G);
        gpio_put(PINO_LED_G, !estado_atual);

        azul_ligado = true;
    }else if(gpio == PINO_BOTAO_B){
        bool estado_atual = gpio_get(PINO_LED_B);
        gpio_put(PINO_LED_B, !estado_atual);

        verde_ligado = true;
    }
}



int main()
{
    stdio_init_all();

    configurar_botoes();

    configurar_leds();

    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &ligar_pinos_callback);

    gpio_set_irq_enabled(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        
    }
}
