#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

// definindo GPIO dos botões
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_BOTAO_J 22

// definindo GPIO dos LEDs
#define PWM_PIN_G 11
#define PWM_PIN_B 12
#define PWM_PIN_R 13

int WRAP = 50000; // RESOLUÇÃO, QUANTO MAIOR, MAIOR A POSSIBILIDADE DE VALORES MAIS EXATOS.
int duty_cycle = 0; // DEFINE O NIVEL DO DUTY CYCLE
int clk_div = 50;

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

void configurar_leds(unsigned int slice_num_G, unsigned int slice_num_B){
    gpio_set_function(PWM_PIN_G, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN_B, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN_R, GPIO_FUNC_PWM);

    pwm_set_clkdiv(slice_num_G, clk_div);
    pwm_set_wrap(slice_num_G, WRAP);
    pwm_set_gpio_level(PWM_PIN_G, (duty_cycle*WRAP/4095)); 
    pwm_set_clkdiv(slice_num_B, clk_div); 
    pwm_set_wrap(slice_num_B, WRAP);
    pwm_set_gpio_level(PWM_PIN_B, (duty_cycle*WRAP/4095));
    pwm_set_gpio_level(PWM_PIN_R, (duty_cycle*WRAP/4095));

}

void ligar_pinos_callback(uint gpio, uint32_t events){
    if(gpio == PINO_BOTAO_A){
        // sleep_ms(50);
        //     if(gpio_get(PINO_BOTAO_A) == 0){
        //         while(gpio_get(PINO_BOTAO_A) == 0){
        //             sleep_ms(50);
        //         }
        //     }
        // duty_cycle_R += 20;
        // if(duty_cycle_R == 100){
        //     duty_cycle_R = 0;
        // }
        
        // pwm_set_gpio_level(PWM_PIN_R, (duty_cycle_R*WRAP/100));
    }else if(gpio == PINO_BOTAO_B){
        // bool estado_atual = gpio_get(PINO_LED_B);
        // gpio_put(PINO_LED_B, !estado_atual);

        // azul_ligado = true;
    }else if(gpio == PINO_BOTAO_J){
        // bool estado_atual = gpio_get(PINO_LED_R);
        // gpio_put(PINO_LED_R, !estado_atual);

        // vermelho_ligado = true;
    }
}



int main()
{

    adc_init();

    stdio_init_all();

    configurar_eixoXY();

    configurar_botoes();

    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &ligar_pinos_callback);

    gpio_set_irq_enabled(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true);

    gpio_set_irq_enabled(PINO_BOTAO_J, GPIO_IRQ_EDGE_FALL, true);
    
    int slice_num_G = pwm_gpio_to_slice_num(PWM_PIN_G);
    int slice_num_B = pwm_gpio_to_slice_num(PWM_PIN_B);
    
    configurar_leds(slice_num_G, slice_num_B);

    pwm_set_enabled(slice_num_G, true);
    pwm_set_enabled(slice_num_B, true);

    while (true) {

        adc_select_input(1); 
        uint16_t vrx_value = adc_read(); 

        adc_select_input(0); 
        uint16_t vry_value = adc_read();
        
        int duty_cycle_G = vrx_value;
        int duty_cycle_B = vry_value;
        int duty_cycle_R = 4095 - vrx_value;
        if (duty_cycle_B > 3700) duty_cycle_G = 0;
        if (duty_cycle_G < 500) duty_cycle_G = 0;
        if (duty_cycle_B < 500) duty_cycle_B = 0;
        if (duty_cycle_R < 500) duty_cycle_R = 0;

        pwm_set_gpio_level(PWM_PIN_G, (duty_cycle_G*WRAP/4095));
        printf("Duty Cycle: %d%%\n ", (duty_cycle_G * 100)/8190);
        pwm_set_gpio_level(PWM_PIN_B, (duty_cycle_B*WRAP/4095));
        printf("Duty Cycle: %d%%\n ", (duty_cycle_B * 100)/4095);
        pwm_set_gpio_level(PWM_PIN_R, (duty_cycle_R*WRAP/4095));
        printf("Duty Cycle: %d%%\n ", (duty_cycle_R * 100)/4095);

        printf("VRX: %u, VRY: %u\n", vrx_value, vry_value);

        sleep_ms(50);
    }

    return 0;
}