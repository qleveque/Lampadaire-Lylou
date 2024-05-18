#include <WiFi.h>
#include <HTTPClient.h>
#include <HX711_ADC.h>

const int LOOP_TIME = 200;

const int STANDBY_INTENSITY = 20;
const char* SSID = "ShellyVintage-6F6A99";
const char* PASSWORD = "";
const String SERVER_NAME = "http://192.168.33.1/light/";

const int HX711_dout = 5;
const int HX711_sck = 6;
const float WEIGHT_THRESHOLD = 800.0;
const int PARTITION[] = {
  8000, 800, 600, 500, 400, 150, 100, 160, 100, 80, 150, 150
};

// Lamp
const size_t PARTITION_SIZE = (sizeof(PARTITION) / sizeof(PARTITION[0]));
HTTPClient http;

void connect_to_bulb() {
  Serial.println("Connecting to lamp");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  http.setTimeout(100);
}

void set_bulb_intensity(int intensity) {
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi Disconnected");
    connect_to_bulb();
  }
  const String server_path = SERVER_NAME + "0?turn=on&brightness=" + String(intensity);
  http.begin(server_path.c_str());
  int response_code = http.GET();
  if (response_code>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(response_code);
    String payload = http.getString();
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(response_code);
  }
  http.end();
}

// Weight sensors
HX711_ADC LoadCell(HX711_dout, HX711_sck);
volatile boolean new_weight_data;
void dataReadyISR() {
  if (LoadCell.update()) {
    new_weight_data = 1;
  }
}

void init_weight_sensors() {
  LoadCell.begin();
  LoadCell.start(2000, true);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(696.0);
    Serial.println("Weights ready");
  }
  attachInterrupt(digitalPinToInterrupt(HX711_dout), dataReadyISR, FALLING);
}

int partition_pointer, wait_for, flicker_pointer, flicker_intensity;
boolean someone_is_sitting = false;

void initialize_state() {
  Serial.println(">Nobody is sitting");
  set_bulb_intensity(STANDBY_INTENSITY);
  wait_for = 0;
  partition_pointer = 0;
  flicker_pointer = 0;
  flicker_intensity = 0;
}

void setup() {
  Serial.begin(115200); 
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  init_weight_sensors();
  WiFi.begin(SSID, PASSWORD);
  connect_to_bulb();
  initialize_state();

  delay(1000);
  Serial.println("Startup is complete");
}

void loop() {
  const boolean someone_was_sitting = someone_is_sitting;

  if (new_weight_data) {
    new_weight_data = 0;
    const float weight = LoadCell.getData();
    someone_is_sitting = weight > WEIGHT_THRESHOLD;
  }
  
  if (!someone_is_sitting) {
    if (someone_was_sitting) {
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
    const boolean on = partition_pointer % 2 == 0;
    set_bulb_intensity(on ? 100: 0);
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
  set_bulb_intensity(on ? 100: 0);
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
