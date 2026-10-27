# Joystick Interface
Analog joystick -> calibrated, dead-zoned X/Y (-100..100) -> arcade-mix differential drive.

| Joystick | Arduino |
|---|---|
| VRx / VRy | A0 / A1 |
| SW | D2 (to GND when pressed) |
| +5V / GND | 5V / GND |

Motor driver wiring is the same as the Bluetooth car (D5-D10). Motors start **disarmed**; press the stick to arm.
Set `DRIVE_MOTORS = false` to use it only as a Serial input.
