# Line Follower (PID)
Arduino + 5 analog IR sensors (A0-A4, left to right) + L298N (D5-D10).

* On power-up, swipe the sensors across the line for 3 s (LED on) to calibrate.
* `SENSOR_HIGH_ON_LINE`: set `true` if your module's value rises over the line.
* Start with `Kp=0.05, Ki=0, Kd=0.25`, then tune: Kp first, then Kd, then a tiny Ki.
* If the robot steers away from the line, swap the two motors' wires or negate the sign in `loop()`.
