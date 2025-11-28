// Incluir bibliotecas
#include <WiFi.h> // Biblioteca para conexão WiFi
#include <PubSubClient.h> // Biblioteca para comunicação MQTT

// Definições de Wi-Fi
char SSIDName[] = "Wokwi-GUEST"; // Nome da rede Wi-Fi
char SSIDPass[] = ""; // Senha da rede Wi-Fi (vazia)

// Configurações do broker MQTT
char BrokerURL[] = "broker.hivemq.com"; // Endereço do broker MQTT
int BrokerPort = 1883; // Porta padrão
char MQTTClientName[] = "ESP32-Enchente"; // Nome do cliente
char mqtt_topic[] = "mackenzie/enchente"; // Tópico de publicação

// Instâncias
WiFiClient espClient; // Cliente TCP/IP
PubSubClient clienteMQTT(espClient); // Cliente MQTT

// Pinos do sensor e atuadores
#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_PIN 23
#define BUZZER_PIN 19

long duration;
float distance;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  Serial.println();
  Serial.print("Conectando-se ao Wi-Fi");

  WiFi.begin(SSIDName, SSIDPass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Função para reconectar ao MQTT
void mqttReconnect() {
  while (!clienteMQTT.connected()) {
    Serial.println("Conectando-se ao broker MQTT...");
    Serial.println(MQTTClientName);

    if (clienteMQTT.connect(MQTTClientName)) {
      Serial.print(MQTTClientName);
      Serial.println(" conectado!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clienteMQTT.state());
      Serial.println(" tente novamente em 5 segundos.");
      delay(5000);
    }
  }
}

// Setup
void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  setup_wifi();
  clienteMQTT.setServer(BrokerURL, BrokerPort);
}

// Loop principal
void loop() {
  if (!clienteMQTT.connected()) {
    mqttReconnect();
  }
  clienteMQTT.loop();

  // Leitura do ultrassônico
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  // Regras de negócio
  String status;
  if (distance > 20) {
    status = "SEGURO";
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

  } else if (distance > 10 && distance <= 20) {
    status = "ATENCAO";
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

  } else {
    status = "ENCHENTE";
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }

  // JSON enviado ao broker
  String payload = "{\"distancia\":" + String(distance) +
                   ",\"status\":\"" + status + "\"}";

  clienteMQTT.publish(mqtt_topic, payload.c_str());

  Serial.print("Enviado: ");
  Serial.println(payload);

  delay(2000);
}
