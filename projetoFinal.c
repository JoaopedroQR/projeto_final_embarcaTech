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
#define WIFI_SSID "REDE"
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

#define NUM_CELLS 25

// Cor de cada célula (0xRRGGBB). -1 = sem cor atribuída
volatile int32_t cell_colors[NUM_CELLS];

// Índice da célula selecionada pelo browser (-1 = nenhuma)
volatile int selected_cell = -1;

// Flag: botão A foi pressionado → aplicar cor
volatile bool apply_color_flag = false;

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

static void build_state_json(char *buf, int buflen) {
    int pos = 0;
    pos += snprintf(buf + pos, buflen - pos,
        "{\"cells\":[");
    for (int i = 0; i < NUM_CELLS; i++) {
        int32_t c = cell_colors[i];
        int cr = -1, cg = -1, cb2 = -1;
        if (c >= 0) {
            cr  = (c >> 16) & 0xFF;
            cg  = (c >>  8) & 0xFF;
            cb2 =  c        & 0xFF;
        }
        if (c < 0)
            pos += snprintf(buf + pos, buflen - pos, "null");
        else
            pos += snprintf(buf + pos, buflen - pos, "{\"r\":%d,\"g\":%d,\"b\":%d}", cr, cg, cb2);
        if (i < NUM_CELLS - 1)
            pos += snprintf(buf + pos, buflen - pos, ",");
    }
    // Converte duty cycles (0–4095) para 0–255
    int cur_r = (duty_cycle_R * 255) / 4095;
    int cur_g = (duty_cycle_G * 255) / 4095;
    int cur_b = (duty_cycle_B * 255) / 4095;
    pos += snprintf(buf + pos, buflen - pos,
        "],\"sel\":%d,\"cur_r\":%d,\"cur_g\":%d,\"cur_b\":%d}",
        selected_cell, cur_r, cur_g, cur_b);
}


static const char *HTML_PAGE =
"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<title>Grade RGB</title>"
"<style>"
"body{font-family:sans-serif;display:flex;flex-direction:column;align-items:center;padding:20px;}"
"#grid{display:grid;grid-template-columns:repeat(5,64px);gap:8px;margin:20px 0;}"
".cell{width:64px;height:64px;border:2px solid #aaa;border-radius:6px;cursor:pointer;background:#eee;}"
".cell.selected{border:3px solid #1a73e8;outline:2px solid #8ab4f8;}"
"#swatch{width:48px;height:48px;border:1px solid #ccc;border-radius:6px;display:inline-block;vertical-align:middle;margin-right:12px;}"
"#info{margin-top:10px;color:#555;font-size:14px;}"
"</style>"
"</head><body>"
"<h2>Painel 5x5 para colorir</h2>"
"<div id='grid'></div>"
"<div style='display:flex;align-items:center;margin-bottom:8px;'>"
"  <span id='swatch'></span>"
"  <span id='rgb-val' style='font-size:18px;font-weight:bold;'></span>"
"</div>"
"<p id='info'>Clique num quadrado para selecioná-lo.</p>"
"<script>"
"let selectedIdx = -1;"

/* Cria as 25 células */
"(function buildGrid(){"
"  const g = document.getElementById('grid');"
"  for(let i=0;i<25;i++){"
"    const d=document.createElement('div');"
"    d.className='cell'; d.dataset.idx=i;"
"    d.addEventListener('click',()=>selectCell(i,d));"
"    g.appendChild(d);"
"  }"
"}());"

/* Seleciona célula → notifica Pico via GET /select?i=N */
"function selectCell(idx, el){"
"  document.querySelectorAll('.cell').forEach(c=>c.classList.remove('selected'));"
"  el.classList.add('selected');"
"  selectedIdx=idx;"
"  fetch('/select?i='+idx);"
"  document.getElementById('info').textContent='Quadrado '+(idx+1)+' selecionado. Pressione o Botão A para colorir.';"
"}"

/* Polling a cada 300 ms */
"function poll(){"
"  fetch('/state?t='+Date.now()).then(r=>r.json()).then(d=>{"
"    const cells=document.querySelectorAll('.cell');"
"    d.cells.forEach((c,i)=>{"
"      if(c) cells[i].style.background='rgb('+c.r+','+c.g+','+c.b+')';"
"      else  cells[i].style.background='';"
"    });"
"    const sw=document.getElementById('swatch');"
"    sw.style.background='rgb('+d.cur_r+','+d.cur_g+','+d.cur_b+')';"
"    document.getElementById('rgb-val').textContent="
"      ' R:'+d.cur_r+' G:'+d.cur_g+' B:'+d.cur_b;"
"    if(d.sel>=0 && d.sel!==selectedIdx){"
"      document.querySelectorAll('.cell').forEach(c=>c.classList.remove('selected'));"
"      document.querySelectorAll('.cell')[d.sel].classList.add('selected');"
"      selectedIdx=d.sel;"
"    }"
"  }).catch(()=>{});"
"}"
"setInterval(poll,300);"
"</script></body></html>\r\n";

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) { tcp_close(tpcb); return ERR_OK; }

    char *req = (char *)p->payload;

    // ── NOVO: GET /select?i=N → atualiza célula selecionada ─────────────────
    if (strncmp(req, "GET /select", 11) == 0) {
        char *qi = strstr(req, "i=");
        if (qi) {
            int idx = atoi(qi + 2);
            if (idx >= 0 && idx < NUM_CELLS)
                selected_cell = idx;
        }
        const char *ok = "HTTP/1.1 204 No Content\r\n\r\n";
        tcp_write(tpcb, ok, strlen(ok), TCP_WRITE_FLAG_COPY);

    // ── NOVO: GET /state → devolve JSON com estado da grade ─────────────────
    } else if (strncmp(req, "GET /state", 10) == 0) {
        char json[700];
        build_state_json(json, sizeof(json));

        char header[128];
        int jlen = strlen(json);
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n", jlen);
        tcp_write(tpcb, header, strlen(header), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, json,   jlen,           TCP_WRITE_FLAG_COPY);

    } else {
        // Página principal
        tcp_write(tpcb, HTML_PAGE, strlen(HTML_PAGE), TCP_WRITE_FLAG_COPY);
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

void ligar_pinos_callback(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_time < 200) return;
    last_time = now;

    if (gpio == PINO_BOTAO_A) {
        // ── NOVO: aplica a cor atual à célula selecionada ────────────────────
        if (selected_cell >= 0 && selected_cell < NUM_CELLS) {
            int r = (duty_cycle_R * 255) / 4095;
            int g = (duty_cycle_G * 255) / 4095;
            int b = (duty_cycle_B * 255) / 4095;
            cell_colors[selected_cell] = ((int32_t)r << 16) | ((int32_t)g << 8) | b;
            printf("Celula %d colorida → RGB(%d,%d,%d)\n", selected_cell, r, g, b);
        }
    }else if(gpio == PINO_BOTAO_B){
        if (selected_cell >= 0 && selected_cell < NUM_CELLS) {
            cell_colors[selected_cell] = -1;
            printf("Celula %d BRANCA\n");
        }
    }else if(gpio == PINO_BOTAO_J){
        for (int i = 0; i < NUM_CELLS; i++) cell_colors[i] = -1;
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

     for (int i = 0; i < NUM_CELLS; i++) cell_colors[i] = -1;

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