#ifndef __ADS1235__
#define __ADS1235__
#include <Arduino.h>
#include <hardware/pio.h>
#include <api/HardwareSPI.h>
#include <hardware/spi.h>
#include "RunningAverage.h"

#define SECONDS_IN_MICROS 1000000

/**
 * This is the library to interface with the ADS1235 ADC . This is a singleton
 * class. Use ADS1235::createInstance() & ADS1235::getInstance() 
 * Based on http://www.ti.com/product/ads1235-q1?qgpn=ads1235-q1
 */
class ADS1235 {

  SPIClass* spi;
  SPISettings* spiSettings;
  uint8_t nrstPin, npwdnPin, startPin, ndrdyPin;
  uint16_t numberOfSamplesPerSec = 0;
  unsigned long calibrationStartMicros = 0;
  RunningAverage *runningAverage = NULL;

  static ADS1235* singleton;

public:

  enum Register : uint8_t {
    ID = 0x00,
    STATUS = 0x01,
    MODE0 = 0x02,
    MODE1 = 0x03,
    MODE2 = 0x04,
    MODE3 = 0x05,
    REF = 0x06,
    OFCAL0 = 0x07,
    OFCAL1 = 0x08,
    OFCAL2 = 0x09,
    FSCAL0 = 0x0A,
    FSCAL1 = 0x0B,
    FSCAL3 = 0x0C,
    PGA = 0x10,
    INPMUX = 0x11,
  };

  enum InputMux : uint8_t {
    AIN0 = 0b0011,
    AIN1 = 0b0100,
    AIN2 = 0b0101,
    AIN3 = 0b0110,
    AIN4 = 0b0111,
    AIN5 = 0b1000,
    INVALID_MUX = 0b1111,
  };

  enum ADCMode: uint8_t {
    NORMAL = 0b00,
    CHOP   = 0b01,
    W2_AC  = 0b10,
    W4_AC  = 0b11,
  };

  enum DataRate : uint8_t {
    SPS_2 = 0b0000,  // Actually 2.5 SPS
    SPS_5 = 0b0001,
    SPS_10 = 0b0010,
    SPS_16 = 0b0011,  // Actually 16.66666... SPS
    SPS_20 = 0b0100,
    SPS_50 = 0b0101,
    SPS_60 = 0b0110,
    SPS_100 = 0b0111,
    SPS_400 = 0b1000,
    SPS_1200 = 0b1001,
    SPS_2400 = 0b1010,
    SPS_4800 = 0b1011,
    SPS_7200 = 0b1100,
    INVALID_DR = 0b1111,
  };

  void init();
  void exchangeData(unsigned char* data, unsigned char* rx_buffer, size_t size);
  uint8_t readRegister(Register reg);
  void writeRegister(Register reg, unsigned char payload);
  void setInputMux(InputMux positive, InputMux negative);
  InputMux getPositiveInputMux();
  InputMux getNegativeInputMux();
  void setDataRate(DataRate rate);
  DataRate getDataRate();
  int getDataRateAsInt();
  int getRealDataRateAsInt();
  void setMode(ADCMode mode);
  ADCMode getMode();
  void caliberate();
  RunningAverage* getRunningAverage();
  long getLastADCValue();
  void setLastADCValue(long val);
  void reset();
  void maybeUpdateNumberOfSamplesPerSec();
  static ADS1235* getInstance();
  static void createInstance(SPIClass& spi_, SPISettings& spi_settings, uint8_t nrst_pin, uint8_t npwdn_pin, uint8_t start_pin, uint8_t ndrdy_pin);

  private:
    long readADCRaw();
    ADS1235(SPIClass& spi_, SPISettings& spi_settings, uint8_t nrst_pin, uint8_t npwdn_pin, uint8_t start_pin, uint8_t ndrdy_pin);
    InputMux getInputMuxFromByte(uint8_t byte);
    static void handleDrdyChange();
};
#endif