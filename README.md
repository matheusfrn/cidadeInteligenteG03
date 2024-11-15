# cidadeInteligenteG03

Link para a dashboard do projeto: https://inteli-ubidots.iot-application.com/app/dashboards/67290275b869fe8e5d59d14e

## Funcionalidades

1. **Detecção de Veículos:** O sensor de luminosidade detecta quando um veículo passa, alterando o estado dos semáforos para facilitar o fluxo.  
2. **Modo Noturno:** Quando a luminosidade ambiente reduz, um dos semáforos entra em modo amarelo piscante.  
3. **Controle Manual para Pedestres:** Botões permitem que todos os semáforos fiquem vermelhos, garantindo segurança para pedestres.  
4. **Monitoramento e Ajustes via Dashboard Ubidots:** Interface de monitoramento e controle remoto para visualizar dados e ajustar o sistema.

## Especificações do Sistema de Semáforos

| ESP32 | Semáforo | LED Verde (pino) | LED Amarelo (pino) | LED Vermelho (pino) | Fotoresistor (pino) | Botão (pino) |  
|-------|----------|------------------|---------------------|----------------------|----------------------|--------------|  
| ESP 1 | Semáforo 1 | Pino 32         | Pino 15            | Pino 33             | Pino 13             | N/A          |  
| ESP 1 | Semáforo 2 | Pino 26         | Pino 17            | Pino 14             | N/A                 | N/A          |  
| ESP 2 | Semáforo 3 | Pino 12         | Pino 14            | Pino 27             | Pino 26             | Pino 32      |

## Componentes Utilizados

| Componente         | Quantidade | Função                                                                 |  
|--------------------|------------|------------------------------------------------------------------------|  
| ESP32              | 2          | Controlador de cada semáforo e comunicação com o dashboard             |  
| LED (verde, amarelo, vermelho) | 9 (3 de cada) | Indicação visual dos sinais de cada semáforo                           |  
| Sensor de Luminosidade (LDR)   | 2          | Detecta presença de veículos e luminosidade ambiente para modo noturno |  
| Resistores         | 13         | Limitação de corrente para LEDs e LDRs                                  |  
| Botões             | 2          | Controle manual para ativar o modo de pedestres                        |

## Montagem e Relato de Construção

1. **Conexão dos LEDs e LDRs:** Os LEDs de cada semáforo foram conectados aos pinos GPIO de cada ESP32 conforme a tabela. Um resistor foi adicionado a cada LED e sensor para evitar sobrecarga.

2. **Configuração dos Sensores de Luminosidade (LDR):** Um sensor LDR foi configurado para detectar condições noturnas e acionar o modo amarelo piscante. O outro sensor foi programado para detectar a presença de um veículo, ajustando a transição de cores entre os semáforos.

3. **Conexão dos Botões:** Cada ESP32 possui um botão dedicado, que, ao ser pressionado, muda o status de todos os semáforos para vermelho, permitindo a travessia segura dos pedestres.

4. **Integração com o Dashboard Ubidots:** Os dados coletados pelo sistema, como a presença de veículos e a ativação dos modos, são enviados ao dashboard para controle remoto e monitoramento.

5. **Testes:** O sistema foi testado em diversas condições de luminosidade e presença simulada de veículos para garantir que os semáforos mudassem de acordo com o esperado e que o modo noturno fosse ativado quando necessário.

## Conclusão

Este projeto oferece uma solução escalável e ajustável para gestão de tráfego em cidades inteligentes, aumentando a segurança e eficiência do trânsito em condições urbanas complexas. Com controle remoto e ajuste automático, ele se adapta a diferentes cenários de luminosidade e tráfego, possibilitando uma experiência mais segura para motoristas e pedestres.

