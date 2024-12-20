#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>

// Pines para el módulo MicroSD
#define SCK 36
#define MISO 35
#define MOSI 37
#define CS 5

// Pines para el RTC DS3231
#define SDA_PIN 21
#define SCL_PIN 19

// Pines para sensores de temperatura
#define TEMP_PIN 17

// Número de sensores y distancia entre sensores
#define NUM_SENSORS 34   // Cambiar a 42 si es necesario
#define DISTANCE_CM 5    // Distancia entre sensores

// Dirección fija del nodo
#define NODE_NUMBER 1

// Variables globales
RTC_DS3231 rtc;                           // Objeto RTC
OneWire oneWire(TEMP_PIN);                // Configuración de OneWire
DallasTemperature sensors(&oneWire);      // Objeto para manejar sensores DS18B20
File dataFile;                            // Archivo en la SD

// Direcciones de los sensores
uint64_t addr[] = {
    0x66ec108034646128, 0x582d168034646128, 0x3b98118034646128, 0x341f118034646128,
    0xf185178034646128, 0x35db178034646128, 0xbfeb8d8d34646128, 0x82b76c8334646128,
    0x70816e8334646128, 0xd1bf6e8334646128, 0x39a46d8334646128, 0x6b497b8334646128,
    0x61d6f8334646128, 0x2e50281835646128, 0xc7b0281835646128, 0xb1d1241835646128,
    0xd724141835646128, 0x5bc4c1835646128, 0x9cb1c1835646128, 0x5810361835646128,
    0x7572e1835646128, 0x5789211835646128, 0x7572291835646128, 0x2d25291835646128,
    0x932b351835646128, 0xae600d1835646128, 0x2847271835646128, 0xbf30371835646128,
    0x6cf72f1835646128, 0x8d425e1c35646128, 0x5df15e1c35646128, 0x9a4a791b35646128,
    0xb4df791b35646128, 0x9ed26d1b35646128};

// Prototipos de funciones
void setupSD();
void logData();
void printToSerial(DateTime now, float temperatures[]);

void setup() {
  // Iniciar monitor serial
  Serial.begin(115200);

  // Inicializar RTC
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.begin()) {
    Serial.println("Error inicializando RTC.");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajusta al tiempo de compilación
  }

  // Inicializar sensores DS18B20
  sensors.begin();
  if (sensors.getDeviceCount() != NUM_SENSORS) {
    Serial.println("Error: número de sensores no coincide.");
    while (1);
  }

  // Inicializar MicroSD
  setupSD();
}

void loop() {
  static unsigned long lastLogTime = 0;
  unsigned long currentTime = millis();

  // Registrar datos cada 15 minutos
  if (currentTime - lastLogTime >= 15 * 60 * 1000 || lastLogTime == 0) {
    logData();
    lastLogTime = currentTime;
  }
}

void setupSD() {
  // Inicializar SPI
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
  dataFile = SD.open("/datalog.csv", FILE_APPEND);
  if (!dataFile) {
    Serial.println("Error abriendo el archivo en la SD.");
    return;
  }

  // Escribir datos en el archivo
  for (int i = 0; i < NUM_SENSORS; i++) {
    dataFile.printf(
      "%04d,%02d,%02d,%02d,%02d,%02d,%d,%d,%.2f\n",
      now.year(), now.month(), now.day(),
      now.hour(), now.minute(), now.second(),
      NODE_NUMBER, i * DISTANCE_CM, temperatures[i]
    );
  }
  dataFile.close();

  // Mostrar datos en el monitor serial
  printToSerial(now, temperatures);
}

void printToSerial(DateTime now, float temperatures[]) {
  Serial.printf("Registro %04d/%02d/%02d %02d:%02d:%02d\n",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second());

  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.printf("Sensor %d (Profundidad: %d cm): %.2f °C\n", 
      i, i * DISTANCE_CM, temperatures[i]);
  }
  Serial.println();
}