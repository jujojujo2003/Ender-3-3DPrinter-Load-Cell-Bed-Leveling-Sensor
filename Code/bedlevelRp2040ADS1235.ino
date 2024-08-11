#include <SPI.h>
#include "PioSPI.h"
#include "ADS1235.h"
#include <Adafruit_NeoPixel.h>

//==================Config Variables========================
// Percentage of deviation allowed before probe trigger signal is sent
#define DELTA_FOR_TRIGGER 5

// Required target accuracy in mm for sensor
#define TARGET_ACCURACY 0.001 

// Probing speed in mm/min
#define PROBING_SPEED 30

// Number of samples to read to check if stable value is achieved
#define SAMPLES_FOR_STABILIZATION 10

// Relative percentage value before sample deemed stable. Value of one means wait for all samples to be ±1%
#define TOLERANCE_FOR_STABILIZATION 0.5

// How many ms to wait before fetching next sample
#define STABILIZATION_SAMPLING_INTERVAL_IN_MS 100

// ADS sampling speed
#define ADS_SPEED ADS1235::DataRate::SPS_7200

//===========================END============================

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

PioSPI spiBus(ADC_MOSI, ADC_MISO, ADC_SCLK, ADC_CS, SPI_MODE1, 20000000);
SPISettings spiSettings(20000000, MSBFIRST, SPI_MODE1);
Adafruit_NeoPixel led(1, LED_DIN, NEO_GRB + NEO_KHZ800);

// State variables
uint64_t ledEndTime = 0;
uint32_t ledColor = 0;
bool is_probing = false;
unsigned long interruptRisingEdgeTime = 0;
uint16_t numberOfSamplesForProbing = 1; 

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

double getNormalizedReadingFromADC() {
  return ADS1235::getInstance()->getRunningAverage()->getAverageLast(numberOfSamplesForProbing) * 100.0 / ADS1235::getInstance()->getRunningAverage()->getAverage();
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
  ADS1235::getInstance()->setDataRate(ADS_SPEED);
  ADS1235::getInstance()->setMode(ADS1235::ADCMode::NORMAL);
  ADS1235::getInstance()->caliberate();

  led.fill(led.Color(255, 0, 0));
  led.show();
  // Wait for buffer to fill
  while(!ADS1235::getInstance()->getRunningAverage()->bufferIsFull());
  led.fill(led.Color(255, 80, 0));
  led.show();

  // Verify settings
  if (ADS1235::getInstance()->getDataRate() != ADS_SPEED) {
    halt("Incorrect Datarate!");
  }

  if (ADS1235::getInstance()->getPositiveInputMux() != ADS1235::InputMux::AIN4) {
    halt("Incorrect Positive mux!");
  }

  if (ADS1235::getInstance()->getNegativeInputMux() != ADS1235::InputMux::AIN5) {
    halt("Incorrect Negative mux!");
  }

  if (ADS1235::getInstance()->getMode() != ADS1235::ADCMode::NORMAL) {
    halt("Incorrect MODE!");
  }

  // Make real time sampling data rate is roughtly same as what we set on initialization
  double dataRateRatio = (ADS1235::getInstance()->getRealDataRateAsInt()*1.0/ADS1235::getInstance()->getDataRateAsInt());
  if (dataRateRatio > 1.1 || dataRateRatio < 0.9) {
    halt("Realtime data rate not the same as what was set. Please check ADC connections");
  }

  // Samples to average for probing is number of samples that can fit in the time probe takes to travel TARGET_ACCURACY distance with PROBING_SPEED
  numberOfSamplesForProbing = ceil(ADS1235::getInstance()->getRealDataRateAsInt()*(TARGET_ACCURACY/(PROBING_SPEED/60.0)));
  if (numberOfSamplesForProbing < 1) {
    halt("numberOfSamplesForProbing < 1 . Your sensor is not capable of hitting the target accuracy given the probing speed. Try changing either of them");
  }

  // Setup Interupts
  attachInterrupt(CONTROL_PIN, handleControlPinChange, CHANGE);

  // Wait for data to stabilize
  // Fetch data at 100 ms intervals and wait for all 10 sample points to be ± 1 of avg value 
  double data[SAMPLES_FOR_STABILIZATION];
  uint8_t last_ptr = 0;
  while(last_ptr<SAMPLES_FOR_STABILIZATION){
    data[last_ptr] = getNormalizedReadingFromADC();
    delay(STABILIZATION_SAMPLING_INTERVAL_IN_MS);
    last_ptr++;
  }
  bool has_stabilized = true;
  do {
    has_stabilized = true;
    for(int i=0;i<SAMPLES_FOR_STABILIZATION;i++) {
      if (data[i] > (100+TOLERANCE_FOR_STABILIZATION) || data[i] < (100-TOLERANCE_FOR_STABILIZATION)) {
        // Data not stable
        has_stabilized = false;
        // get next value
        last_ptr = (last_ptr+1)%SAMPLES_FOR_STABILIZATION;
        data[last_ptr]  = getNormalizedReadingFromADC();
        led.fill(led.Color(255*((millis()/100)%2), 80*((millis()/100)%2), 0));
        led.show();
        delay(STABILIZATION_SAMPLING_INTERVAL_IN_MS);
        break;
      }
    }
  } while(!has_stabilized);
  led.fill(led.Color(0, 255, 0));
  led.show();
  delay(1000);
  led.fill(led.Color(0, 0, 0));
  led.show();
}

void loop() {
  updateLED();
  // Uncomment for debug logs
  /*if (micros()%100000 ==0) {
    Serial.print(ADS1235::getInstance()->getRealDataRateAsInt());
    Serial.print(" ");
    Serial.print(numberOfSamplesForProbing);
    Serial.print(" ");
    Serial.print(getNormalizedReadingFromADC());
    Serial.println();
  }*/
  if (is_probing) {
    double normalizedValue = getNormalizedReadingFromADC();
    if (normalizedValue <= (100 - DELTA_FOR_TRIGGER) || normalizedValue >= (100 + DELTA_FOR_TRIGGER)) {
      // Probing  done
      // Reset state variables
      is_probing = false;
      trigger();
    }
  }
}