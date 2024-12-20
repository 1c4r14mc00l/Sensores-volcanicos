#include <SPI.h>
#include <SD.h>

// Pines para el módulo microSD
#define SCK 36
#define MISO 35
#define MOSI 37
#define CS 5

File myFile;

void setup() {
  // Iniciar comunicación serie
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Esperar hasta que se abra el puerto serie
  }

  Serial.println("Iniciando módulo microSD...");
  
  // Configurar SPI para usar los pines personalizados
  SPI.begin(SCK, MISO, MOSI, CS);
  
  // Inicializar la tarjeta SD
  if (!SD.begin(CS)) {
    Serial.println("Error al inicializar la tarjeta SD. Verifica la conexión.");
    return;
  }
  Serial.println("Tarjeta SD inicializada correctamente.");

  // Crear y abrir un archivo para escritura
  myFile = SD.open("/prueba.txt", FILE_WRITE);
  if (myFile) {
    Serial.println("Escribiendo en el archivo...");
    myFile.println("Hola, este es un archivo de prueba en la microSD.");
    myFile.println("ESP32 conectado correctamente.");
    myFile.close(); // Cerrar el archivo después de escribir
    Serial.println("Escritura completa.");
  } else {
    Serial.println("Error al abrir el archivo para escritura.");
  }

  // Abrir el archivo para lectura
  myFile = SD.open("/prueba.txt");
  if (myFile) {
    Serial.println("Leyendo el archivo...");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close(); // Cerrar el archivo después de leer
    Serial.println("\nLectura completa.");
  } else {
    Serial.println("Error al abrir el archivo para lectura.");
  }
}

void loop() {
  // No se necesita código en el loop para esta prueba
}
