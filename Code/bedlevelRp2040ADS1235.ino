#include <SPI.h>
#include "PioSPI.h"
#include "ADS1235.h"
#include <Adafruit_NeoPixel.h>

/* Config Variables */
// Percentage of deviation allowed before probe trigger signal is sent
#define DELTA_FOR_TRIGGER 90

// ADC Pins
#define ADC_MISO 6
#define ADC_NOT_DRDY 7
#define ADC_MOSI 8
#define ADC_SCLK 9
#define ADC_CS 10
#define ADC_START 11
#define ADC_NOT_RESET 12
#define ADC_NOT_PWDN 13

// MCU PINS
#define LED_DIN 18
#define CONTROL_PIN 2
#define OUTPUT_PIN 1

PioSPI spiBus(ADC_MOSI, ADC_MISO, ADC_SCLK, ADC_CS, SPI_MODE1, 8000000);
SPISettings spiSettings(20000000, MSBFIRST, SPI_MODE1);
Adafruit_NeoPixel led(1, LED_DIN, NEO_GRB + NEO_KHZ800);

// State variables
uint64_t ledEndTime = 0;
uint32_t ledColor = 0;
bool is_probing = false;
unsigned long interruptRisingEdgeTime = 0;

void halt(String reason) {
  unsigned long int start = millis();
  while ((millis()-start)<=2000) {
    if (micros() % 1000 == 0) {
      Serial.print("Halted: ");
      Serial.println(reason);
    }
    if ((millis() / 100) % 2 == 0) {
      led.fill(led.Color(255, 0, 0));
    } else {
      led.fill(led.Color(0, 0, 0));
    }
    led.show();
  }

  ADS1235::getInstance()->reset();
  NVIC_SystemReset();
}

void handleControlPinChange() {
  if (is_probing) {
    // If already probing, don't accept any more command
    return;
  }
  int input = digitalRead(CONTROL_PIN);
  if (input == HIGH) {
    interruptRisingEdgeTime = micros();
  } else {
    unsigned long int delta = micros() - interruptRisingEdgeTime;
    interruptRisingEdgeTime = 0;
    // BLTouch
    // https://youprintin3d.de/media/pdf/54/75/b4/f5a1c8_77037f55e5d542309d9fc178165c9f3f.pdf
    // Pin Down dutycycle 650us ±20 as per spec
    if (delta >= 620 && delta <= 680) {
      is_probing = true;
    }
  }
}

void updateLED() {
  if (is_probing) {
    led.fill(led.Color(0, 0, 255));
    led.show();
  } else if (ledEndTime == 0) {
    // Nothing to  do
  } else if (millis() < ledEndTime) {
    // Show color
    led.fill(ledColor);
    led.show();
  } else {
    // Clear LED reset state
    ledEndTime = 0;
    led.clear();
    led.show();
  }
}

void setLEDAndClearAfter(uint64_t duration_millis, uint32_t color) {
  ledEndTime = millis() + duration_millis;
  ledColor = color;
  updateLED();
}

void trigger() {
  digitalWrite(OUTPUT_PIN, HIGH);
  setLEDAndClearAfter(1000, led.Color(0, 255, 0));
  Serial.println("Triggered");
  delay(10);
  digitalWrite(OUTPUT_PIN, LOW);
}


void setup() {
  Serial.begin(115200);
  Serial.println("Inititalizing");
  
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
  pinMode(CONTROL_PIN, INPUT_PULLUP);

  led.fill(led.Color(0, 0, 0));
  led.show();

  spiBus.begin();
  ADS1235::createInstance(spiBus, spiSettings, ADC_NOT_RESET, ADC_NOT_PWDN, ADC_START, ADC_NOT_DRDY);
  ADS1235::getInstance()->init();
  ADS1235::getInstance()->setInputMux(ADS1235::InputMux::AIN4, ADS1235::InputMux::AIN5);
  ADS1235::getInstance()->setDataRate(ADS1235::DataRate::SPS_7200);
  ADS1235::getInstance()->setMode(ADS1235::ADCMode::CHOP);
  ADS1235::getInstance()->caliberate();

  led.fill(led.Color(255, 0, 0));
  led.show();
  // Wait for buffer to fill
  while(!ADS1235::getInstance()->getRunningAverage()->bufferIsFull());
  led.fill(led.Color(0, 0, 0));
  led.show();

  // Verify settings
  if (ADS1235::getInstance()->getDataRate() != ADS1235::DataRate::SPS_7200) {
    halt("Incorrect Datarate!");
  }

  if (ADS1235::getInstance()->getPositiveInputMux() != ADS1235::InputMux::AIN4) {
    halt("Incorrect Positive mux!");
  }

  if (ADS1235::getInstance()->getNegativeInputMux() != ADS1235::InputMux::AIN5) {
    halt("Incorrect Negative mux!");
  }

  if (ADS1235::getInstance()->getMode() != ADS1235::ADCMode::CHOP) {
    halt("Incorrect MODE!");
  }

  // Setup Interupts
  attachInterrupt(CONTROL_PIN, handleControlPinChange, CHANGE);
}

void loop() {
  updateLED();
  if (is_probing) {
    double normalizedValue = ADS1235::getInstance()->getLastADCValue() * 100.0 / ADS1235::getInstance()->getRunningAverage()->getFastAverage();
    if (normalizedValue <= (100 - DELTA_FOR_TRIGGER) || normalizedValue >= (100 + DELTA_FOR_TRIGGER)) {
      // Probing  done
      // Reset state variables
      is_probing = false;
      trigger();
    }
  }
}