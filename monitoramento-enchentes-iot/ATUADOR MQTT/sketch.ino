#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* broker = "broker.hivemq.com";
const int brokerPort = 1883;

const char* topic_status = "mackenzie/enchente";                 // Recebe status do sensor
const char* topic_telemetry = "mackenzie/atuador/telemetria";    // Envia telemetria
const char* topic_eventos = "mackenzie/atuador/eventos";         // Envia contadores e eventos
const char* topic_estado = "mackenzie/atuador/estado";           // Estado atual do atuador

// Pinos
#define LED_PIN 2
#define BUZZ_PIN 4

WiFiClient espClient;
PubSubClient client(espClient);

// Métricas
unsigned long mensagens_recebidas_total = 0;
unsigned long contador_enchente = 0;
unsigned long contador_atencao = 0;
unsigned long contador_seguro = 0;

unsigned long tempo_ultimo_evento = 0;
unsigned long uptime_inicio = 0;

unsigned long tempo_seguro_total = 0;
unsigned long tempo_atencao_total = 0;
unsigned long tempo_enchente_total = 0;

String estado_atual = "SEGURO";

// Conexão WiFi
void setup_wifi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.println(WiFi.localIP());
}

// Publicação simplificada
void publicar(const char* topico, String msg) {
  client.publish(topico, msg.c_str());
}

// Atualiza telemetria a cada evento
void atualizar_tempos(String novo_estado) {
  unsigned long agora = millis();

  if (estado_atual == "SEGURO") tempo_seguro_total += (agora - tempo_ultimo_evento);
  if (estado_atual == "ATENCAO") tempo_atencao_total += (agora - tempo_ultimo_evento);
  if (estado_atual == "ENCHENTE") tempo_enchente_total += (agora - tempo_ultimo_evento);

  tempo_ultimo_evento = agora;
  estado_atual = novo_estado;
}

// Callback MQTT
void callback(char* topic, byte* message, unsigned int length) {
  mensagens_recebidas_total++;

  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)message[i];

  Serial.print("Status recebido: ");
  Serial.println(msg);

  // Extrair o status do JSON
  String status = "";
  int key = msg.indexOf("\"status\"");
  int asp1 = msg.indexOf('"', key + 9);
  int asp2 = msg.indexOf('"', asp1 + 1);
  status = msg.substring(asp1 + 1, asp2);

  atualizar_tempos(status);

  // Atualiza contadores e atuadores
  if (status == "ENCHENTE") {
    contador_enchente++;
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZ_PIN, HIGH);
  } 
  else if (status == "ATENCAO") {
    contador_atencao++;
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZ_PIN, LOW);
  } 
  else {
    contador_seguro++;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZ_PIN, LOW);
  }

  // Publica eventos
  publicar(topic_eventos,
           "{\"enchente\":" + String(contador_enchente) +
           ",\"atencao\":" + String(contador_atencao) +
           ",\"seguro\":" + String(contador_seguro) + "}");

  publicar(topic_estado, "{\"estado\":\"" + estado_atual + "\"}");
}

// Reconexão
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_ATUADOR")) {
      client.subscribe(topic_status);
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);

  setup_wifi();

  client.setServer(broker, brokerPort);
  client.setCallback(callback);

  uptime_inicio = millis();
  tempo_ultimo_evento = uptime_inicio;
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Publica telemetria a cada 5s
  static unsigned long lastTelemetry = 0;
  if (millis() - lastTelemetry >= 5000) {
    lastTelemetry = millis();

    publicar(topic_telemetry,
      "{\"mensagens\":" + String(mensagens_recebidas_total) +
      ",\"uptime\":" + String((millis() - uptime_inicio) / 1000) +
      ",\"seguro_tempo\":" + String(tempo_seguro_total / 1000) +
      ",\"atencao_tempo\":" + String(tempo_atencao_total / 1000) +
      ",\"enchente_tempo\":" + String(tempo_enchente_total / 1000) +
      "}"
    );
  }
}
