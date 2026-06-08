//----------------------------------------------------- IOT - SENSORES ---------------------------------------------------
/*
Autor: Amanda Marques, Bianca Neves, Catarina Macedo, Gabrielli Marcelino, Jamily Vittoria Alecrim e Laura Basilio.
Data: 26/03/2026
Versão: teste
*/

// ================== BIBLIOTECAS ==================
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <max6675.h>

// ================== PINOS ==================
#define sck 18   // MAX6675 SCK
#define sc 5     // MAX6675 CS
#define so 19    // MAX6675 SO
#define buzzer 23

// ================== SENSORES ==================
MAX6675 termopar(sck, sc, so);
Adafruit_BME280 bme;

// ================== WIFI / MQTT ==================
const char* ssid = "Teste";
const char* password = "Teste123";
const char* mqtt_server = "10.137.107.36";

WiFiClient espClient;
PubSubClient client(espClient);

// ================== CONTROLE ==================
unsigned long tempoUltimaLeitura = 0;
const long intervaloLeitura = 5000;

const float LIMITE_TEMP_FORNO = 180.0;
bool alarmeAtivo = false;

// ================== WIFI ==================
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

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ================== MQTT ==================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    String clientId = "ESP32-Cozinha-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" erro: ");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s...");
      delay(5000);
    }
  }
}

// ================== BUZZER ==================
void acionarBuzzer() {
  digitalWrite(buzzer, HIGH);
  Serial.println(">>> BUZZER LIGADO!");
}

void desligarBuzzer() {
  digitalWrite(buzzer, LOW);
  Serial.println(">>> BUZZER DESLIGADO!");
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Inicializa BME280
  if (!bme.begin(0x76)) {
    Serial.println("Erro ao iniciar BME280!");
  } else {
    Serial.println("BME280 OK!");
  }
}

// ================== LOOP ==================
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoUltimaLeitura >= intervaloLeitura) {

    tempoUltimaLeitura = tempoAtual;

    // ===== LEITURA DOS SENSORES =====
    float tempEquipamento = termopar.readCelsius();

    float tempAmbiente = bme.readTemperature();
    float umidadeAmbiente = bme.readHumidity();
    float pressaoAmbiente = bme.readPressure() / 100.0; // hPa

    // ===== TRATAMENTO DE ERROS =====
    if (isnan(tempEquipamento)) {
      Serial.println("Erro no termopar!");
      tempEquipamento = 0.0;
    }

    if (isnan(tempAmbiente) || isnan(umidadeAmbiente)) {
      Serial.println("Erro no BME280!");
    }

    // ===== REGRA DE NEGÓCIO =====
    String statusForno = "OK";

    if (tempEquipamento > LIMITE_TEMP_FORNO) {
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

    // ===== JSON =====
    JsonDocument doc;

    doc["forno_temp"] = tempEquipamento;
    doc["forno_status"] = statusForno;
    doc["ambiente_temp"] = tempAmbiente;
    doc["ambiente_umid"] = umidadeAmbiente;
    doc["ambiente_pressao"] = pressaoAmbiente;
    doc["uptime_segundos"] = millis() / 1000;

    String payloadJson;
    serializeJson(doc, payloadJson);

    // ===== LOG =====
    Serial.println("\n====== DADOS ======");
    Serial.println(payloadJson);

    // ===== MQTT =====
    client.publish("cozinha/telemetria", payloadJson.c_str());
  }
}