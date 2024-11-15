#include <WiFi.h>
#include "UbidotsEsp32Mqtt.h"

// Credenciais e configurações do Ubidots e WiFi
const char *UBIDOTS_TOKEN = "BBUS-fB2eR6n1JEeD8vkdU8i3NcKurxyRYl"; // Token para autenticação no Ubidots
const char *WIFI_SSID = ""; // Nome da rede WiFi
const char *WIFI_PASS = ""; // Senha da rede WiFi

// Rótulos para identificação no Ubidots
const char *DEVICE_LABEL = "esp32_t11_g"; // Identificador do dispositivo no Ubidots
const char *VARIABLE_LABEL_PEDESTRE = "pedestrianButton"; // Variável para indicar o botão de pedestre
const char *VARIABLE_LABEL_MUDANCA = "changeButton"; // Variável para indicar o botão de mudança
const char *VARIABLE_LABEL_PEDESTRE_ATIVO = "pedestrianActive2"; // Variável para o estado do pedestre ativo
const char *VARIABLE_LABEL_CICLO_SECUNDARIO = "cicloSecundario"; // Variável para o estado do ciclo secundário

// Pinos do primeiro semáforo
const uint8_t LED_VERMELHO_1 = 33;
const uint8_t LED_AMARELO_1 = 15;
const uint8_t LED_VERDE_1 = 32;

// Pinos do segundo semáforo
const uint8_t LED_VERMELHO_2 = 14;
const uint8_t LED_AMARELO_2 = 27;
const uint8_t LED_VERDE_2 = 26;

// Pino do botão de pedestre
const uint8_t BOTAO_PEDESTRE = 18;

// Estados possíveis do semáforo
enum Estados {
    ESTADO_VERDE,    // Estado em que o semáforo está verde
    ESTADO_VERMELHO, // Estado em que o semáforo está vermelho
    ESTADO_AMARELO,  // Estado em que o semáforo está amarelo
    ESTADO_PEDESTRE  // Estado em que o semáforo está vermelho para permitir passagem de pedestres
};

// Variáveis gerais de controle
int estado_botao_pedestre; // Armazena o estado do botão de pedestre
long tempo_decorrido;      // Controla o tempo decorrido
long tempo_anterior = 0;   // Marca o tempo da última atualização
long tempo_verde_inicial;  // Marca o início do estado verde
long intervalo = 0;        // Tempo para permanecer em cada estado
Estados estado_atual = ESTADO_VERDE; // Estado atual do semáforo

// Flags para controlar ciclos e estados
bool mudanca = false;                // Indica se houve solicitação de mudança
bool pedestreAtivo1 = false;         // Indica se o botão de pedestre foi pressionado
bool cicloPedestre = false;          // Indica se está em ciclo de pedestres
bool cicloSecundario = false;        // Indica se está em ciclo secundário
bool cicloMudanca = false;           // Indica se houve uma mudança recente
bool novoCiclo = true;               // Marca o início de um novo ciclo
bool pedestre_secundario_ativo;      // Indica se o pedestre está ativo no estado secundário
long TEMPO_MINIMO_VERDE = 3000;      // Tempo mínimo para manter o semáforo no estado verde

// Configuração da instância do Ubidots
Ubidots ubidots(UBIDOTS_TOKEN);

// Função de callback para processar mensagens MQTT recebidas
void callback(char *topic, byte *payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    // Atualiza o estado do pedestre ativo com base no tópico recebido
    if (String(topic) == VARIABLE_LABEL_PEDESTRE_ATIVO) {
        pedestre_secundario_ativo = (message == "1");
    }

    // Atualiza o estado de mudança com base no tópico recebido
    if (String(topic) == VARIABLE_LABEL_MUDANCA) {
        mudanca = (message == "1");
    }
}

void setup() {
    Serial.begin(115200);

    // Configuração do WiFi e conexão com o Ubidots
    ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
    ubidots.setCallback(callback);
    ubidots.setup();
    ubidots.reconnect();
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_PEDESTRE_ATIVO);
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_MUDANCA);

    // Configuração dos pinos dos LEDs dos semáforos
    pinMode(LED_VERMELHO_1, OUTPUT);
    pinMode(LED_AMARELO_1, OUTPUT);
    pinMode(LED_VERDE_1, OUTPUT);
    pinMode(LED_VERMELHO_2, OUTPUT);
    pinMode(LED_AMARELO_2, OUTPUT);
    pinMode(LED_VERDE_2, OUTPUT);

    // Configuração do pino do botão de pedestres
    pinMode(BOTAO_PEDESTRE, INPUT);
}

