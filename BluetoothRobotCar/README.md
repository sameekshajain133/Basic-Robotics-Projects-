# Bluetooth Robot Car
Arduino Uno/Nano + HC-05 + L298N + 2 DC motors.

| Part | Arduino pin |
|---|---|
| HC-05 TX | D2 |
| HC-05 RX | D3 (through 1k/2k divider) |
| L298N ENA / IN1 / IN2 | D5 / D6 / D7 |
| L298N IN3 / IN4 / ENB | D8 / D9 / D10 |

Pair the HC-05 (PIN 1234 or 0000), open a Bluetooth RC-car app at 9600 baud and drive.
Commands: F B L R G I H J S, digits 0-9 for speed, q for full speed.
Common GND between battery, L298N and Arduino is required.
