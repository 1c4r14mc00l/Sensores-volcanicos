#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sleep.h>

// Pines para el módulo MicroSD
#define SCK 36
#define MISO 35
#define MOSI 37
#define CS 5

// Pines para el RTC DS3231
#define SDA_PIN 21
#define SCL_PIN 19
#define WAKE_PIN GPIO_NUM_4  // Pin para despertar el ESP32 desde el RTC

// Pines para sensores de temperatura
#define TEMP_PIN 17

// Número de sensores y distancia entre sensores
#define NUM_SENSORS 36
#define DISTANCE_CM 5

// Dirección fija del nodo
#define NODE_NUMBER 4

// Variables globales
RTC_DS3231 rtc;                           
OneWire oneWire(TEMP_PIN);                
DallasTemperature sensors(&oneWire);      
File dataFile;                            

// Direcciones de los sensores
DeviceAddress sensorAddresses[36] = {
  {0x28, 0xFF, 0x64, 0x0E, 0x71, 0xCB, 0xE4, 0x08},
  {0x28, 0xFF, 0x64, 0x0E, 0x72, 0x27, 0x72, 0x18},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x6E, 0xEE, 0xD9},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x68, 0x39, 0x92},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x6D, 0xFD, 0xC6},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x26, 0x07, 0x74},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x68, 0x3C, 0x98},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x67, 0x7E, 0x7A},
  {0x28, 0xFF, 0x64, 0x0E, 0x6B, 0xB4, 0xB4, 0x5D},
  {0x28, 0xFF, 0x64, 0x0E, 0x6B, 0xB1, 0xD6, 0x7B},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x6A, 0xE0, 0xC8},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x33, 0xF7, 0x13},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x27, 0x52, 0x54},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x48, 0x5B, 0x8A},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x71, 0xA0, 0x41},
  {0x28, 0xFF, 0x64, 0x0E, 0x6B, 0xB6, 0x9A, 0xF0},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x65, 0xC3, 0x24},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x4F, 0x85, 0xAC},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x24, 0xF2, 0xAE},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x61, 0x5C, 0x4F},
  {0x28, 0xFF, 0x64, 0x0E, 0x6A, 0x4D, 0x90, 0x9F},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x6A, 0xBF, 0x67},
  {0x28, 0xFF, 0x64, 0x0E, 0x6B, 0xB1, 0x47, 0x34},
  {0x28, 0xFF, 0x64, 0x0E, 0x6F, 0x6E, 0xC7, 0x66},
  {0x28, 0xFF, 0x64, 0x0E, 0x6B, 0xB1, 0x22, 0x6E},
  {0x28, 0xFF, 0x64, 0x0E, 0x75, 0x50, 0x44, 0xD9},
  {0x28, 0xFF, 0x64, 0x0E, 0x7B, 0x5B, 0x60, 0x4C},
  {0x28, 0xFF, 0x64, 0x0E, 0x69, 0x0F, 0x50, 0xBB}
};



// Prototipos de funciones
void setupSD();
void logData();
void setAlarm();

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.begin()) {
    Serial.println("Error inicializando RTC.");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  setAlarm();

  pinMode(WAKE_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(WAKE_PIN, LOW);

  sensors.begin();
  setupSD();
}

void loop() {
  logData();
  Serial.println("Entrando en DeepSleep...");
  delay(100);
  esp_deep_sleep_start();
}

void setupSD() {
  SPI.begin(SCK, MISO, MOSI, CS);

  if (!SD.begin(CS)) {
    Serial.println("Error inicializando la tarjeta SD.");
    while (1);
  }
  Serial.println("Tarjeta SD inicializada correctamente.");
}

void logData() {
  // Obtener tiempo actual del RTC
  DateTime now = rtc.now();

  // Solicitar temperaturas a los sensores
  sensors.requestTemperatures();
  float temperatures[NUM_SENSORS];
  for (int i = 0; i < NUM_SENSORS; i++) {
    temperatures[i] = sensors.getTempCByIndex(i);
  }

  // Crear o abrir el archivo CSV
  bool fileExists = SD.exists("/datalog.csv");
  dataFile = SD.open("/datalog.csv", FILE_APPEND);
  if (!dataFile) {
    Serial.println("Error abriendo el archivo en la SD.");
    return;
  }

  // Escribir encabezados si el archivo no existe
  if (!fileExists) {
    dataFile.println("Año,Mes,Día,Hora,Minuto,Segundo,Nodo,Profundidad (cm),Temperatura (°C),Dirección del Sensor");
  }

  // Escribir datos en el archivo
  for (int i = 0; i < NUM_SENSORS; i++) {
    char addressBuffer[60]; // Buffer para almacenar la dirección del sensor en el formato solicitado
    snprintf(
      addressBuffer, sizeof(addressBuffer), 
      "{ 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X }", 
      sensorAddresses[i][0], sensorAddresses[i][1], sensorAddresses[i][2], 
      sensorAddresses[i][3], sensorAddresses[i][4], sensorAddresses[i][5], 
      sensorAddresses[i][6], sensorAddresses[i][7]
    );

    dataFile.printf(
      "%04d,%02d,%02d,%02d,%02d,%02d,%d,%d,%.2f,%s\n",
      now.year(), now.month(), now.day(),
      now.hour(), now.minute(), now.second(),
      NODE_NUMBER, i * DISTANCE_CM, temperatures[i], addressBuffer
    );
  }
  dataFile.close();



  // Mostrar datos en el monitor serial
  // printToSerial(now, temperatures);
}


// void printToSerial(DateTime now, float temperatures[]) {
//   Serial.printf("Registro %04d/%02d/%02d %02d:%02d:%02d\n",
//     now.year(), now.month(), now.day(),
//     now.hour(), now.minute(), now.second());

//   for (int i = 0; i < NUM_SENSORS; i++) {
//     Serial.printf("Sensor %d (Profundidad: %d cm): %.2f °C\n", 
//       i, i * DISTANCE_CM, temperatures[i]);
//   }
//   Serial.println();
// }

void setAlarm() {
  DateTime now = rtc.now();
  DateTime alarmTime = now + TimeSpan(0, 0, 15, 0);
  rtc.clearAlarm(1);
  rtc.setAlarm1(alarmTime, DS3231_A1_Minute);
}
