# Smart Blind Stick
Ultrasonic obstacle detection with buzzer + vibration feedback.

| Part | Arduino pin |
|---|---|
| HC-SR04 TRIG / ECHO | D9 / D10 |
| Buzzer | D8 |
| Vibration motor (via NPN transistor + flyback diode) | D6 |
| Locate button | D2 -> GND |

Zones: >150 cm silent, 100-150 slow beep, 50-100 fast beep, <50 continuous + vibration.
Adjust `NEAR_CM`, `MID_CM`, `FAR_CM` to taste.
