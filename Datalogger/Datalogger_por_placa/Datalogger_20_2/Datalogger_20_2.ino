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
#define NUM_SENSORS 20
#define DISTANCE_CM 5

// Dirección fija del nodo
#define NODE_NUMBER 4

// Variables globales
RTC_DS3231 rtc;                           
OneWire oneWire(TEMP_PIN);                
DallasTemperature sensors(&oneWire);      
File dataFile;                            

// Direcciones de los sensores
DeviceAddress sensorAddresses[20] = {
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x2A, 0x34, 0xBB }, // Nodo 0
  { 0x28, 0x61, 0x64, 0x34, 0x80, 0x11, 0x46, 0x73 }, // Nodo 1
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x27, 0x23, 0x2C }, // Nodo 2
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x31, 0x93, 0x58 }, // Nodo 3
  { 0x28, 0x61, 0x64, 0x35, 0x1B, 0x4D, 0x4B, 0xD2 }, // Nodo 4
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x1C, 0x13, 0x9C }, // Nodo 5
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x28, 0x7B, 0x2D }, // Nodo 6
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x25, 0xE8, 0x57 }, // Nodo 7
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x20, 0x09, 0x1F }, // Nodo 8
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x28, 0x67, 0x13 }, // Nodo 9
  { 0x28, 0x61, 0x64, 0x34, 0x80, 0x16, 0x64, 0x82 }, // Nodo 10
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x3E, 0xE7, 0xD9 }, // Nodo 11
  { 0x28, 0x61, 0x64, 0x34, 0x83, 0x68, 0x2E, 0x34 }, // Nodo 12
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x23, 0xE2, 0x83 }, // Nodo 13
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x35, 0x91, 0xDF }, // Nodo 14
  { 0x28, 0x61, 0x64, 0x34, 0x83, 0x6E, 0x40, 0xE4 }, // Nodo 15
  { 0x28, 0x61, 0x64, 0x34, 0x83, 0x66, 0x25, 0xC8 }, // Nodo 16
  { 0x28, 0x61, 0x64, 0x34, 0x83, 0x6D, 0x90, 0xE6 }, // Nodo 17
  { 0x28, 0x61, 0x64, 0x35, 0x1B, 0x71, 0xDE, 0x9C }, // Nodo 18
  { 0x28, 0x61, 0x64, 0x35, 0x18, 0x34, 0xC2, 0x22 }, // Nodo 19
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
