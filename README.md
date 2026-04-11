# 🎨 Painel de Pintura Virtual - EmbarcaTech

Este projeto consiste em um **Painel de Pintura Virtual 5x5** interativo, desenvolvido para o microcontrolador **Raspberry Pi Pico W**. Através de um servidor HTTP hospedado na própria placa, o usuário pode selecionar células em uma grade via navegador e colori-las fisicamente utilizando o hardware da placa (Joystick e Botões).

---

## 🎯 Objetivo do Projeto

O objetivo principal é integrar de forma prática os conceitos fundamentais de sistemas embarcados explorados durante o programa EmbarcaTech:

- **ADC (Analog-to-Digital Converter):** Leitura dos eixos X e Y do Joystick para controle dinâmico de cores RGB.
- **PWM (Pulse Width Modulation):** Controle da intensidade luminosa dos LEDs RGB e geração de níveis de cores.
- **Interrupções (IRQ):** Tratamento eficiente dos botões para aplicação de cores, limpeza de células e reset do painel.
- **IoT (Internet of Things):** Implementação de um servidor Web funcional utilizando a pilha LwIP e o chip CYW43 para interface em tempo real via navegador.

---

## 🛠️ Dependências

Para compilar e rodar este projeto, você precisará de:

- SDK do Raspberry Pi Pico devidamente configurado no sistema.
- Biblioteca **LwIP** (incluída no SDK) para suporte à rede TCP/IP.
- Compilador **CMake** e **GCC Arm Embedded Toolchain**.
- Extensão **Raspberry Pi Pico** no VS Code (recomendado).

---

## ⚙️ Instruções de Instalação

1. Clone este repositório em sua máquina local.
2. Abra a pasta do projeto no VS Code.
3. **Configuração Crítica (IntelliSense):** Para que o VS Code reconheça as bibliotecas de rede corretamente, você deve adicionar o caminho do arquivo `lwipopts.h` ao seu arquivo `.vscode/c_cpp_properties.json`.

   No campo `includePath`, certifique-se de incluir o diretório onde o seu arquivo de configuração do LwIP reside. Exemplo:

```json
   "includePath": [
       "${workspaceFolder}/**",
       "${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include"
   ]
```

4. Certifique-se de que o arquivo `CMakeLists.txt` inclua a biblioteca `pico_cyw43_arch_lwip_threadsafe_background` (ou similar).

---

## 🚀 Execução

Antes de compilar, você precisa configurar as credenciais da sua rede local:

1. No arquivo principal do código, localize as definições:

```c
   #define WIFI_SSID "NOME_DA_SUA_REDE"
   #define WIFI_PASS "SENHA_DA_SUA_REDE"
```

2. Substitua `"REDE"` e `"SENHA"` pelo nome e senha do seu Wi-Fi.
3. Compile o projeto (**Build**).
4. Coloque o Raspberry Pi Pico W em modo **BOOTSEL** e arraste o arquivo `.uf2` gerado.
5. Abra o **Monitor Serial** para verificar o endereço IP atribuído à placa.
6. Acesse o endereço IP no seu navegador (ex: `http://192.168.1.15`).

---

## 🎮 Como Usar

**No Navegador:** Clique em um dos 25 quadrados da grade para selecioná-lo.

**No Hardware:**

| Controle | Ação |
|---|---|
| 🕹️ Joystick | Mistura as componentes Red, Green e Blue (visualizadas no LED RGB e no swatch do navegador) |
| 🔴 Botão A (GPIO 5) | Pinta a célula selecionada com a cor atual |
| 🔵 Botão B (GPIO 6) | Limpa a cor da célula selecionada |
| ⚫ Botão do Joystick (GPIO 22) | Reseta todo o painel |

---

## 👤 Autor

**João Rodrigues**
Projeto Final — Programa EmbarcaTech 2026
