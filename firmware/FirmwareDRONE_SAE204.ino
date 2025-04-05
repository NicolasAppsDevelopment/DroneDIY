#include "WiFi.h"
#include "AsyncUDP.h"
#include "string"
#include "FeatherShieldPinouts.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// GYROSCOPE VAR PART
Adafruit_BNO055 orientDevice = Adafruit_BNO055(55, 0x28, &Wire);
int currentRoll = 0; // in degres
int currentPitch = 0; // in degres
int currentHeading = 0; // in degres
int current_Z_Acceleration = 0; //in  m/s^2

// UDP SERVER VAR PART
const char* ssid = "Dronator2000";
const char* password = "123456789";
AsyncUDP udp;
#define PORT 1234
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// MOTOR PWM MANAGER VAR PART
// use firsts channels of 16 channels (started from zero)
#define RIGHT_MOTOR_CHANNEL 0
#define LEFT_MOTOR_CHANNEL 1
#define DOWN_MOTOR_CHANNEL 2
#define UP_MOTOR_CHANNEL 3

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

// motors PIN
#define RIGHT_MOTOR_PIN    D4
#define LEFT_MOTOR_PIN     A0
#define DOWN_MOTOR_PIN     D2
#define UP_MOTOR_PIN       A5

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

bool startWith(char* input, char* match) {
  if (strlen(input) < strlen(match)) {
    return false;
  }

  for (int i = 0; i < strlen(match); i++) {
    if (input[i] != match[i]) {
      return false;
    }
  }

  return true;
}

float x_LeftJoystick = 0.0;
float y_LeftJoystick = 0.0;
float x_RightJoystick = 0.0;
float y_RightJoystick = 0.0;
unsigned long lastTime;
void updateControls(char* data) {
  strtok(data, ";");
  x_LeftJoystick = atof(strtok(NULL, ";"));
  y_LeftJoystick = atof(strtok(NULL, ";"));
  x_RightJoystick = atof(strtok(NULL, ";"));
  y_RightJoystick = atof(strtok(NULL, ";"));
  lastTime = millis();
}

bool Start = false;
void updateMode(char* data) {
  strtok(data, ";");
  Start = (bool)atof(strtok(NULL, ";"));
}


void updatePos() {
  // get acceleration
  sensors_event_t orientationData,accelerometerData;
  orientDevice.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  
  currentRoll = (int)(orientationData.orientation.y);
  currentPitch = (int)(orientationData.orientation.z);
  currentHeading = (int)(orientationData.orientation.x);

  orientDevice.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  current_Z_Acceleration = (int)(accelerometerData.acceleration.z);
}

