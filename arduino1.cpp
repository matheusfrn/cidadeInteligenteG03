#include <WiFi.h>
#include <PubSubClient.h>

// Configurações do WiFi
const char* SSID = "Inteli.Iot";
const char* SENHA = "@Intelix10T#";

// Configurações do MQTT
const char* ID_CLIENTE = "ESP32_Semaforo_001";
const char* SERVIDOR_MQTT = "broker.emqx.io";
const int PORTA_MQTT = 1883;

// Definição dos tópicos MQTT
const char* TOPICO_MODO_PEDESTRE = "esp32/traffic/pedestrian_mode";
const char* TOPICO_CICLO_SECUNDARIO = "esp32/traffic/secondary_cycle";
const char* TOPICO_MODO_NOTURNO = "esp32/traffic/night_mode";
const char* TOPICO_BOTAO_PEDESTRE = "esp32/traffic/pedestrian_button";

// Pinos usados no ESP32
#define PINO_LED_VERMELHO 27
#define PINO_LED_AMARELO 14
#define PINO_LED_VERDE 12
#define PINO_BOTAO_PEDESTRE 19
#define PINO_LDR_NOITE 32

// Estados do semáforo
enum EstadosSemaforo {
    ESTADO_VERMELHO,
    ESTADO_AMARELO,
    ESTADO_VERDE,
    ESTADO_MODO_NOTURNO
};

// Configuração do wifi e do MQTT
WiFiClient cliente_wifi;
PubSubClient cliente_mqtt(cliente_wifi);

EstadosSemaforo estado_atual = ESTADO_VERMELHO; // Estado inicial
long tempo_decorrido;
long intervalo;
long tempo_anterior = 0;
bool modo_noturno_ativo = false; // Indica se o modo noturno está ativo
bool botao_pressionado = false; // Estado atual do botão
const int LIMITE_MODO_NOTURNO = 500; // Limite de luminosidade para ativar o modo noturno

void setup() {

  // Inicialização do monitor serial
  Serial.begin(115200);
  Serial.println("Iniciando o sistema de semáforo");

  // Conecta ao WiFi
  conectarWiFi();

  // Conecta ao broker MQTT
  conectarMQTT(); 

  // Configura os pinos e LEDs
  inicializarComponentes(); 
}

void loop() {

  // Se o MQTT não estiver conectado, chama a função para reconectar
  if (!cliente_mqtt.connected()) {
    conectarMQTT();
  }

  // Mantém a conexão MQTT ativa
  cliente_mqtt.loop(); 

  // Monitora o estado do botão e do LDR
  verificarBotao(); 
  verificarLDR();
  
  // Conta o tempo decorrido
  tempo_decorrido = millis();

  // Atualiza os estados do semáforo ao passar o tempo do intervalo
  if (tempo_decorrido - tempo_anterior >= intervalo) {

    // Atualiza o tempo anterior
    tempo_anterior = tempo_decorrido;

    // Desliga todos os LEDs antes de mudar de estado
    resetarLeds(); 

    if (modo_noturno_ativo) {
      modoNoturno(); // Executa o comportamento do modo noturno
    } else {
      alternarSemaforo(); // Alterna entre os outros estados do semáforo
    }
  }
}

