#include "ADS1235.h"

ADS1235* ADS1235::singleton = nullptr;

ADS1235::ADS1235(SPIClass& spi_, SPISettings& spi_settings, uint8_t nrst_pin, uint8_t npwdn_pin, uint8_t start_pin, uint8_t ndrdy_pin) {
  this->spi = &spi_;
  this->spiSettings = &spi_settings;
  this->nrstPin = nrst_pin;
  this->npwdnPin = npwdn_pin;
  this->startPin = start_pin;
  this->ndrdyPin = ndrdy_pin;
}

void ADS1235::init() {
  pinMode(this->nrstPin, OUTPUT);
  digitalWrite(this->nrstPin, HIGH);

  pinMode(this->npwdnPin, OUTPUT);
  digitalWrite(this->npwdnPin, HIGH);

  pinMode(this->startPin, OUTPUT);
  digitalWrite(this->startPin, HIGH);

  pinMode(this->ndrdyPin, INPUT_PULLUP);
  delay(100);
  this->reset();
  attachInterrupt(this->ndrdyPin, this->handleDrdyChange, FALLING);
}

void ADS1235::reset() {
  digitalWrite(this->startPin, LOW);
  delay(100);
  digitalWrite(this->startPin, HIGH);
}

void ADS1235::exchangeData(unsigned char* data, unsigned char* rx_buffer, size_t size) {
  this->spi->beginTransaction(*this->spiSettings);
  this->spi->transfer(data, rx_buffer, size);
  this->spi->endTransaction();
}

uint8_t ADS1235::readRegister(Register reg) {
  unsigned char data[3] = { 0 }, rx_buffer[3] = { 0 };
  data[0] = (uint8_t)reg + 0x20;
  this->exchangeData(data, rx_buffer, sizeof(data));
  return rx_buffer[2];
}

void ADS1235::writeRegister(Register reg, unsigned char payload) {
  digitalWrite(this->startPin, LOW);
  unsigned char data[2] = { 0 }, rx_buffer[2] = { 0 };
  data[0] = 0x40 + (uint8_t)reg;
  data[1] = payload;
  this->exchangeData(data, rx_buffer, sizeof(data));
  digitalWrite(this->startPin, HIGH);
}

void ADS1235::setMode(ADCMode mode){
  uint8_t reg = this->readRegister(ADS1235::Register::MODE1);
  reg = (reg & 0b10011111) | (mode << 5);
  this->writeRegister(ADS1235::Register::MODE1, reg);   
}

ADS1235::ADCMode ADS1235::getMode(){
  uint8_t reg = this->readRegister(ADS1235::Register::MODE1);
  uint8_t mode = (reg & 0b01100000) >> 5;
  return static_cast<ADCMode>(mode);
}

void ADS1235::caliberate() {
  unsigned char data[2] = { 0 }, rx_buffer[2] = { 0 };
  data[0] = 0x19;
  this->exchangeData(data, rx_buffer, sizeof(data));
  delay(100);

  // Measure number of samples we get in 1 sec in real time
  this->calibrationStartMicros = micros();
  this->numberOfSamplesPerSec = 0;

  // Wait for enough interrupts on handleDrdyChange
  while((micros() - this->calibrationStartMicros) < 1000000);

  if (this->runningAverage != NULL) {
    delete this->runningAverage;
  }
  this->runningAverage = new RunningAverage(this->numberOfSamplesPerSec*2);
  this->runningAverage->clear();
}

void ADS1235::setInputMux(InputMux positive, InputMux negative) {
  uint8_t payload = ((uint8_t)positive << 4) | ((uint8_t)negative);
  this->writeRegister(Register::INPMUX, payload);
}

ADS1235::InputMux ADS1235::getInputMuxFromByte(uint8_t byte) {
  switch (byte) {
    case InputMux::AIN0: 
      return InputMux::AIN0;
    case InputMux::AIN1: 
      return InputMux::AIN1;
    case InputMux::AIN2: 
      return InputMux::AIN2;
    case InputMux::AIN3: 
      return InputMux::AIN3;
    case InputMux::AIN4: 
      return InputMux::AIN4;
    case InputMux::AIN5: 
      return InputMux::AIN5;
  }
  return InputMux::INVALID_MUX;
}

