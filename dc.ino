#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

#include <SPI.h>
#include "pearson-hashing.h"

RF24 radio(9,10); // init module on pins 9 and 10 for Uno and Nano
byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};  //Possible pipes numbers

const int enablePin1 = 3;    // H-bridge motor 1 enable pin
const int motor1Pin1 = 4;    // H-bridge leg 1 (pin 2, 1A)
const int motor1Pin2 = 5;    // H-bridge leg 2 (pin 7, 2A)

const int enablePin2 = 6;   // H-bridge motor 2 enable pin
const int motor2Pin1 = 7;    // H-bridge leg 3 (pin 10, 1A)
const int motor2Pin2 = 8;    // H-bridge leg 4 (pin 15, 2A)

byte recieved_data[3];

void setup() {
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enablePin1, OUTPUT);
  pinMode(enablePin2, OUTPUT);

  Serial.begin(9600);
  
  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(0,15);
  radio.enableAckPayload();
  radio.setPayloadSize(32);

  radio.openReadingPipe(1,address[0]);
  radio.setChannel(0x60);

  radio.setPALevel (RF24_PA_MIN); //radio power level. Possible values: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //Bandwidth. RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //should be the same on TX/RX!
  // Warning!!! enableAckPayload is not working on RF24_250KBPS!
  
  radio.powerUp();
  radio.startListening();
}

void loop() {

  int x = 0;
  int y = 0;
  byte pipeNo;
  while( radio.available(&pipeNo)){
    radio.read(&recieved_data, sizeof(recieved_data));
//    if (!validate(recieved_data, sizeof(recieved_data))) {
//      continue;
//    }
    printData(recieved_data, sizeof(recieved_data));
    x = recieved_data[1];
    y = recieved_data[2];
    moveCar(x, y);
 }
}

void printData(byte data[], size_t arr_size) {
  for (int i = 0; i < arr_size; i++) {
    Serial.print(data[i], DEC); 
    Serial.print(" "); 
  }
  Serial.print("\n");
}

void printSpeed(int left, int right) {
  Serial.print("L: "); 
  Serial.print(left, DEC);
  Serial.print(" R: ");
  Serial.print(right, DEC);
  Serial.print("\n");
}

void printDirection(int dir) {
  if (dir < 0) {
      Serial.print("FORWARD");
  } else {
      Serial.print("BACKWARD");
  }
  Serial.print("\n");
}

bool validate(byte data[], size_t arr_size) {
  char x[arr_size];
  for (int i = 1; i<arr_size; i++) {
    x[i] = data[i];
  }
  unsigned char hash = phash_rfc3074(x);
  return byte(hash) == data[0];
}

void direct_motor(int pin1, int pin2, bool forward) {
    digitalWrite(pin1, forward ? LOW : HIGH);
    digitalWrite(pin2, forward ? HIGH : LOW);
}

void speed_motor(int pin, int value) {
    analogWrite(pin, value);
}

void moveCar(int x, int y) {
  const float turn_zone = 0.3;
  int x_dir = x - 127;
  int y_dir = y - 127;
  bool dir_1 = true;
  bool dir_2 = false;
  printDirection(y_dir);
  if (y_dir < 0) {
    dir_1 = true;
    dir_2 = false;
  } else {
    dir_1 = false;
    dir_2 = true;
  }
  int speed_val = abs(y_dir);
  int speed_mapped = map(speed_val, 0, 127, 0, 254);
  int direction_mapped = map(x_dir, -127, 127, -254, 254);
  if (abs(direction_mapped) < turn_zone * 254) {
    direction_mapped = 0;
  }
  int speed_L = speed_mapped;
  int speed_R = speed_mapped;

  if (speed_mapped < 254 * turn_zone) {
    speed_L = abs(direction_mapped);
    speed_R = abs(direction_mapped);
    dir_1 = direction_mapped < 0;
    dir_2 = direction_mapped < 0;
  } else {
    if (direction_mapped < 0) {
      speed_L -= abs(direction_mapped);
      speed_R = max(abs(direction_mapped), speed_R);
      if (speed_L < 0) { speed_L = 0; }
    } else if (direction_mapped > 0) {
      speed_R -= abs(direction_mapped);
      speed_L = max(abs(direction_mapped), speed_L);
      if (speed_R < 0) { speed_R = 0; }
    }
  }

  direct_motor(motor1Pin1, motor1Pin2, dir_1);
  direct_motor(motor2Pin1, motor2Pin2, dir_2);
  speed_motor(enablePin1, speed_L);
  speed_motor(enablePin2, speed_R);
  printSpeed(speed_L, speed_R);
}

