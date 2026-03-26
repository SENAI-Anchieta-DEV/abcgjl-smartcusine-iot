//----------------------------------------------------- IOT - SENSORES ---------------------------------------------------
/*
Autor: Amanda Marques, Bianca Neves, Catarina Macedo, Gabrielli Marcelino, Jamily Vittoria Alecrim e Laura Basilio.
Data: 26/03/2026
Versão: teste
*/

//INCLUI BIBLIOTECAS ---------------------------------------
#include <Arduino.h> //Inclui a biblioteca universal do Arduino
#include <ArduinoJson.h> //Inclui a biblioteca json
#include <WiFi.h> // Biblioteca padrão para conectar o ESP32 ao Wi-Fi
#include <PubSubClient.h> // Biblioteca para o protocolo MQTT

// Bibliotecas dos sensores (já prontas para quando você for soldar)
//#include <Wire.h> 
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
//#include <max6675.h> 
//----------------------------------------------------------

// --- CONFIGURAÇÕES DE REDE GLOBAIS ---
// ATENÇÃO: Preencha com os dados do Wi-Fi que o ESP32 vai usar!
const char* ssid = "Teste"; 
const char* password = "Teste123";     
const char* mqtt_server = "10.83.150.110";  // O IP do seu computador (Correto!)

WiFiClient espClient;
PubSubClient client(espClient);

// --- VARIÁVEIS DE CONTROLE ---
unsigned long tempoUltimaLeitura = 0;
const long intervaloLeitura = 5000; // Publica a cada 5 segundos
const float LIMITE_TEMP_FORNO = 180.0;
bool alarmeAtivo = false;

// --- FUNÇÕES AUXILIARES ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se a rede: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP recebido: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até conectar ao Broker MQTT
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT com o servidor ");
    Serial.print(mqtt_server);
    Serial.print("...");
    
    String clientId = "CozinhaESP32-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("\nCONECTADO AO BROKER MQTT COM SUCESSO!");
    } else {
      Serial.print("\nFalhou, erro codigo: ");
      Serial.print(client.state());
      Serial.println(" - Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void acionarBuzzer() {
  Serial.println("--> ALERTA: BUZZER LIGADO! (Simulacao)");
}

void desligarBuzzer() {
  Serial.println("--> ALERTA: BUZZER DESLIGADO! (Simulacao)");
}

// --- SETUP (Roda uma vez) ---
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

// --- LOOP (Roda continuamente) ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoUltimaLeitura >= intervaloLeitura) {
    tempoUltimaLeitura = tempoAtual;

    // --- LEITURA DOS SENSORES (SIMULADA / MOCK) ---
    // Gerando dados aleatórios porque os sensores ainda não estão na placa
    float tempEquipamento = random(1700, 1900) / 10.0; // Gera de 170.0 a 190.0 °C
    float tempAmbiente = random(250, 350) / 10.0;      // Gera de 25.0 a 35.0 °C
    float umidadeAmbiente = random(400, 600) / 10.0;   // Gera de 40.0 a 60.0 %
    float pressaoAmbiente = 1013.25;

    // Verificação de erro e regra de negócio do Alarme
    String statusForno = "OK";
    if (isnan(tempEquipamento)) {
      statusForno = "ERRO_SENSOR";
      tempEquipamento = 0.0; 
    } else if (tempEquipamento > LIMITE_TEMP_FORNO) {
      statusForno = "ALERTA_SUPERAQUECIMENTO";
      if (!alarmeAtivo) {
        alarmeAtivo = true;
        acionarBuzzer();
      }
    } else {
      if (alarmeAtivo) {
        alarmeAtivo = false;
        desligarBuzzer();
      }
    }

    // ==========================================
    // CONSTRUÇÃO DO PACOTE JSON
    // ==========================================
    JsonDocument doc;
    
    doc["forno_temp"] = tempEquipamento;
    doc["forno_status"] = statusForno;
    doc["ambiente_temp"] = tempAmbiente;
    doc["ambiente_umid"] = umidadeAmbiente;
    doc["ambiente_pressao"] = pressaoAmbiente;
    doc["uptime_segundos"] = millis() / 1000;

    String payloadJson;
    serializeJson(doc, payloadJson);

    // --- LOG NO SERIAL MONITOR ---
    Serial.println("\n====== DADOS ENVIADOS ======");
    Serial.println(payloadJson); 

    // --- PUBLICAÇÃO VIA MQTT ---
    client.publish("cozinha/telemetria", payloadJson.c_str());
  }
}