ADS1235::InputMux ADS1235::getPositiveInputMux() {
  uint8_t reg = this->readRegister(ADS1235::Register::INPMUX);
  return getInputMuxFromByte((reg >> 4) & 0b00001111);
}

ADS1235::InputMux ADS1235::getNegativeInputMux() {
  uint8_t reg = this->readRegister(ADS1235::Register::INPMUX);
  return getInputMuxFromByte((reg) & 0b00001111);
}

void ADS1235::setDataRate(DataRate rate) {
  uint8_t reg = this->readRegister(Register::MODE0);
  reg = (reg & 0b10000111) | (rate << 3);
  this->writeRegister(Register::MODE0, reg);
}

ADS1235::DataRate ADS1235::getDataRate() {
  uint8_t reg = this->readRegister(Register::MODE0);
  reg = (reg >> 3) & 0b00001111;
  switch (reg) {
    case DataRate::SPS_2:
      return DataRate::SPS_2;
    case DataRate::SPS_5:
      return DataRate::SPS_5;
    case DataRate::SPS_10:
      return DataRate::SPS_10;
    case DataRate::SPS_16:
      return DataRate::SPS_16;
    case DataRate::SPS_20:
      return DataRate::SPS_20;
    case DataRate::SPS_50:
      return DataRate::SPS_50;
    case DataRate::SPS_60:
      return DataRate::SPS_60;
    case DataRate::SPS_100:
      return DataRate::SPS_100;
    case DataRate::SPS_400:
      return DataRate::SPS_400;
    case DataRate::SPS_1200:
      return DataRate::SPS_1200;
    case DataRate::SPS_2400:
      return DataRate::SPS_2400;
    case DataRate::SPS_4800:
      return DataRate::SPS_4800;
    case DataRate::SPS_7200:
      return DataRate::SPS_7200;
  }

  return DataRate::INVALID_DR; 
}

long ADS1235::readADCRaw() {
  unsigned char data[6] = { 0 }, rx_buffer[6] = { 0 };
  data[0] = 0x12;
  this->exchangeData(data, rx_buffer, sizeof(data));
  bool isNegative = (bool)((rx_buffer[2] & 0b10000000) >> 7);
  int32_t value =(((uint32_t)(isNegative ? 0xFF : 0x00)) << 24) + (((uint32_t)rx_buffer[2]) << 16) + (((uint32_t)rx_buffer[3]) << 8) + ((uint32_t)rx_buffer[4]);
  return value;
}

ADS1235* ADS1235::getInstance() {
  return singleton;
}
void ADS1235::createInstance(SPIClass& spi_, SPISettings& spi_settings, uint8_t nrst_pin, uint8_t npwdn_pin, uint8_t start_pin, uint8_t ndrdy_pin) {
  if (ADS1235::singleton != NULL) {
    return;
  }
  ADS1235::singleton = new ADS1235(
    spi_,
    spi_settings,
    nrst_pin,
    npwdn_pin,
    start_pin,
    ndrdy_pin
  );
}

void ADS1235::maybeUpdateNumberOfSamplesPerSec() {
  if ((micros() - this->calibrationStartMicros) < 1000000) {
    this->numberOfSamplesPerSec++;
  }
}

RunningAverage* ADS1235::getRunningAverage() {
  return this->runningAverage;
}

long ADS1235::getLastADCValue() {
  return this->lastRawADCValue;
}

void ADS1235::setLastADCValue(long val) {
  this->lastRawADCValue = val;
}

void ADS1235::handleDrdyChange() {
  ADS1235::getInstance()->maybeUpdateNumberOfSamplesPerSec();
  if (ADS1235::getInstance()->getRunningAverage() != NULL) {
    long val = ADS1235::getInstance()->readADCRaw();
    ADS1235::getInstance()->setLastADCValue(val);
    ADS1235::getInstance()->getRunningAverage()->addValue(val);
  }
}