// Função para conectar ao WiFi
void conectarWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(SSID, SENHA);

  while (WiFi.status() != WL_CONNECTED) { // Espera até a conexão ser estabelecida
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// Callback para processar mensagens MQTT recebidas
void callbackMQTT(char* topico, byte* payload, unsigned int comprimento) {
  String mensagem;
  for (int i = 0; i < comprimento; i++) {
    mensagem += (char)payload[i];
  }

  // Imprime a mensagem recebida
  Serial.print("Mensagem recebida [");
  Serial.print(topico);
  Serial.print("]: ");
  Serial.println(mensagem);

  // Se receber mensagem do topico modo noturno
  if (String(topico) == TOPICO_MODO_NOTURNO) {

    // alterna o se o modo está ativo ou não
    modo_noturno_ativo = (mensagem == "1");

    // Se o modo está ativo, muda para o estado de modo noturno
    if (modo_noturno_ativo) {
      intervalo = 0;
      estado_atual = ESTADO_MODO_NOTURNO;
    }
  }

  // Se receber a mensagem do tópico ciclo secundario igual a 1
  if (String(topico) == TOPICO_CICLO_SECUNDARIO && mensagem == "1") {
    estado_atual = ESTADO_VERDE; // Ativa o ciclo secundário
    intervalo = 0;
  }
}

// Função para conectar ao broker MQTT
void conectarMQTT() {
  cliente_mqtt.setServer(SERVIDOR_MQTT, PORTA_MQTT);
  cliente_mqtt.setCallback(callbackMQTT);

  while (!cliente_mqtt.connected()) {
    Serial.println("\nTentando conectar ao MQTT...");
    if (cliente_mqtt.connect(ID_CLIENTE)) {
      Serial.println("Conectado ao MQTT!");

      // Inscreve-se nos tópicos necessários
      cliente_mqtt.subscribe(TOPICO_MODO_NOTURNO);
      cliente_mqtt.subscribe(TOPICO_CICLO_SECUNDARIO);
    } else {
      Serial.print("Falha na conexão, estado: ");
      Serial.println(cliente_mqtt.state());
      delay(2000); // Aguarda antes de tentar novamente
    }
  }
}

// Configura os LEDs e botão
void inicializarComponentes() {
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_AMARELO, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_BOTAO_PEDESTRE, INPUT_PULLUP);
  resetarLeds();
  digitalWrite(PINO_LED_VERMELHO, HIGH); // Inicia no estado vermelho
}

// Desliga todos os LEDs
void resetarLeds() {
  digitalWrite(PINO_LED_VERMELHO, LOW);
  digitalWrite(PINO_LED_AMARELO, LOW);
  digitalWrite(PINO_LED_VERDE, LOW);
}

// Alterna entre os estados do semáforo
void alternarSemaforo() {
  switch (estado_atual) {
    case ESTADO_VERMELHO:
      estadoVermelho();
      break;
    case ESTADO_AMARELO:
      estadoAmarelo();
      break;
    case ESTADO_VERDE:
      estadoVerde();
      break;
  }
}

// Liga o LED vermelho e ajusta o intervalo
void estadoVermelho() {
  digitalWrite(PINO_LED_VERMELHO, HIGH);
  intervalo = 1000;
}

// Liga o LED amarelo e ajusta o intervalo
void estadoAmarelo() {
  digitalWrite(PINO_LED_AMARELO, HIGH);
  intervalo = 2000;
  estado_atual = ESTADO_VERMELHO; // Próximo estado
}

// Liga o LED verde e ajusta o intervalo
void estadoVerde() {
  digitalWrite(PINO_LED_VERDE, HIGH);
  intervalo = 4000;
  estado_atual = ESTADO_AMARELO; // Próximo estado
}

// Alterna o LED amarelo para o modo noturno
void modoNoturno() {
  static bool led_amarelo_ligado = false;
  if (led_amarelo_ligado) {
    digitalWrite(PINO_LED_AMARELO, HIGH);
  }
  led_amarelo_ligado = !led_amarelo_ligado; // Alterna o estado do LED
  intervalo = 500;
}

// Verifica se o botão foi pressionado
void verificarBotao() {
  bool estado_atual_botao = !digitalRead(PINO_BOTAO_PEDESTRE); // LOW significa pressionado
  if (estado_atual_botao != botao_pressionado) {
    botao_pressionado = estado_atual_botao;
    cliente_mqtt.publish(TOPICO_BOTAO_PEDESTRE, botao_pressionado ? "0" : "1"); // Envia o estado do botão
  }
}

// Verifica a luminosidade do LDR para ativar o modo noturno
void verificarLDR() {
  int leitura_ldr = analogRead(PINO_LDR_NOITE); // Lê o valor do sensor de luz
  bool modo_noturno = leitura_ldr < LIMITE_MODO_NOTURNO; // Define se está escuro

  if (modo_noturno != modo_noturno_ativo) {
    intervalo = 0; // Reinicia o intervalo
    modo_noturno_ativo = modo_noturno;
    cliente_mqtt.publish(TOPICO_MODO_NOTURNO, modo_noturno_ativo ? "1" : "0"); // Publica o estado do modo noturno
  }

  if (!modo_noturno && estado_atual == ESTADO_MODO_NOTURNO) {
    estado_atual = ESTADO_VERMELHO; // Retorna ao estado normal
  }

  delay(200);
}