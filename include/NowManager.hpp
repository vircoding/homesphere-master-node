#pragma once

#include <esp_now.h>

class NowManager {
 public:
  struct TemperatureData {
    float hum;
    float temp;
    int node_id;
  };

  bool init();
  void onSend(esp_now_send_cb_t callback);
  void onReceived(esp_now_recv_cb_t callback);

 private:
};