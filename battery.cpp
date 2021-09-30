/*
   HelTec Automation(TM) LoRaWAN 1.0.2 OTAA example use OTAA, CLASS A

   Function summary:

   - use internal RTC(150KHz);

   - Forward the data measured by the 12-bit ADC to the server.

   - Include stop mode and deep sleep mode;

   - 15S data send cycle;

   - Informations output via serial(115200);

   - Only ESP32 + LoRa series boards can use this library, need a license
     to make the code run(check you license here: http://www.heltec.cn/search/);

   You can change some definition in "Commissioning.h" and "LoRaMac-definitions.h"

   HelTec AutoMation, Chengdu, China.
   成都惠利特自动化科技有限公司
   https://heltec.org
   support@heltec.cn

  this project also release in GitHub:
  https://github.com/HelTecAutomation/ESP32_LoRaWAN
*/

#include <ESP32_LoRaWAN.h>
#include "Arduino.h"

/*license for Heltec ESP32 LoRaWan, quary your ChipID relevant license: http://resource.heltec.cn/search */
uint32_t  license[4] = XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
/* OTAA para*/
//WireLessStick
uint8_t DevEui[] = xxxxxxxxx
uint8_t AppKey[] = x
uint8_t AppEui[] = {x

//WirelessStick_811
//uint8_t DevEui[] = x
//uint8_t AppKey[] = x
//uint8_t AppEui[] = x

/* ABP para*/
uint8_t NwkSKey[] = x
uint8_t AppSKey[] = x;
uint32_t DevAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };


/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/*!
  Number of trials to transmit the frame, if the LoRaMAC layer did not
  receive an acknowledgment. The MAC performs a datarate adaptation,
  according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
  to the following table:

  Transmission nb | Data Rate
  ----------------|-----------
  1 (first)       | DR
  2               | DR
  3               | max(DR-1,0)
  4               | max(DR-1,0)
  5               | max(DR-2,0)
  6               | max(DR-2,0)
  7               | max(DR-3,0)
  8               | max(DR-3,0)

  Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
  the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 8;

/*LoraWan debug level, select in arduino IDE tools.
  None : print basic info.
  Freq : print Tx and Rx freq, DR info.
  Freq && DIO : print Tx and Rx freq, DR, DIO0 interrupt and DIO1 interrupt info.
  Freq && DIO && PW: print Tx and Rx freq, DR, DIO0 interrupt, DIO1 interrupt and MCU deepsleep info.
*/
uint8_t debugLevel = LoRaWAN_DEBUG_LEVEL;

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

static void ADC_Process(uint16_t val) {
  appDataSize = 12 ;
  appData[5] = 0x00;
  appData[4] = 0x00;
  

  int TH     = (int)(val / 1000);
  appData[3] = (char)(TH) ;

  int HU     = (int)((val - TH * 1000) / 100);
  appData[2] = (char)(HU) ;

  int Ten    = (int)((val - TH * 1000 - HU * 100) / 10);
  appData[1] = (char)(Ten) ;

  int Sin    = (val % 10);
  appData[0] = (char)(Sin) ;
}
#define Vext 21
// Add your initialization code here
void setup()
{
  Serial.begin(115200);
  while (!Serial);
  SPI.begin(SCK, MISO, MOSI, SS);
  Mcu.init(SS, RST_LoRa, DIO0, DIO1, license);

  adcAttachPin(37);
  analogSetClockDiv(255); // 1338mS
  pinMode(Vext, OUTPUT);
  deviceState = DEVICE_STATE_INIT;
}

// The loop function is called in an endless loop
void loop()
{
  switch ( deviceState )
  {
    case DEVICE_STATE_INIT:
      {
        LoRaWAN.init(loraWanClass, loraWanRegion);
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        digitalWrite(Vext, LOW);
        delay(10);
        uint16_t ADC_voltage = analogRead(37);
        digitalWrite(Vext, HIGH);
        ADC_Process( ADC_voltage );
        LoRaWAN.send(loraWanClass);
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
        LoRaWAN.sleep(loraWanClass, debugLevel);
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
}
