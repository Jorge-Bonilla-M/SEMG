//////////////////////////////////////////////////////////////////////////////////////////
//
//   Arduino Library for ADS1292R Shield/Breakout
//
//   Copyright (c) 2017 ProtoCentral
//   Heartrate and respiration computation based on original code from Texas Instruments
//   Note: This example is not compatible with arduino uno as the algorithm requires higher RAM size for its computations.
//
//   This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   Requires g4p_control graphing library for processing.  Built on V4.1
//   Downloaded from Processing IDE Sketch->Import Library->Add Library->G4P Install
//   If you have bought the breakout the connection with the Arduino board is as follows:
//
//  |ads1292r pin label| Arduino Connection   |Pin Function      |
//  |----------------- |:--------------------:|-----------------:|
//  | VDD              | +3.3V                |  Supply voltage  |
//  | PWDN/RESET       | D19                  |  Reset           |
//  | START            | D18                  |  Start Input     |
//  | DRDY             | D27                  |  Data Ready Outpt|
//  | CS1              | D21                  |  Chip Select     |
//  | MOSI             | D13                  |  Slave In        |
//  | MISO             | D12                  |  Slave Out       |
//  | SCK              | D14                  |  Serial Clock    |
//  | GND              | Gnd                  |  Gnd             |
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "protocentralAds1292r.h"
#include <SPI.h>
#include <Arduino.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

volatile uint8_t globalHeartRate;
volatile uint8_t globalRespirationRate=0;

const int ADS1292_DRDY_PIN = 27;
const int ADS1292_START_PIN = 18;
const int ADS1292_PWDN_PIN = 19;
const int CS_1 = 25;
const int CS_2 = 25;
const int CS_4 = 4;
const int CS_5 = 16;

SPIClass *hspi = NULL;
int SPI_MOSI = 13;
int SPI_MISO = 12;
int SPI_SCK = 14;

const int PWM_CHANNEL = 0;
const int PWM_FREQ = 512000;
const int PWM_RESOLUTION = 2;
const int PWM_PIN = 17;

int16_t ecgWaveBuff;
int16_t resWaveBuff;

long timeElapsed=0;

ads1292r ADS1292R;

volatile char DataPacketHeader[5];
volatile char DataPacketFooter[2];
volatile int datalen = 135;

volatile byte SPI_RX_Buff[150] ;
volatile static int SPI_RX_Buff_Count = 0;
volatile char* SPI_RX_Buff_Ptr;
volatile int Responsebyte = false;
volatile unsigned int pckt=0;
volatile unsigned int buff=0;
volatile unsigned int t=0;
volatile unsigned int j2=0;
volatile unsigned long int EEG_Ch1_Data[150],EEG_Ch2_Data[150];
volatile unsigned char datac[150];
unsigned long ueegtemp = 0,Pkt_Counter=0;
signed long seegtemp=0;
volatile int j,i;
float feegtemp ,output;

volatile signed long EMGValues[24];
volatile bool ads1292dataReceived = false;
unsigned long uecgtemp = 0;
unsigned long resultTemp = 0;
long statusByte = 0;
signed long secgtemp = 0;

