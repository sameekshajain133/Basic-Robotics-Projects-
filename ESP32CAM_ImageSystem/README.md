# ESP32-CAM Image System
AI-Thinker ESP32-CAM that serves a live MJPEG stream and still captures over WiFi.

1. Enter your WiFi name/password at the top of the sketch.
2. Board: **AI Thinker ESP32-CAM**, Partition Scheme: **Huge APP**.
3. Upload with an FTDI adapter: GPIO0 -> GND while uploading, then remove it and press RESET.
4. Open Serial Monitor (115200), read the IP, open `http://<ip>/` in a browser on the same network.

Endpoints: `/` page, `/capture` JPEG, `:81/stream` live video, `/led?on=1|0` flash LED.
Use a stable 5 V supply (>=500 mA) - weak power is the #1 cause of resets.
