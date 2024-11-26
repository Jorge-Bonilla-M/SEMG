//////////////////////////////////////////////////////////////////////////////////////////
//
//   Arduino Library for ADS1292R Shield/Breakout
//
//   Copyright (c) 2017 ProtoCentral
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
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "protocentralAds1292r.h"
#include <SPI.h>


char* ads1292r::ads1292ReadData(const int chipSelect)
{
  static char SPI_Dummy_Buff[10];
  digitalWrite(chipSelect, LOW);

  for (int i = 0; i < 9; ++i)
  {
    SPI_Dummy_Buff[i] = SPI.transfer(CONFIG_SPI_MASTER_DUMMY);
  }

  digitalWrite(chipSelect, HIGH);
  return SPI_Dummy_Buff;
}

void ads1292r::ads1292Init(const int chipSelect,const int pwdnPin,const int startPin)
{
  // start the SPI library:
  ads1292Reset(pwdnPin);
  delay(100);
  ads1292DisableStart(startPin);
  ads1292EnableStart(startPin);
  ads1292HardStop(startPin);
  ads1292StartDataConvCommand(chipSelect);
  ads1292SoftStop(chipSelect);
  delay(50);
  ads1292StopReadDataContinuous(chipSelect);					// SDATAC command
  delay(300);
  ads1292RegWrite(ADS1292_REG_CONFIG1, 0x00,chipSelect); 		//Set sampling rate to 125 SPS
  delay(10);
  ads1292RegWrite(ADS1292_REG_CONFIG2, 0b10000000,chipSelect);	//Lead-off comp off, test signal disabled
  delay(10);
  ads1292RegWrite(ADS1292_REG_LOFF, 0b00010000,chipSelect);		//Lead-off defaults
  delay(10);                  //Cambio a 000 registro CH1SET para que tenga ganancia de 6 igual que CH2SET
  ads1292RegWrite(ADS1292_REG_CH1SET, 0b00110000,chipSelect);	//Ch 1 enabled, gain 6, connected to electrode in
  delay(10);                  //Cambio a 000 registro CH1SET para que tenga ganancia de 6 igual que CH1SET
  ads1292RegWrite(ADS1292_REG_CH2SET, 0b00110000,chipSelect);	//Ch 2 enabled, gain 6, connected to electrode in
  delay(10);          //original abajo 0b00101100
  ads1292RegWrite(ADS1292_REG_RLDSENS, 0b00110000,chipSelect);	//RLD settings: fmod/16, RLD enabled, RLD inputs from Ch2 only
  delay(10);
  ads1292RegWrite(ADS1292_REG_LOFFSENS, 0x00,chipSelect);		//LOFF settings: all disabled
  delay(10);		
  ads1292RegWrite(ADS1292_REG_LOFFSTAT, 0x00,chipSelect);		//LOFF Status:
  delay(10);											
  ads1292RegWrite(ADS1292_REG_GPIO, 0b00001100,chipSelect);		//GPIO as inputs, pin 1 and 0 have no effect
  delay(10);
  ads1292StartReadDataContinuous(chipSelect);
  delay(10);
  ads1292EnableStart(startPin);
}

void ads1292r::ads1292Reset(const int pwdnPin)
{
  digitalWrite(pwdnPin, HIGH);
  delay(100);					// Wait 100 mSec
  digitalWrite(pwdnPin, LOW);
  delay(100);
  digitalWrite(pwdnPin, HIGH);
  delay(100);
}

void ads1292r::ads1292DisableStart(const int startPin)
{
  digitalWrite(startPin, LOW);
  delay(20);
}

void ads1292r::ads1292EnableStart(const int startPin)
{
  digitalWrite(startPin, HIGH);
  delay(20);
}

void ads1292r::ads1292HardStop (const int startPin)
{
  digitalWrite(startPin, LOW);
  delay(100);
}

void ads1292r::ads1292StartDataConvCommand (const int chipSelect)
{
  ads1292SPICommandData(START,chipSelect);					// Send 0x08 to the ADS1x9x
}

void ads1292r::ads1292SoftStop (const int chipSelect)
{
  ads1292SPICommandData(STOP,chipSelect);                   // Send 0x0A to the ADS1x9x
}

void ads1292r::ads1292StartReadDataContinuous (const int chipSelect)
{
  ads1292SPICommandData(RDATAC,chipSelect);					// Send 0x10 to the ADS1x9x
}

void ads1292r::ads1292StopReadDataContinuous (const int chipSelect)
{
  ads1292SPICommandData(SDATAC,chipSelect);					// Send 0x11 to the ADS1x9x
}

void ads1292r::ads1292SPICommandData(unsigned char dataIn,const int chipSelect)
{
  byte data[1];
  //data[0] = dataIn;
  digitalWrite(chipSelect, LOW);
  delay(2);
  digitalWrite(chipSelect, HIGH);
  delay(2);
  digitalWrite(chipSelect, LOW);
  delay(2);
  SPI.transfer(dataIn);
  delay(2);
  digitalWrite(chipSelect, HIGH);
}

//Sends a write command to SCP1000
void ads1292r::ads1292RegWrite (unsigned char READ_WRITE_ADDRESS, unsigned char DATA,const int chipSelect)
{

  switch (READ_WRITE_ADDRESS)
  {
    case 1:
            DATA = DATA & 0x87;
	          break;
    case 2:
            DATA = DATA & 0xFB;
	          DATA |= 0x80;
	          break;
    case 3:
      	    DATA = DATA & 0xFD;
      	    DATA |= 0x10;
      	    break;
    case 7:
      	    DATA = DATA & 0x3F;
      	    break;
    case 8:
    	      DATA = DATA & 0x5F;
	          break;
    case 9:
      	    DATA |= 0x02;
      	    break;
    case 10:
      	    DATA = DATA & 0x87;
      	    DATA |= 0x01;
      	    break;
    case 11:
      	    DATA = DATA & 0x0F;
      	    break;
    default:
            break;
  }
  // now combine the register address and the command into one byte:
  byte dataToSend = READ_WRITE_ADDRESS | WREG;
  digitalWrite(chipSelect, LOW);
  delay(2);
  digitalWrite(chipSelect, HIGH);
  delay(2);
  // take the chip select low to select the device:
  digitalWrite(chipSelect, LOW);
  delay(2);
  SPI.transfer(dataToSend); //Send register location
  SPI.transfer(0x00);		//number of register to wr
  SPI.transfer(DATA);		//Send value to record into register
  delay(2);
  // take the chip select high to de-select:
  digitalWrite(chipSelect, HIGH);
}