void setup() {
  Serial.begin(115200);

  /* Initialise the sensor */
  if (!orientDevice.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

  // Setup timer and attach timer to a motor pin
  ledcSetup(RIGHT_MOTOR_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(RIGHT_MOTOR_PIN, RIGHT_MOTOR_CHANNEL);
  ledcSetup(LEFT_MOTOR_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LEFT_MOTOR_PIN, LEFT_MOTOR_CHANNEL);
  ledcSetup(DOWN_MOTOR_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(DOWN_MOTOR_PIN, DOWN_MOTOR_CHANNEL);
  ledcSetup(UP_MOTOR_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(UP_MOTOR_PIN, UP_MOTOR_CHANNEL);

  // Setup WIFI Acces Point
  WiFi.setSleep(false);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // Setup UDP Server
  if (udp.listen(PORT)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(local_ip);
    Serial.print("UDP Listening on port: ");
    Serial.println(PORT);
    udp.onPacket([](AsyncUDPPacket packet) {
      // When a packet is received
      char* data = (char*)(packet.data());
      if (startWith(data, "JOY_DATA;")) {
        updateControls(data);
      }
      if (startWith(data, "SET_MODE;")) {
        updateMode(data);
      }
    });
  }
}

float deadZone = 0.25;

float forward = 0.0;
float backward = 0.0;
float right = 0.0;
float left = 0.0;
float up = 0.0;
float down = 0.0;
float right_turn = 0.0;
float left_turn = 0.0;
bool idle = false;

float motors_speed = 0.6;

float up_motor_speed_variation = 0.0;
float down_motor_speed_variation = 0.0;
float right_motor_speed_variation = 0.0;
float left_motor_speed_variation = 0.0;

void stabilize(int wantedRoll, int wantedPitch, int wantedHeading) {
  up_motor_speed_variation = 0.0;
  down_motor_speed_variation = 0.0;
  right_motor_speed_variation = 0.0;
  left_motor_speed_variation = 0.0;

  if (currentRoll > wantedRoll ) {
    // penche trop vers la gauche

    // intensité de correction
    float correct_intensity = abs(currentRoll - wantedRoll) / 30.0;

    left_motor_speed_variation = correct_intensity;
    right_motor_speed_variation = -correct_intensity;
  }
  if (currentRoll < wantedRoll ) {
    // penche trop vers la droite

    // intensité de correction
    float correct_intensity = abs(currentRoll - wantedRoll ) / 30.0;

    left_motor_speed_variation = -correct_intensity;
    right_motor_speed_variation = correct_intensity;
  }

  if (currentPitch > wantedPitch ) {
    // penche trop vers en avant

    // intensité de correction
    float correct_intensity = abs(currentPitch - wantedPitch ) / 30.0;

    up_motor_speed_variation = correct_intensity;
    down_motor_speed_variation = -correct_intensity;
  }
  if (currentPitch < wantedPitch ) {
    // penche trop en arrière

    // intensité de correction
    float correct_intensity = abs(currentPitch - wantedPitch ) / 30.0;

    up_motor_speed_variation = -correct_intensity;
    down_motor_speed_variation = correct_intensity;
  }

  ledcAnalogWrite(UP_MOTOR_CHANNEL, (int)(max((float)0, min(motors_speed + up_motor_speed_variation, (float)1)) * 255.0));
  ledcAnalogWrite(DOWN_MOTOR_CHANNEL, (int)(max((float)0, min(motors_speed + down_motor_speed_variation, (float)1)) * 255.0));
  ledcAnalogWrite(LEFT_MOTOR_CHANNEL, (int)(max((float)0, min(motors_speed + left_motor_speed_variation, (float)1)) * 255.0));
  ledcAnalogWrite(RIGHT_MOTOR_CHANNEL, (int)(max((float)0, min(motors_speed + right_motor_speed_variation, (float)1)) * 255.0));
}


unsigned long lastTimePrint = 0;
void loop() {
  // get data from gyroscope
  updatePos();


  forward = 0.0;
  backward = 0.0;
  right = 0.0;
  left = 0.0;
  up = 0.0;
  down = 0.0;
  right_turn = 0.0;
  left_turn = 0.0;
  idle = true;

  // in degres
  int wantedRoll = 0;
  int wantedPitch = 0;
  int wantedHeading = 0;

  // manage forward and backward
  if (y_LeftJoystick > deadZone) {
    // going forward
    forward = y_LeftJoystick;
    idle = false;

    wantedPitch = y_LeftJoystick * 30;
  }
  if (y_LeftJoystick < -deadZone) {
    // going backward
    backward = y_LeftJoystick;
    idle = false;

    wantedPitch = y_LeftJoystick * 30;
  }

  // manage left and right
  if (x_LeftJoystick > deadZone) {
    // going right
    right = x_LeftJoystick;
    idle = false;

    wantedRoll = -x_LeftJoystick * 30;
  }
  if (x_LeftJoystick < -deadZone) {
    // going left
    left = -x_LeftJoystick;
    idle = false;

    wantedRoll = -x_LeftJoystick * 30;
  }

  // manage up and dowm
  if (y_RightJoystick > deadZone) {
    // going up
    up = y_RightJoystick;
    idle = false;

    
  }
  if (y_RightJoystick < -deadZone) {
    // going down
    down = y_RightJoystick;
    idle = false;

    
  }

  // manage rotation
  if (x_RightJoystick > deadZone) {
    // turning right
    right_turn = x_RightJoystick;
    idle = false;


  }
  if (x_RightJoystick < -deadZone) {
    // turning left
    left_turn = x_RightJoystick;
    idle = false;


  }

  if (Start=false || abs(currentRoll) > 30 || abs(currentPitch) > 30){
    ledcAnalogWrite(UP_MOTOR_CHANNEL, 0);
    ledcAnalogWrite(DOWN_MOTOR_CHANNEL, 0);
    ledcAnalogWrite(LEFT_MOTOR_CHANNEL, 0);
    ledcAnalogWrite(RIGHT_MOTOR_CHANNEL, 0); 
  }else{// motor action
    stabilize(wantedRoll,wantedPitch,wantedHeading);
  }

  if (millis() - lastTime > 1000) {
    // TIMEOUT (after 1s of inactivity) : connection lost
    x_LeftJoystick = 0;
    y_LeftJoystick = 0;
    x_RightJoystick = 0;
    y_RightJoystick = 0;
  }

  delay(10);
}
