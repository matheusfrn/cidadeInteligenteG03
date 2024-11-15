#include <WiFi.h>
#include "UbidotsEsp32Mqtt.h"

// Configuração dos pinos dos LEDs do semáforo
const uint8_t LED_VERMELHO = 27; // LED vermelho
const uint8_t LED_AMARELO = 14;  // LED amarelo
const uint8_t LED_VERDE = 12;    // LED verde

// Credenciais para Wi-Fi e Ubidots
const char *UBIDOTS_TOKEN = "BBUS-fB2eR6n1JEeD8vkdU8i3NcKurxyRYl"; // Token de autenticação do Ubidots
const char *WIFI_SSID = "";  // Nome da rede Wi-Fi
const char *WIFI_PASS = "";  // Senha da rede Wi-Fi

// Configuração de labels para os tópicos do Ubidots
const char *DEVICE_LABEL = "esp32_t11_g";                  // Identificador do dispositivo no Ubidots
const char *VARIABLE_LABEL_PEDESTRE = "pedestrianButton";  // Tópico para o botão de pedestre
const char *VARIABLE_LABEL_MUDANCA = "changeButton";       // Tópico para o botão de mudança
const char *VARIABLE_LABEL_PEDESTRE_ATIVO = "pedestrianActive2"; // Tópico para sinalizar pedestres ativos
const char *VARIABLE_LABEL_CICLO_SECUNDARIO = "cicloSecundario"; // Tópico para ciclo secundário

// Configuração do botão físico
const uint8_t BOTAO_MUDANCA = 19; // Botão físico para solicitar mudança no semáforo

// Enum para os estados do semáforo
enum Estados {
    ESTADO_VERMELHO, // Semáforo no estado vermelho
    ESTADO_AMARELO,  // Semáforo no estado amarelo
    ESTADO_VERDE,    // Semáforo no estado verde
    ESTADO_PEDESTRE  // Semáforo no estado de pedestre (todos os LEDs vermelhos)
};

// Variáveis gerais de controle
int estado_botao_mudanca; // Armazena o estado do botão físico de mudança
Estados estado_atual = ESTADO_VERMELHO; // Estado inicial do semáforo
long tempo_decorrido;       // Marca o tempo atual do ciclo
long tempo_anterior = 0;    // Armazena o tempo do último ciclo
long intervalo = 0;         // Duração do estado atual do semáforo
bool pedestre_ativo = false; // Indica se o estado de pedestre foi ativado

// Inicialização do cliente Ubidots
Ubidots ubidots(UBIDOTS_TOKEN);

// Função de callback para processar mensagens recebidas do Ubidots
void callback(char *topic, byte *payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    // Verifica se o tópico é relacionado ao botão de pedestre e ativa o estado de pedestre
    if (String(topic) == VARIABLE_LABEL_PEDESTRE && message == "1" && estado_atual == ESTADO_AMARELO) { 
        pedestre_ativo = true; // Ativa o estado de pedestre
    }

    // Verifica o tópico para ciclo secundário e muda para o estado verde, se estiver no vermelho
    if (String(topic) == VARIABLE_LABEL_CICLO_SECUNDARIO && message == "1" && estado_atual == ESTADO_VERMELHO) { 
        estado_atual = ESTADO_VERDE;
        intervalo = 0; // Reinicia o temporizador do estado
    }
}

void setup
