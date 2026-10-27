/*
  Joystick Interface  (analog thumb joystick -> normalised X/Y -> differential drive)

  - Auto-calibrates the stick centre at power-up (do not touch the stick for 1 s).
  - Dead-zone removes drift around the centre.
  - Output is -100..100 on each axis, printed on Serial and mixed into left/right motor speeds.
  - Joystick push-button toggles motors enabled/disabled (safety switch).
  Set DRIVE_MOTORS to false to use it as a pure Serial input device.
*/

// ---------- Pins ----------
const uint8_t PIN_X  = A0;
const uint8_t PIN_Y  = A1;
const uint8_t PIN_SW = 2;          // joystick button (to GND, internal pull-up)

const uint8_t ENA = 5, IN1 = 6, IN2 = 7;     // left motor
const uint8_t IN3 = 8, IN4 = 9, ENB = 10;    // right motor

// ---------- Settings ----------
const bool DRIVE_MOTORS = true;
const bool INVERT_X = false;
const bool INVERT_Y = false;       // flip if "forward" drives backwards
const int  DEADZONE = 40;          // ADC counts around centre

int centerX = 512, centerY = 512;
bool motorsEnabled = false;        // start disabled; press the stick to arm

int readAveraged(uint8_t pin) {
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(pin);
  return sum / 8;
}

// Returns -100..100 with dead-zone and per-side scaling
int readAxis(uint8_t pin, int center, bool invert) {
  int v = readAveraged(pin) - center;
  int out = 0;
  if (v > DEADZONE)       out = map(v,  DEADZONE, 1023 - center,  0,  100);
  else if (v < -DEADZONE) out = map(v, -DEADZONE,      -center,   0, -100);
  out = constrain(out, -100, 100);
  return invert ? -out : out;
}

void setMotor(uint8_t en, uint8_t inA, uint8_t inB, int speed) {
  speed = constrain(speed, -255, 255);
  digitalWrite(inA, speed > 0);
  digitalWrite(inB, speed < 0);
  analogWrite(en, abs(speed));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  setMotor(ENA, IN1, IN2, 0);
  setMotor(ENB, IN3, IN4, 0);

  delay(1000);                     // let the stick settle at centre
  centerX = readAveraged(PIN_X);
  centerY = readAveraged(PIN_Y);
  Serial.print(F("Calibrated centre  X=")); Serial.print(centerX);
  Serial.print(F("  Y="));                   Serial.println(centerY);
  Serial.println(F("Press the stick button to arm/disarm the motors."));
}

void loop() {
  // --- debounced button toggle ---
  static bool lastBtn = HIGH;
  static unsigned long lastBtnMs = 0;
  bool btn = digitalRead(PIN_SW);
  if (btn != lastBtn && millis() - lastBtnMs > 50) {
    lastBtnMs = millis();
    lastBtn = btn;
    if (btn == LOW) {
      motorsEnabled = !motorsEnabled;
      Serial.println(motorsEnabled ? F("MOTORS ARMED") : F("MOTORS DISARMED"));
    }
  }

  int x = readAxis(PIN_X, centerX, INVERT_X);   // turn
  int y = readAxis(PIN_Y, centerY, INVERT_Y);   // throttle

  // arcade mix
  int left  = constrain(y + x, -100, 100);
  int right = constrain(y - x, -100, 100);

  if (DRIVE_MOTORS) {
    if (motorsEnabled) {
      setMotor(ENA, IN1, IN2, map(left,  -100, 100, -255, 255));
      setMotor(ENB, IN3, IN4, map(right, -100, 100, -255, 255));
    } else {
      setMotor(ENA, IN1, IN2, 0);
      setMotor(ENB, IN3, IN4, 0);
    }
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    Serial.print(F("X:")); Serial.print(x);
    Serial.print(F("  Y:")); Serial.print(y);
    Serial.print(F("  L:")); Serial.print(left);
    Serial.print(F("  R:")); Serial.println(right);
  }
  delay(5);
}
