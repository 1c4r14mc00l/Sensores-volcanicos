#include "LoRaWan_APP.h"
#include "OneWireESP32.h"

// Declarar temperatures en memoria RTC
RTC_DATA_ATTR int start = 0;  // Variable para el índice del sensor actual
RTC_DATA_ATTR uint8_t MaxDevs = 34;
RTC_DATA_ATTR bool MatrizCreada = false;
RTC_DATA_ATTR int temperatures[34][3] = { 0 };  // Matriz para las temperaturas
RTC_DATA_ATTR uint32_t appTxDutyCycle = 5000;  // Ciclo inicial de transmisión: 3 segundos

/* OTAA para*/
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x53, 0xC8 };
uint8_t appEui[] = { 0x0D, 0xDB, 0x4E, 0x05, 0xD0, 0xBF, 0x54, 0x77 };
uint8_t appKey[] = { 0x74, 0xD6, 0x6E, 0x63, 0x45, 0x82, 0x48, 0x27, 0xFE, 0xC5, 0xB7, 0x70, 0xBA, 0x2B, 0x50, 0x45 };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xB1, 0xD0, 0xEF, 0xA4, 0x63, 0xDF, 0xBE, 0x3D, 0x11, 0x18, 0x1E, 0x1E, 0xC7, 0xDA,0x85 };
uint8_t appSKey[] = { 0xD7, 0x2C, 0x78, 0x75, 0x8C, 0xDC, 0xCA, 0xBF, 0x55, 0xEE, 0x4A, 0x77, 0x8D, 0x16, 0xEF,0x67 };
uint32_t devAddr =  ( uint32_t )0x007E6AE1;

uint16_t userChannelsMask[6]={ 0x0000,0x0000,0x0000,0x0000,0x0000,0x00FF };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

// *******Sensors Information*******

// Función para leer sensores
const int x = MaxDevs;
const int y = 3;

static void ReadSensors() {
  if (!MatrizCreada){
  // Serial.println("Entrando a ReadSensors...");

  OneWire32 ds(7);  // Inicializar OneWire en el pin 17
  uint64_t addr[] = {
    0x66ec108034646128,
    0x582d168034646128,
    0x3b98118034646128,
    0x341f118034646128,
    0xf185178034646128,
    0x35db178034646128,
    0xbfeb8d8d34646128,
    0x82b76c8334646128,
    0x70816e8334646128,
    0xd1bf6e8334646128,
    0x39a46d8334646128,
    0x6b497b8334646128,
    0x61d6f8334646128,
    0x2e50281835646128,
    0xc7b0281835646128,
    0xb1d1241835646128,
    0xd724141835646128,
    0x5bc4c1835646128,
    0x9cb1c1835646128,
    0x5810361835646128,
    0x7572e1835646128,
    0x5789211835646128,
    0x7572291835646128,
    0x2d25291835646128,
    0x932b351835646128,
    0xae600d1835646128,
    0x2847271835646128,
    0xbf30371835646128,
    0x6cf72f1835646128,
    0x8d425e1c35646128,
    0x5df15e1c35646128,
    0x9a4a791b35646128,
    0xb4df791b35646128,
    0x9ed26d1b35646128,
  };  // Direcciones de los sensores
  
  // Inicializar la matriz con ceros
    // Serial.println("Inicializando la matriz con ceros...");
    for (int i = 0; i < x; i++) {
      for (int j = 0; j < y; j++) {
        temperatures[i][j] = 0;
      }
    }
    Serial.println("Matriz de ceros creada");

    // Solicitar lectura de temperaturas
    // Serial.println("Comenzando a leer sensores...");
    vTaskDelay(750 / portTICK_PERIOD_MS);
    ds.request();
    vTaskDelay(750 / portTICK_PERIOD_MS);  // Esperar el tiempo necesario para la lectura

    // Leer cada sensor
    for (int k = 0; k < MaxDevs; k++) {
      float tempC;
      uint8_t err = ds.getTemp(addr[k], tempC);

      if (!err) {
        int temp = tempC * 100;
        int e = temp / 100;
        int d = temp % 100;

        temperatures[k][0] = k;  // Índice del sensor
        temperatures[k][1] = e;  // Parte entera
        temperatures[k][2] = d;  // Parte decimal
      } else {
        Serial.printf("Error al leer el sensor %d\n", k);
      }
    }
    Serial.println("Lectura de sensores completada");
    MatrizCreada = true; // Cambiar el estado de la matriz
  } // Cierre del bloque 'if'
}

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
  appDataSize = 0;

  if (start < MaxDevs) {
    appTxDutyCycle = 5000;
    // Preparar datos para enviar
    appData[appDataSize++] = temperatures[start][0];
    appData[appDataSize++] = temperatures[start][1];
    appData[appDataSize++] = temperatures[start][2];
    Serial.printf("%d | %d | %d ( %d / %d ) at %d\n", appData[0], appData[1], appData[2], start, MaxDevs, appTxDutyCycle);
    start++; // Incrementar 'start' después de preparar la trama
  }

  if (start >= MaxDevs) {
    Serial.println(" DATO MAXIMO ALCANZADO, REINICIANDO... ");
    appTxDutyCycle = 60000; // Cambiar a tiempo extendido: 1 minuto
    start = 0; // Reiniciar 'start' a 0
    MatrizCreada = false; // Reiniciar la bandera para volver a crear la matriz
  }
}

// Configuración inicial
void setup() {
  Serial.begin(115200);
  // Serial.println("Comenzando Serial ...");
  ReadSensors();
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      //both set join DR and DR when ADR off 
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}