#ifndef WEB_STREAM_MODULE_H
#define WEB_STREAM_MODULE_H

#include "esp_http_server.h"
#include "CameraModule.h"

class WebStream {
private:
  static httpd_handle_t stream_httpd;

  static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    if (res != ESP_OK) return res;

    while (true) {
      fb = Camera::captureFrame();
      if (!fb) {
        Serial.println("Camera capture failed");
        res = ESP_FAIL;
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
      if (res == ESP_OK) {
        size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
        res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      }
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
      }
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 12);
      }
      if (fb) {
        Camera::releaseFrame(fb);
        fb = NULL;
        _jpg_buf = NULL;
      } else if (_jpg_buf) {
        free(_jpg_buf);
        _jpg_buf = NULL;
      }
      if (res != ESP_OK) break;
    }
    return res;
  }

  static esp_err_t still_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    fb = Camera::captureFrame();
    if (!fb) {
      Serial.println("Camera capture failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_send(req, (const char *)fb->buf, fb->len);
    Camera::releaseFrame(fb);
    return res;
  }

public:
  static void startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t stream_uri = { .uri = "/", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
    httpd_uri_t still_uri = { .uri = "/still", .method = HTTP_GET, .handler = still_handler, .user_ctx = NULL };

    stream_httpd = NULL;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
      httpd_register_uri_handler(stream_httpd, &stream_uri);
      httpd_register_uri_handler(stream_httpd, &still_uri);
    }
  }
  static void handleClient() { }
};

httpd_handle_t WebStream::stream_httpd = NULL;
#endif