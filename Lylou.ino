#include <HX711_ADC.h>

#include <IRremote.hpp>

#define TOGGLE 0xFFA857
#define OFF 0xFF9867
#define ON 0xFF02FD

const int LOOP_TIME = 200;

// weights
const int HX711_dout = 3;
const int HX711_sck = 5;
const int IR_dat = 9;

HX711_ADC LoadCell(HX711_dout, HX711_sck);
volatile boolean newDataReady;
void dataReadyISR() {
  if (LoadCell.update()) {
    newDataReady = 1;
  }
}
const float WEIGHT_THRESHOLD = 1000.0;

// lamp
IRsend irsend(IR_dat);
const int PARTITION[] = {
  8000,
  800,
  600,
  500,
  400,
  150,
  100,
  160,
  100,
  80,
  150,
  150,
};
const size_t PARTITION_SIZE = (sizeof(PARTITION) / sizeof(PARTITION[0]));
void set_IR(boolean on, int rep=2) {
  if(on) {
    Serial.println("lamp on");
    for(size_t i(0); i<rep; ++i)
      irsend.sendNEC(ON, 32);
  } else {
    Serial.println("lamp off");
    for(size_t i(0); i<rep; ++i)
      irsend.sendNEC(OFF, 32);
  }
}

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  // weights
  LoadCell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    float calibrationValue = 696.0;
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Weights ready");
  }
  attachInterrupt(digitalPinToInterrupt(HX711_dout), dataReadyISR, FALLING);

  // lamp
  set_IR(false);
  delay(2000);

  Serial.println("Startup is complete");
}

int partition_pointer = 0;
int wait_for = 0;
int flicker_pointer = 0;
int flicker_intensity = 0;
boolean someoneIsSitting = false;

void initialize_state() {
  Serial.println(">Nobody is sitting");
  set_IR(false, 3);
  wait_for = 0;
  partition_pointer = 0;
  flicker_pointer = 0;
}

void loop() {
  const boolean someoneWasSitting = someoneIsSitting;

  if (newDataReady) {
    newDataReady = 0;
    const float weight = LoadCell.getData();
    someoneIsSitting = weight > WEIGHT_THRESHOLD;
  }
  
  if (!someoneIsSitting) {
    if (someoneWasSitting) {
      initialize_state();
    }
    delay(LOOP_TIME);
    return;
  }

  if(wait_for > 0) {
    const int t = min(wait_for, LOOP_TIME);
    delay(t);
    wait_for -= t;
    return;
  }

  if(partition_pointer < PARTITION_SIZE) {
    Serial.print(">Partition: ");
    Serial.println(partition_pointer);
    set_IR(partition_pointer%2 == 0);
    wait_for = PARTITION[partition_pointer];
    ++partition_pointer;
    return;
  }

  if(flicker_pointer == 0) {
    flicker_intensity=random(0, 3);
    switch(flicker_intensity) {
      case 0:
        Serial.println(">Low intensity");
        flicker_pointer = 2;
        break;
      case 1:
        Serial.println(">Medium intensity");
        flicker_pointer = 2 * random(1, 3);
        break;
      case 2:
        Serial.println(">High intensity");
        flicker_pointer = 2 * random(1, 6);
        break;
    }
  }

  Serial.print("flicker_pointer: ");
  Serial.println(flicker_pointer);
  const boolean on = flicker_pointer % 2 == 0;
  --flicker_pointer;
  set_IR(on);
  switch(flicker_intensity) {
    case 0:
      wait_for = on ? random(1000, 6000): random(500, 1000);
      break;
    case 1:
      wait_for = on ? random(500, 1500): random(200, 800);
      break;
    case 2:
      wait_for = on ? random(60, 400): random(60, 400);
      break;
  }
}
