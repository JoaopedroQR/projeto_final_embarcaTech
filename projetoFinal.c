#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

//Declaração de variáveis globais para mandar dados ao html
volatile int duty_cycle_G = 0;
volatile int duty_cycle_B = 0;
volatile int duty_cycle_R = 0;

//Configurações wifi
#define WIFI_SSID "REDEWIFI"
#define WIFI_PASS "SENHA"

// definindo GPIO dos botões
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_BOTAO_J 22

// definindo GPIO dos LEDs
#define PWM_PIN_G 11
#define PWM_PIN_B 12
#define PWM_PIN_R 13

// Variáveis do PWM:
int WRAP = 50000; // RESOLUÇÃO, QUANTO MAIOR, MAIOR A POSSIBILIDADE DE VALORES MAIS EXATOS.
int duty_cycle = 0; // DEFINE O NIVEL DO DUTY CYCLE
int clk_div = 50;

// definindo GPIO dos eixos X e Y
#define PINO_X 26
#define PINO_Y 27

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


// Configurando pinos do ADC
void configurar_eixoXY(){
    adc_gpio_init(PINO_X);
    adc_gpio_init(PINO_Y);
}


// Configurando pinos dos leds que utilizaram PWM
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

//Configurando HTML e conectividade wifi
char resposta_http[1024];

char mensagemB1[100] = "Aguardando clique";
char mensagemB2[100] = "Aguardando clique";

void create_http_response() {
    snprintf(resposta_http, sizeof(resposta_http),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<title>Monitor de Botões</title>"
             "<script>"
             "  function buscarStatus() {"
             "    fetch('/status?t=' + new Date().getTime()).then(res => res.text()).then(data => {"
             "      const partes = data.split('|');"
             "      if(partes.length >= 2) {"
             "        document.getElementById('msg_b1').innerText = partes[0];"
             "        document.getElementById('msg_b2').innerText = partes[1];"
             "      }"
             "    }).catch(err => console.error(err));"
             "  }"
             "  setInterval(buscarStatus, 500);" // O navegador checa o servidor a cada 0.5s
             "</script>"
             "</head><body>"
             "  <h1>Estado dos Botões</h1>"
             "  <p><b>Botão A:</b> <span id='msg_b1'>%s</span></p>"
             "  <p><b>Botão B:</b> <span id='msg_b2'>%s</span></p>"
             "</body></html>\r\n",
             mensagemB1, mensagemB2);
}

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    if (strstr(request, "GET /status")) {
        char status_raw[128];
        snprintf(status_raw, sizeof(status_raw), "%s|%s", mensagemB1, mensagemB2);

        char response[256];
        // Adicionado Content-Length e removido um espaço extra
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n\r\n"
                "%s",
                (int)strlen(status_raw), status_raw);

        tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    } else {
        // Se for qualquer outra coisa (como carregar a página inicial), envia o HTML
        create_http_response();
        tcp_write(tpcb, resposta_http, strlen(resposta_http), TCP_WRITE_FLAG_COPY);
    }

    pbuf_free(p);
    return ERR_OK;
}


static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

// Função para monitorar cliques nos botões
void ligar_pinos_callback(uint gpio, uint32_t events){

    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time < 200) return;
    last_time = current_time;

    if(gpio == PINO_BOTAO_A){
        snprintf(mensagemB1, sizeof(mensagemB1), "RGB: %d, %d, %d", (duty_cycle_R*255)/4095, (duty_cycle_G*255)/4095, (duty_cycle_B*255)/4095);
        printf("Botão 1 pressionado\n");

    }else if(gpio == PINO_BOTAO_B){
        snprintf(mensagemB2, sizeof(mensagemB2), "RGB: %d, %d, %d", (duty_cycle_R*255)/4095, (duty_cycle_G*255)/4095, (duty_cycle_B*255)/4095);
        printf("Botão 2 pressionado\n");

    }else if(gpio == PINO_BOTAO_J){

    }
}


int main()
{

    // iniciando biblioteca adc
    adc_init();
    
    // iniciando biblioteca stdio
    stdio_init_all();

    // tempo para entrar no monitor serial
    sleep_ms(10000);

    //Iniciar servidor
    printf("Iniciando servidor HTTP\n");
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");


    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    printf("Wi-Fi conectado!\n");

    // Funções de configuração
    configurar_eixoXY();

    configurar_botoes();

    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &ligar_pinos_callback);

    gpio_set_irq_enabled(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true);

    gpio_set_irq_enabled(PINO_BOTAO_J, GPIO_IRQ_EDGE_FALL, true);
    
    // Definindo o slice do gpio para o PWM; slice do led azul = slice do led vermelho
    int slice_num_G = pwm_gpio_to_slice_num(PWM_PIN_G);
    int slice_num_B = pwm_gpio_to_slice_num(PWM_PIN_B);
    
    configurar_leds(slice_num_G, slice_num_B);

    pwm_set_enabled(slice_num_G, true);
    pwm_set_enabled(slice_num_B, true);

    start_http_server();

    while (true) {

        // Mantém wifi ativo
        cyw43_arch_poll();

        // leitura do adc
        adc_select_input(1); 
        uint16_t vrx_value = adc_read(); 

        adc_select_input(0); 
        uint16_t vry_value = adc_read();
        
        //Atribuição de valores
        duty_cycle_G = vrx_value;
        duty_cycle_B = vry_value;
        duty_cycle_R = 4095 - vrx_value;
        if (duty_cycle_B > 3700) duty_cycle_G = 0;
        if (duty_cycle_G < 500) duty_cycle_G = 0;
        if (duty_cycle_B < 500) duty_cycle_B = 0;
        if (duty_cycle_R < 500) duty_cycle_R = 0;

        pwm_set_gpio_level(PWM_PIN_G, (duty_cycle_G*WRAP/4095));
        pwm_set_gpio_level(PWM_PIN_B, (duty_cycle_B*WRAP/4095));
        pwm_set_gpio_level(PWM_PIN_R, (duty_cycle_R*WRAP/4095));

        sleep_ms(50);
    }

    cyw43_arch_deinit();
    return 0;
}