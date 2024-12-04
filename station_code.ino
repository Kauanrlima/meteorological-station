#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Configuração Wi-Fi
const char* ssid = "";
const char* password = "";

// Configuração ThingSpeak
const char* server = "http://api.thingspeak.com";
String apiKey = "1GY973GZ1Z4CGU04"; // Substitua pela sua Write API Key

// Sensores
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085); // BMP180
Adafruit_AHTX0 aht; // AHT10

void setup() {
  Serial.begin(115200);

  // Conexão Wi-Fi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");

  // Inicializa o barramento I2C (opcional para D21 e D22)
  Wire.begin(); // SDA=D21, SCL=D22 por padrão no ESP32

  // Inicializa o sensor BMP180
  if (!bmp.begin()) {
    Serial.println("Erro ao inicializar o sensor BMP180!");
    while (1);
  }

  // Inicializa o sensor AHT10
  if (!aht.begin()) {
    Serial.println("Erro ao inicializar o sensor AHT10!");
    while (1);
  }
}

void loop() {
  // Lê os dados do BMP180
  sensors_event_t event;
  bmp.getEvent(&event);
  float temperaturaBMP;
  bmp.getTemperature(&temperaturaBMP);

  float altitude = 0.0;
  if (event.pressure) {
    altitude = bmp.pressureToAltitude(1013.25, event.pressure); // 1013.25 hPa = pressão ao nível do mar padrão
  } else {
    Serial.println("Erro ao ler pressão do BMP180!");
  }

  // Lê os dados do AHT10
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float temperaturaAHT = temp.temperature;
  float umidade = humidity.relative_humidity;

  // Calcula o índice de calor (Heat Index) em °C
  float hif_C = -8.78469475556 + 
                1.61139411 * temperaturaAHT + 
                2.33854883889 * umidade + 
               -0.14611605 * temperaturaAHT * umidade + 
               -0.012308094 * pow(temperaturaAHT, 2) + 
               -0.0164248277778 * pow(umidade, 2) + 
                0.002211732 * pow(temperaturaAHT, 2) * umidade + 
                0.00072546 * temperaturaAHT * pow(umidade, 2) + 
               -0.000003582 * pow(temperaturaAHT, 2) * pow(umidade, 2);

  // Mostra os dados no Serial Monitor
  Serial.print("Temperatura (BMP180): ");
  Serial.print(temperaturaBMP);
  Serial.println(" °C");
  Serial.print("Temperatura (AHT10): ");
  Serial.print(temperaturaAHT);
  Serial.println(" °C");
  Serial.print("Pressão: ");
  Serial.print(event.pressure);
  Serial.println(" hPa");
  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");
  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");
  Serial.print("Índice de Calor: ");
  Serial.print(hif_C);
  Serial.println(" °C");

  // Envia os dados para o ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Monta a URL da requisição
    String url = String(server) + "/update?api_key=" + apiKey +
                 "&field1=" + String(temperaturaBMP) +
                 "&field2=" + String(event.pressure) +
                 "&field3=" + String(altitude) +
                 "&field4=" + String(umidade) +
                 "&field5=" + String(temperaturaAHT) +
                 "&field6=" + String(hif_C);
    
    http.begin(url);
    int httpCode = http.GET(); // Faz a requisição GET

    if (httpCode > 0) {
      Serial.println("Dados enviados com sucesso!");
    } else {
      Serial.println("Falha ao enviar dados: " + http.errorToString(httpCode));
    }

    http.end();
  }

  delay(20000); // ThingSpeak permite uma atualização a cada 15 segundos
}
