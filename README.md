# Documentação do Sistema de Semáforos Inteligentes

## Link do Google Drive com o vídeo de funcionamento e foto da montagem do dispositivo

[Vídeo e foto da montagem](https://drive.google.com/drive/folders/1VjuIugYh5RtSvTQ1AQyVscyxscSRVsIo?usp=sharing)

## Funcionalidades

1. **Detecção de Veículos e Prioridade no Fluxo:** O sensor LDR detecta a presença de veículos, ajustando o ciclo dos semáforos para otimizar o fluxo de tráfego.  
2. **Modo Noturno:** Quando a luminosidade ambiente é baixa, todos os semáforos entram automaticamente no modo amarelo piscante, garantindo economia de energia e segurança em horários de baixo tráfego.  
3. **Controle Manual para Pedestres:** Botões permitem a ativação de ciclos para pedestres, com ajuste dinâmico dos semáforos para segurança e fluidez.  
4. **Sincronização Dinâmica:** O sistema garante que os semáforos alternem entre estados com base em condições específicas, como a presença de veículos e acionamento de botões.  

## Lógica do Sistema

1. **Modo Noturno (Ativado pelo LDR no ESP1):**  
   - Quando a luminosidade ambiente é baixa, **todos os semáforos entram no modo amarelo piscante**.

2. **Ciclo Normal (Dia):**  
   - O **Semáforo 1 (ESP1)** inicia no **verde** por 10 segundos.  
   - Após os 10 segundos:
     - **Se houver um carro aguardando (LDR no ESP2 detecta leitura baixa):**  
       - Semáforo 1 muda para **amarelo** e depois para **vermelho**.  
       - O **Semáforo 2 (ESP2)** muda para **verde**, completa seu ciclo (verde -> amarelo -> vermelho).  
       - Após o ciclo do Semáforo 2, ambos os semáforos ficam no **vermelho** por um intervalo, e o Semáforo 1 retorna ao **verde**.  

     - **Se não houver carro aguardando, mas o botão foi pressionado:**  
       - O Semáforo 1 muda para **amarelo**, depois para **vermelho**.  
       - Após um intervalo de segurança, o Semáforo 1 retorna ao **verde**.  

     - **Se houver carro aguardando e o botão foi pressionado:**  
       - O Semáforo 1 muda para **amarelo** e depois para **vermelho**.  
       - O Semáforo 2 completa seu ciclo (verde -> amarelo -> vermelho).  
       - Ambos os semáforos ficam no **vermelho** por um intervalo antes de o Semáforo 1 retornar ao **verde**.

## Especificações do Sistema de Semáforos

| ESP32 | Semáforo | LED Verde (pino) | LED Amarelo (pino) | LED Vermelho (pino) | Fotoresistor (pino) | Botão (pino) |  
|-------|----------|------------------|---------------------|----------------------|----------------------|--------------|  
| ESP 1 | Semáforo 1 | Pino 12         | Pino 14            | Pino 27             | Pino 32             | Pino 19      |  
| ESP 2 | Semáforo 2 | Pino 17         | Pino 15            | Pino 16             | Pino 32             | Pino 18      |  

## Componentes Utilizados

| Componente         | Quantidade | Função                                                                 |  
|--------------------|------------|------------------------------------------------------------------------|  
| ESP32              | 2          | Controlador de cada semáforo e comunicação entre os dispositivos       |  
| LED (verde, amarelo, vermelho) | 6 (3 de cada) | Indicação visual dos sinais de cada semáforo                           |  
| Sensor de Luminosidade (LDR)   | 2          | Detecta presença de veículos e luminosidade ambiente para modo noturno |  
| Resistores         | 10         | Limitação de corrente para LEDs e LDRs                                  |  
| Botões             | 2          | Controle manual para ativar o ciclo de pedestres                      |  

## Montagem e Relato de Construção

1. **Conexão dos LEDs e LDRs:**  
   - Os LEDs de cada semáforo foram conectados aos pinos GPIO dos ESP32 conforme a tabela.  
   - Um resistor foi adicionado a cada LED e sensor para evitar sobrecarga.

2. **Configuração dos Sensores de Luminosidade (LDR):**  
   - O LDR no ESP1 foi programado para detectar condições noturnas e acionar o modo amarelo piscante.  
   - O LDR no ESP2 detecta a presença de veículos para ajustar o ciclo dos semáforos.  

3. **Conexão dos Botões:**  
   - Cada ESP32 possui um botão dedicado para controle manual de pedestres, ativando ciclos específicos nos semáforos.

4. **Sincronização dos ESP32:**  
   - Foi implementada comunicação entre os ESP32 para garantir transições sincronizadas e evitar conflitos nos ciclos.

5. **Testes e Validação:**  
   - O sistema foi testado em diversas condições de luminosidade, presença de veículos simulada e acionamento manual, garantindo que todos os estados funcionassem conforme esperado.

## Código do ESP1

```cpp
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
```

## Código do ESP2

```cpp
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
```

## Conclusão

O sistema de semáforos proposto oferece uma solução eficiente, segura e flexível para gestão de tráfego urbano em cidades inteligentes. Com ajuste automático baseado em condições externas e interação manual, o projeto promove segurança para pedestres e fluidez no tráfego, adaptando-se a diferentes cenários urbanos.
