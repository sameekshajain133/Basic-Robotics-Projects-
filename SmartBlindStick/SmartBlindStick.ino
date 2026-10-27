/*
  Smart Blind Stick  (Arduino Nano/Uno + HC-SR04 + buzzer + vibration motor)

  Distance zones:
    > 150 cm : silent
    100-150  : slow beeps
    50-100   : fast beeps
    < 50 cm  : continuous tone + strong vibration
  A median filter over 5 readings removes ultrasonic spikes.
  Optional "find my stick" button: press to sound the buzzer.
*/

// ---------- Pins ----------
const uint8_t TRIG   = 9;
const uint8_t ECHO   = 10;
const uint8_t BUZZER = 8;     // active or passive buzzer
const uint8_t VIB    = 6;     // vibration motor via NPN transistor (PWM pin)
const uint8_t BTN    = 2;     // optional locate button to GND

// ---------- Distance thresholds (cm) ----------
const float NEAR_CM = 50;
const float MID_CM  = 100;
const float FAR_CM  = 150;

const unsigned int TONE_HZ = 2000;

// Single ultrasonic reading in cm, or -1 if no echo
float readCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long us = pulseIn(ECHO, HIGH, 25000UL);   // ~4 m timeout
  if (us == 0) return -1;
  return us * 0.0343f / 2.0f;
}

// Median of 5 valid readings; returns 999 if nothing detected
float distanceCm() {
  float v[5];
  uint8_t n = 0;
  for (uint8_t i = 0; i < 5; i++) {
    float d = readCm();
    if (d > 2 && d < 400) v[n++] = d;
    delay(8);
  }
  if (n == 0) return 999;
  for (uint8_t i = 1; i < n; i++) {                  // insertion sort
    float key = v[i]; int j = i - 1;
    while (j >= 0 && v[j] > key) { v[j + 1] = v[j]; j--; }
    v[j + 1] = key;
  }
  return v[n / 2];
}

void alertOff() { noTone(BUZZER); analogWrite(VIB, 0); }
void alertOn(uint8_t vibStrength) { tone(BUZZER, TONE_HZ); analogWrite(VIB, vibStrength); }

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(VIB, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  alertOff();
  Serial.begin(9600);
}

void loop() {
  static unsigned long lastToggle = 0;
  static bool beepOn = false;

  // Locate button overrides everything while held
  if (digitalRead(BTN) == LOW) {
    alertOn(0);
    return;
  }

  float d = distanceCm();
  Serial.println(d);

  if (d <= NEAR_CM) {
    alertOn(255);                         // continuous
    beepOn = true;
  } else if (d <= FAR_CM) {
    unsigned long interval = (d <= MID_CM) ? 150 : 400;
    if (millis() - lastToggle >= interval) {
      lastToggle = millis();
      beepOn = !beepOn;
    }
    if (beepOn) alertOn(160); else alertOff();
  } else {
    alertOff();
    beepOn = false;
  }
}
