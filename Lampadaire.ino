#include <IRremote.hpp>

#define TOGGLE 0xFFA857
#define OFF 0xFF9867
#define ON 0xFF02FD

IRsend irsend(3);

int partition[] = {
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
};

size_t size = 11;
int intensity = 0;

void setIR(size_t value) {
  irsend.sendNEC(0xFFA857, 32);
}

void setup() {
  Serial.begin(9600);
  delay(2000);
  setIR(ON);
  for(size_t i(0); i<size; ++i) {
    int ms = partition[i];
    delay(ms);
    if (i%2 == 0) setIR(ON);
    else setIR(OFF);
  }
}

void flicker(int on1, int on2, int off1, int off2) {
  setIR(ON);
  delay(random(on1, on2));
  setIR(OFF);
  delay(random(off1, off2));
}
  
void flicker_low() {
  flicker(1000, 6000, 500, 1000);
}

void flicker_medium() {
  flicker(500, 1500, 200, 800);
}

void flicker_high() {
  flicker(60, 400, 60, 400);
}

void loop() {
  int intensity=random(0, 3);
  int max;
  Serial.println(intensity);
  switch(intensity) {
    case 0:
      flicker_low();
      break;
    case 1:
      max = random(1, 3);
      for(int j(0); j<max; ++j) flicker_medium();
      break;
    case 2:
      max = random(1, 6);
      for(int j(0); j<max; ++j) flicker_high();
      break;
  }
}
