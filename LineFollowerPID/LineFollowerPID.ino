/*
  Line Follower with PID  (Arduino Uno/Nano + 5-channel analog IR array + L298N)

  1. Power on. The LED lights for 3 s: slide the sensor array back and forth across the line
     so every sensor sees both the line and the floor (calibration).
  2. Robot then follows the line using a PID controller on the weighted line position.

  Tuning order: set Ki = Kd = 0, raise Kp until it follows but wobbles,
  add Kd to damp the wobble, add a tiny Ki only if it drifts on long curves.
*/

// ---------- Pins ----------
const uint8_t NUM_SENSORS = 5;
const uint8_t sensorPins[NUM_SENSORS] = {A0, A1, A2, A3, A4};   // left -> right

const uint8_t ENA = 5, IN1 = 6, IN2 = 7;      // left motor
const uint8_t IN3 = 8, IN4 = 9, ENB = 10;     // right motor
const uint8_t LED = 13;

// ---------- Settings ----------
const bool SENSOR_HIGH_ON_LINE = false;   // true if the reading goes UP over the line
const int  BASE_SPEED = 150;              // 0..255
const int  MAX_SPEED  = 255;
const int  LOOP_MS    = 5;

float Kp = 0.05;     // error is -2000..2000, so 0.05 -> up to +-100 PWM
float Ki = 0.0;
float Kd = 0.25;
const float INTEGRAL_LIMIT = 3000;

int sMin[NUM_SENSORS], sMax[NUM_SENSORS];
int lastPosition = 0;
float integral = 0;
int lastError = 0;

void setMotor(uint8_t en, uint8_t inA, uint8_t inB, int speed) {
  speed = constrain(speed, -MAX_SPEED, MAX_SPEED);
  digitalWrite(inA, speed > 0);
  digitalWrite(inB, speed < 0);
  analogWrite(en, abs(speed));
}

void drive(int left, int right) {
  setMotor(ENA, IN1, IN2, left);
  setMotor(ENB, IN3, IN4, right);
}

void calibrate() {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) { sMin[i] = 1023; sMax[i] = 0; }
  digitalWrite(LED, HIGH);
  unsigned long start = millis();
  while (millis() - start < 3000) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
      int v = analogRead(sensorPins[i]);
      if (v < sMin[i]) sMin[i] = v;
      if (v > sMax[i]) sMax[i] = v;
    }
  }
  digitalWrite(LED, LOW);
}

// Returns -2000 (line far left) .. +2000 (line far right); 0 = centred.
int readPosition() {
  long weighted = 0, sum = 0;
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    int raw = analogRead(sensorPins[i]);
    int norm = 0;
    if (sMax[i] - sMin[i] > 20) {
      norm = constrain(map(raw, sMin[i], sMax[i], 0, 1000), 0, 1000);
    }
    if (!SENSOR_HIGH_ON_LINE) norm = 1000 - norm;   // make 1000 = "on the line"
    if (sMax[i] - sMin[i] <= 20) norm = 0;          // dead / uncalibrated sensor
    weighted += (long)norm * ((long)i * 1000 - 2000);
    sum      += norm;
  }
  if (sum < 200) {                                  // line lost: keep turning the last way
    return lastPosition > 0 ? 2000 : -2000;
  }
  lastPosition = weighted / sum;
  return lastPosition;
}

void setup() {
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(LED, OUTPUT);
  drive(0, 0);
  Serial.begin(115200);
  calibrate();
  delay(1000);
}

void loop() {
  static unsigned long lastRun = 0;
  if (millis() - lastRun < LOOP_MS) return;
  lastRun = millis();

  int error = readPosition();               // setpoint = 0 (line centred)
  integral = constrain(integral + error, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);
  int derivative = error - lastError;
  lastError = error;

  float correction = Kp * error + Ki * integral + Kd * derivative;

  // positive error = line on the right -> steer right (left wheel faster)
  int leftSpeed  = BASE_SPEED + (int)correction;
  int rightSpeed = BASE_SPEED - (int)correction;
  drive(leftSpeed, rightSpeed);

  // Uncomment while tuning:
  // Serial.print(error); Serial.print('\t'); Serial.println(correction);
}
