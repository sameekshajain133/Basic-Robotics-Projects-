/*
  Bluetooth Robot Car  (Arduino Uno/Nano + HC-05 + L298N)

  Control from any "Bluetooth RC Car" style phone app (9600 baud, single-character commands):
    F forward   B backward   L spin left   R spin right
    G fwd-left  I fwd-right  H back-left   J back-right
    S stop      0..9 speed   q full speed
*/
#include <SoftwareSerial.h>

// ---------- Pins (change to match your wiring) ----------
const uint8_t BT_RX = 2;   // Arduino RX  <- HC-05 TX
const uint8_t BT_TX = 3;   // Arduino TX  -> HC-05 RX (use a 1k/2k voltage divider)

const uint8_t ENA = 5;     // Left motor speed (PWM)
const uint8_t IN1 = 6;
const uint8_t IN2 = 7;
const uint8_t IN3 = 8;     // Right motor
const uint8_t IN4 = 9;
const uint8_t ENB = 10;    // Right motor speed (PWM)

// If > 0, the car stops when no command arrives for this many ms.
// Use ~500 only if your app keeps sending while a button is held.
const unsigned long FAILSAFE_MS = 0;

SoftwareSerial bt(BT_RX, BT_TX);
int speedPwm = 180;                 // 0..255
unsigned long lastCmdMs = 0;

void setMotor(uint8_t en, uint8_t inA, uint8_t inB, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0)      { digitalWrite(inA, HIGH); digitalWrite(inB, LOW); }
  else if (speed < 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else                { digitalWrite(inA, LOW);  digitalWrite(inB, LOW); }
  analogWrite(en, abs(speed));
}

void drive(int left, int right) {
  setMotor(ENA, IN1, IN2, left);
  setMotor(ENB, IN3, IN4, right);
}

void handleCommand(char c) {
  const int s = speedPwm;
  switch (c) {
    case 'F': drive( s,      s);      break;
    case 'B': drive(-s,     -s);      break;
    case 'L': drive(-s,      s);      break;   // spin on the spot
    case 'R': drive( s,     -s);      break;
    case 'G': drive( s / 2,  s);      break;   // forward-left
    case 'I': drive( s,      s / 2);  break;   // forward-right
    case 'H': drive(-s / 2, -s);      break;   // back-left
    case 'J': drive(-s,     -s / 2);  break;   // back-right
    case 'S': drive(0, 0);            break;
    case 'q': speedPwm = 255;         break;
    default:
      if (c >= '0' && c <= '9') speedPwm = map(c - '0', 0, 9, 100, 255);
      break;
  }
}

void setup() {
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  drive(0, 0);
  Serial.begin(9600);
  bt.begin(9600);                    // HC-05 default data baud rate
  Serial.println(F("Bluetooth car ready"));
}

void loop() {
  while (bt.available()) {
    char c = bt.read();
    if (c == '\r' || c == '\n') continue;
    handleCommand(c);
    lastCmdMs = millis();
    Serial.print(F("cmd: ")); Serial.println(c);
  }
  if (FAILSAFE_MS > 0 && millis() - lastCmdMs > FAILSAFE_MS) drive(0, 0);
}
