#include <WiFi.h>
#include <PubSubClient.h>

// Configurações do WiFi
const char* SSID = "Inteli.Iot";
const char* SENHA = "@Intelix10T#";

// Configurações do MQTT
const char* ID_CLIENTE = "ESP32_Semaforo_002";
const char* SERVIDOR_MQTT = "broker.emqx.io";
const int PORTA_MQTT = 1883;

// Definição dos tópicos MQTT
const char* TOPICO_MODO_NOTURNO = "esp32/traffic/night_mode";
const char* TOPICO_MODO_PEDESTRE = "esp32/traffic/pedestrian_mode";
const char* TOPICO_BOTAO_PEDESTRE = "esp32/traffic/pedestrian_button";
const char* TOPICO_CICLO_SECUNDARIO = "esp32/traffic/secondary_cycle";

// Pinos usados no ESP32
#define PINO_LED_VERMELHO 16
#define PINO_LED_AMARELO 15
#define PINO_LED_VERDE 17
#define PINO_BOTAO_PEDESTRE 18
#define PINO_LDR_CARRO 32

// Estados do semáforo
enum EstadosSemaforo {
    ESTADO_VERDE,
    ESTADO_VERMELHO,
    ESTADO_AMARELO,
    ESTADO_MODO_NOTURNO
};

// Variáveis gerais de controle
EstadosSemaforo estado_atual = ESTADO_VERDE; // Começa no estado verde
long tempo_decorrido;
long tempo_anterior = 0;
long intervalo = 1000;
bool modo_noturno_ativo = false;
bool pedestre_ativo = false;
bool botao_pressionado = false; // Indica se o botão foi pressionado
bool modo_noturno_anterior = false;
bool carro_esperando = false;
bool ciclo_secundario = false;
bool proximo_ciclo = false;
bool estado_verde_ativo = true;

// Controle de tempo no estado verde
long inicio_estado_verde = 0; // Marca o início do estado verde
const long TEMPO_MINIMO_VERDE = 10000; // 10 segundos
const int LIMITE_CARRO = 200;

// Clientes WiFi e MQTT
WiFiClient cliente_wifi;
PubSubClient cliente_mqtt(cliente_wifi);

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

    // Desativa o estado verde ativo
    estado_verde_ativo = false;

    // Desliga todos os LEDs antes de mudar de estado
    resetarLeds(); 

    if (modo_noturno_ativo) {
      modoNoturno(); // Executa o comportamento do modo noturno
    } else {
      alternarSemaforo(); // Alterna entre os outros estados do semáforo
    }
  }
    
    // verifica se o estado verde está ativo
    if (estado_verde_ativo) {

        //verifica se passou o tempo minimo do verde
        if ((tempo_decorrido - inicio_estado_verde) >= TEMPO_MINIMO_VERDE) {

          //verfica se o botao foi pressionado ou se tem um carro esperando
          if (botao_pressionado || carro_esperando){

            // se o botão tenha sido pressionado
            if (botao_pressionado) {

                // informa que o modo pedestre está ativo
                pedestre_ativo = true;

                // atualiza o estado do botao pressionado
                botao_pressionado = false;

                // atualiza o estado do proximo ciclo de acordo se tem carro esperando ou não
                proximo_ciclo = carro_esperando;

            } else {
                // ativo o ciclo secundario
                ciclo_secundario = true;
            }

            // muda o estado atual para amarelo
            estado_atual = ESTADO_AMARELO;

            // define o intervalo como 0
            intervalo = 0;
        }
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
      cliente_mqtt.subscribe(TOPICO_BOTAO_PEDESTRE);
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
  digitalWrite(PINO_LED_VERDE, HIGH); // Inicia no estado verde
}

// Callback do MQTT
void callbackMQTT(char* topico, byte* payload, unsigned int comprimento) {
    String mensagem;
    for (int i = 0; i < comprimento; i++) {
        mensagem += (char)payload[i];
    }

    // Se receber mensagem do topico modo noturno
    if (String(topico) == TOPICO_MODO_NOTURNO) {
        modo_noturno_anterior = modo_noturno_ativo;

        // alterna o se o modo está ativo ou não
        modo_noturno_ativo = (mensagem == "1");
        intervalo = 0;
        
        if (modo_noturno_anterior && !modo_noturno_ativo) {
            estado_atual = ESTADO_VERDE;
        }
    }

    if (String(topico) == TOPICO_BOTAO_PEDESTRE && mensagem == "1") {
        if (estado_atual == ESTADO_VERDE) {
            botao_pressionado = true;
        }
    }

    // Imprime a mensagem recebida
    Serial.print("Mensagem recebida [");
    Serial.print(topico);
    Serial.print("]: ");
    Serial.println(mensagem);
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

// Liga o LED verde e ajusta o intervalo
void estadoVerde() {

    // deixa o estado verde como ativo
    estado_verde_ativo = true;

    // se for o inicio do estado verde marca o tempo
    if (inicio_estado_verde == 0) {
        inicio_estado_verde = millis();
    }

    digitalWrite(PINO_LED_VERDE, HIGH);
    intervalo = 1000;

    // atualiza as variaveis para falso
    pedestre_ativo = false;
    ciclo_secundario = false;
    proximo_ciclo = false;
}

// Liga o LED amarelo e ajusta o intervalo
void estadoAmarelo() {
  digitalWrite(PINO_LED_AMARELO, HIGH);
  intervalo = 2000;
  estado_atual = ESTADO_VERMELHO; // Próximo estado
}

// Liga o LED vermelho e ajusta o intervalo
void estadoVermelho() {
    digitalWrite(PINO_LED_VERMELHO, HIGH);

    // verfica se o pedestre está ativo
    if (pedestre_ativo) {
        // se o proximo ciclo for positivo fica um tempo maior
        intervalo = proximo_ciclo ? 11300 : 5000;

        // publica os tópicos do pedestre e do ciclp secundario
        cliente_mqtt.publish(TOPICO_MODO_PEDESTRE, "1");
        cliente_mqtt.publish(TOPICO_CICLO_SECUNDARIO, proximo_ciclo ? "1" : "0");
    } else if (ciclo_secundario) {
        // no caso do ciclo secundario fica com 6300 de intervalo
        intervalo = 6300;
        cliente_mqtt.publish(TOPICO_CICLO_SECUNDARIO, "1");
    }

    // define que o proximo estado é o verde
    estado_atual = ESTADO_VERDE;
    inicio_estado_verde = 0;
}

void modoNoturno() {
    static bool led_amarelo_ligado = false;
    if (led_amarelo_ligado) {
        digitalWrite(PINO_LED_AMARELO, HIGH);
    }
    led_amarelo_ligado = !led_amarelo_ligado; // Alterna o estado do LED
    intervalo = 500;

    inicio_estado_verde = 0;
    botao_pressionado = false;
}

// Verifica a luminosidade do LDR
void verificarLDR() {
    int leitura_ldr = analogRead(PINO_LDR_CARRO);

    // de acordo com a uminosidade, modifica a variavel carro esperando
    carro_esperando = leitura_ldr < LIMITE_CARRO;

    delay(200);
}

// Verifica se o botão foi pressionado
void verificarBotao() {
    // se estiver no estado verde ativo e o botao for pressionado
    if (estado_verde_ativo && digitalRead(PINO_BOTAO_PEDESTRE)) {
        // define a variavel como verdadeira
        botao_pressionado = true;
    }
}