void setup()
{
  delay(2000);

  hspi = new SPIClass(HSPI);

  pinMode(ADS1292_DRDY_PIN, INPUT);
  pinMode(CS_1, OUTPUT);
  pinMode(CS_2, OUTPUT);
  pinMode(CS_4, OUTPUT);
  pinMode(CS_5, OUTPUT);
  pinMode(ADS1292_START_PIN, OUTPUT);
  pinMode(ADS1292_PWDN_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  digitalWrite(CS_2, HIGH);
  digitalWrite(CS_4, HIGH);
  digitalWrite(CS_5, HIGH);

  hspi->begin(SPI_SCK, SPI_MISO, SPI_MOSI, CS_1);
  hspi->setBitOrder(MSBFIRST);
  //CPOL = 0, CPHA = 1
  hspi->setDataMode(SPI_MODE1);
  // Selecting 1Mhz clock for SPI
  hspi->setClockDivider(SPI_CLOCK_DIV16);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);

  ADS1292R.ads1292Init(CS_1,ADS1292_PWDN_PIN,ADS1292_START_PIN);  //initalize ADS1292 master
  ADS1292R.ads1292Init(CS_2,ADS1292_PWDN_PIN,ADS1292_START_PIN);  //initalize ADS1292 slave
  ADS1292R.ads1292Init(CS_4,ADS1292_PWDN_PIN,ADS1292_START_PIN);  //initalize ADS1292 slave
  ADS1292R.ads1292Init(CS_5,ADS1292_PWDN_PIN,ADS1292_START_PIN);  //initalize ADS1292 slave
  
  ledcWrite(PWM_CHANNEL, 2);
  Serial.begin(115200);
  Serial.println("Initiliziation is done");
  SerialBT.begin("ADS1292R-BT");  // Nombre del dispositivo Bluetooth
  Serial.println("Bluetooth is initialized");
}

void loop(){

  if (digitalRead(ADS1292_DRDY_PIN) == LOW){

    if(millis() > timeElapsed){
      
      ReadAdc(CS_1);
      timeElapsed += 1;
    }
  }
}

char* ReadData(const int chipSelect){
  static char SPI_Dummy_Buff[10];
  digitalWrite(chipSelect, LOW);

  for (int i = 0; i < 9; ++i)
  {
    SPI_Dummy_Buff[i] = SPI.transfer(CONFIG_SPI_MASTER_DUMMY);
  }

  digitalWrite(chipSelect, HIGH);
  return SPI_Dummy_Buff;
}
void ReadAdc(const int chipSelect){
  if ((digitalRead(ADS1292_DRDY_PIN)) == LOW)      // Sampling rate is set to 125SPS ,DRDY ticks for every 8ms
  {
    SPI_RX_Buff_Ptr = ReadData(chipSelect); // Read the data,point the data to a pointer

    for (int i = 0; i < 9; i++)
    {
      SPI_RX_Buff[SPI_RX_Buff_Count++] = *(SPI_RX_Buff_Ptr + i);  // store the result data in array
    }

    ads1292dataReceived = true;
    //Serial.print("datarecieved from ads 1292R    ");
    //Serial.println(ads1292dataReceived);
    //delay(1000);
    j = 0;

    for (i = 3; i < 9; i += 3)         // data outputs is (24 status bits + 24 bits Respiration data +  24 bits ECG data)
    {
      uecgtemp = (unsigned long) (  ((unsigned long)SPI_RX_Buff[i + 0] << 16) | ( (unsigned long) SPI_RX_Buff[i + 1] << 8) |  (unsigned long) SPI_RX_Buff[i + 2]);
      uecgtemp = (unsigned long) (uecgtemp << 8);
      secgtemp = (signed long) (uecgtemp);
      secgtemp = (signed long) (secgtemp >> 8);

      EMGValues[j++] = secgtemp;  //s32DaqVals[0] is Resp data and s32DaqVals[1] is ECG data
    }
    statusByte = (long)((long)SPI_RX_Buff[2] | ((long) SPI_RX_Buff[1]) <<8 | ((long) SPI_RX_Buff[0])<<16); // First 3 bytes represents the status
    Serial.print(statusByte);
    Serial.print(",");

    Serial.print(EMGValues[0]);
    Serial.print(",");
    Serial.println(EMGValues[1]);
    //Serial.println(EMGValues[1]);
    //BLUETOOTH COMMUNICATION
    SerialBT.print(statusByte);
    SerialBT.print(",");

    SerialBT.print(EMGValues[0]);
    SerialBT.print(",");
    SerialBT.println(EMGValues[1]);

    ads1292dataReceived = false;
    SPI_RX_Buff_Count = 0;
  }
} 

  
