#include <Wire.h>
#include <RTClib.h>
#include <SD.h>

// Pines para el módulo MicroSD
#define SCK 36
#define MISO 35
#define MOSI 37
#define CS 5

// Pines para el RTC DS3231
#define SDA_PIN 21
#define SCL_PIN 19

RTC_DS3231 rtc;

void setup() {
  // Inicializa comunicación serial
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Configurar SPI para usar los pines personalizados
  SPI.begin(SCK, MISO, MOSI, CS);

  // Configura pines I2C para el RTC
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializa RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC perdió la energía, configurando fecha/hora por defecto.");
    // Configura el RTC con la fecha y hora de compilación
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicializa la tarjeta SD
  if (!SD.begin(CS)) {
    Serial.println("Fallo al inicializar la tarjeta SD.");
    while (1);
  }
  Serial.println("Tarjeta SD inicializada correctamente.");

  // Genera y escribe la fecha en la SD
  escribeFechaEnSD();

  // Lee el contenido del archivo y lo muestra en el monitor serial
  leeArchivoSD();
}

void loop() {
  // No se necesita realizar nada en el loop
}

void escribeFechaEnSD() {
  // Obtiene la fecha y hora actual del RTC
  DateTime now = rtc.now();
  
  // Formatea la fecha y hora como una cadena
  String fechaHora = String(now.year()) + "-" +
                     String(now.month()) + "-" +
                     String(now.day()) + " " +
                     String(now.hour()) + ":" +
                     String(now.minute()) + ":" +
                     String(now.second());

  // Abre el archivo para escribir (crea si no existe, añade si ya existe)
  File archivo = SD.open("/fecha.txt", FILE_APPEND);
  if (archivo) {
    archivo.println(fechaHora);
    Serial.println("Fecha/hora escrita en la SD: " + fechaHora);
    archivo.close();
  } else {
    Serial.println("Error al abrir el archivo para escribir.");
  }
}

void leeArchivoSD() {
  // Abre el archivo para lectura
  File archivo = SD.open("/fecha.txt", FILE_READ);
  if (archivo) {
    Serial.println("Contenido del archivo /fecha.txt:");
    while (archivo.available()) {
      Serial.write(archivo.read());
    }
    archivo.close();
  } else {
    Serial.println("Error al abrir el archivo para leer.");
  }
}