/*
  ESP32-CAM Image System (AI-Thinker board)
    http://<ip>/          simple web page with live video
    http://<ip>/capture   single JPEG still
    http://<ip>:81/stream MJPEG live stream
    http://<ip>/led?on=1  flash LED on (on=0 off)

  Arduino IDE: Board = "AI Thinker ESP32-CAM", Partition = "Huge APP".
  Upload with GPIO0 connected to GND, then remove it and press RESET.
  Power the board from a solid 5 V supply (weak USB power causes brown-outs).
*/
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ---- AI-Thinker camera pins ----
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22
#define FLASH_LED_PIN    4

static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frameboundary";
static const char* STREAM_BOUNDARY     = "\r\n--frameboundary\r\n";
static const char* STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM</title>
<style>body{font-family:sans-serif;text-align:center;background:#111;color:#eee}
img{max-width:100%;border-radius:8px}a{color:#6cf;margin:0 8px}</style></head>
<body><h2>ESP32-CAM Image System</h2>
<img id="v" alt="live stream"><p>
<a href="/capture" target="_blank">Capture still</a>
<a href="/led?on=1" target="hidden">Flash ON</a>
<a href="/led?on=0" target="hidden">Flash OFF</a></p>
<iframe name="hidden" style="display:none"></iframe>
<script>document.getElementById('v').src='http://'+location.hostname+':81/stream';</script>
</body></html>
)rawliteral";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static esp_err_t index_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t capture_handler(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

static esp_err_t stream_handler(httpd_req_t* req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char part[64];
  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }

    size_t hlen = snprintf(part, sizeof(part), STREAM_PART, (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;          // client disconnected
  }
  return res;
}

static esp_err_t led_handler(httpd_req_t* req) {
  char query[32], val[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "on", val, sizeof(val)) == ESP_OK) {
    digitalWrite(FLASH_LED_PIN, val[0] == '1' ? HIGH : LOW);
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

void startServers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri   = { "/",        HTTP_GET, index_handler,   NULL };
  httpd_uri_t capture_uri = { "/capture", HTTP_GET, capture_handler, NULL };
  httpd_uri_t led_uri     = { "/led",     HTTP_GET, led_handler,     NULL };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &led_uri);
  }

  config.server_port = 81;             // separate server so streaming never blocks the page
  config.ctrl_port  += 1;
  httpd_uri_t stream_uri = { "/stream", HTTP_GET, stream_handler, NULL };
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk = XCLK_GPIO_NUM;
  cfg.pin_pclk = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM;
  cfg.pin_href = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn = PWDN_GPIO_NUM;
  cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_VGA;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 2;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    cfg.frame_size   = FRAMESIZE_QVGA;
    cfg.jpeg_quality = 15;
    cfg.fb_count     = 1;
    cfg.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print('.'); }
  Serial.print("\nOpen http://"); Serial.println(WiFi.localIP());

  startServers();
}

void loop() {
  delay(10000);                        // servers run in their own tasks
}