void loop() {
    // Reconecta ao Ubidots caso desconectado
    if (!ubidots.connected()) {
        ubidots.reconnect();
    }

    // Lê o estado do botão de pedestres
    estado_botao_pedestre = !digitalRead(BOTAO_PEDESTRE);

    // Ativa o ciclo de pedestres se o botão for pressionado
    if (estado_botao_pedestre) {
        pedestreAtivo1 = true;
    }

    // Verifica e gerencia transições de estado
    if (cicloMudanca && pedestre_secundario_ativo) {
        intervalo = 4000;
        estado_atual = ESTADO_AMARELO;
        cicloPedestre = true;
        cicloMudanca = false;
    }

    if (estado_atual == ESTADO_VERDE && (millis() - tempo_verde_inicial) >= TEMPO_MINIMO_VERDE) {
        if (pedestreAtivo1 || mudanca) {
            intervalo = 0;
            estado_atual = ESTADO_AMARELO;
            cicloPedestre = pedestreAtivo1;
            cicloSecundario = mudanca;
        }
    }

    // Controle dos tempos e troca de estados
    tempo_decorrido = millis();
    if (tempo_decorrido - tempo_anterior >= intervalo) {
        resetarLEDs();

        // Gerencia os estados dos semáforos
        switch (estado_atual) {
            case ESTADO_VERDE: estadoVerde(); break;
            case ESTADO_VERMELHO: estadoVermelhoSecundario(); break;
            case ESTADO_AMARELO: estadoAmarelo(); break;
            case ESTADO_PEDESTRE: estadoPedestre(); break;
        }

        tempo_anterior = tempo_decorrido;
    }

    // Publica os estados dos ciclos no Ubidots
    ubidots.add(VARIABLE_LABEL_PEDESTRE, cicloPedestre ? 1 : 0);
    ubidots.add(VARIABLE_LABEL_CICLO_SECUNDARIO, cicloSecundario ? 1 : 0);
    ubidots.publish(DEVICE_LABEL);

    // Mantém o loop MQTT ativo
    ubidots.loop();
}

// Função para apagar todos os LEDs
void resetarLEDs() {
    digitalWrite(LED_VERMELHO_1, LOW);
    digitalWrite(LED_AMARELO_1, LOW);
    digitalWrite(LED_VERDE_1, LOW);
    digitalWrite(LED_VERMELHO_2, LOW);
    digitalWrite(LED_AMARELO_2, LOW);
    digitalWrite(LED_VERDE_2, LOW);
}

// Função para gerenciar o estado verde
void estadoVerde() {
    if (novoCiclo) {
        tempo_verde_inicial = millis();
        novoCiclo = false;
    }
    digitalWrite(LED_VERDE_1, HIGH);
    digitalWrite(LED_VERDE_2, HIGH);
    intervalo = 4000; // Tempo padrão no estado verde
    cicloPedestre = false;
    cicloSecundario = false;
    cicloMudanca = false;
}

// Função para gerenciar o estado amarelo
void estadoAmarelo() {
    digitalWrite(LED_AMARELO_1, HIGH);
    digitalWrite(LED_AMARELO_2, HIGH);
    intervalo = 2000;

    estado_atual = cicloPedestre ? ESTADO_PEDESTRE : ESTADO_VERMELHO;
}

// Função para gerenciar o estado vermelho
void estadoVermelhoSecundario() {
    novoCiclo = true;
    cicloMudanca = true;
    digitalWrite(LED_VERMELHO_1, HIGH);
    digitalWrite(LED_VERMELHO_2, HIGH);
    intervalo = 6000;
    estado_atual = ESTADO_VERDE;
}

// Função para gerenciar o estado de pedestre
void estadoPedestre() {
    novoCiclo = true;
    digitalWrite(LED_VERMELHO_1, HIGH);
    digitalWrite(LED_VERMELHO_2, HIGH);
    intervalo = 3500;
    estado_atual = cicloSecundario ? ESTADO_VERMELHO : ESTADO_VERDE;
