/*
 * BÀI 1 - NGẮT NGOÀI (EXTERNAL INTERRUPT) + PWM LED TRÊN ESP32
 * ---------------------------------------------------------------
 * Mục tiêu:
 *   - Khi nhấn nút nhấn (nối GPIO4, dùng INPUT_PULLUP, cạnh xuống FALLING)
 *     -> trình phục vụ ngắt (ISR) chỉ đặt cờ "buttonPressed = true"
 *     -> loop() đọc cờ đó và cho LED (GPIO2, điều khiển qua PWM/LEDC) nháy
 *        tắt/sáng nhanh vài lần để báo hiệu đã nhận được sự kiện nhấn nút.
 *   - Khi không nhấn nút, LED tự "thở" (fade) tăng/giảm độ sáng liên tục
 *     bằng PWM (hiệu ứng breathing LED) nhờ biến brightness/step.
 *
 * Phần cứng:
 *   - Nút nhấn: 1 chân nối GPIO4, 1 chân nối GND (không cần điện trở ngoài
 *     vì đã bật INPUT_PULLUP nội bộ của ESP32).
 *   - LED: chân dương (qua điện trở ~220-330 ohm) nối GPIO2, chân âm nối GND.
 *     (Nhiều board ESP32 DevKit có sẵn LED onboard ở GPIO2.)
 *
 * Lưu ý quan trọng về ISR trên ESP32:
 *   - Hàm ISR phải NGẮN GỌN, không gọi Serial.print, không delay().
 *   - Biến dùng chung giữa ISR và loop() phải khai báo "volatile".
 *   - Dùng IRAM_ATTR để ISR được nạp vào IRAM, tránh lỗi khi code đang
 *     chạy từ flash (SPI flash) bị cache miss lúc có ngắt.
 */

#include <Arduino.h>

#define BUTTON_PIN   4   // Chân nút nhấn (ngắt ngoài)
#define LED_PIN      2   // Chân LED (điều khiển bằng PWM qua LEDC)
#define LED_CHANNEL  0   // Kênh LEDC dùng cho LED (core Arduino-ESP32 v2.x)
#define LED_FREQ     5000
#define LED_RES_BITS 8   // 8-bit -> giá trị PWM 0-255

// Cờ báo hiệu đã có sự kiện nhấn nút, dùng chung giữa ISR và loop()
volatile bool buttonPressed = false;

// ----------------- TRÌNH PHỤC VỤ NGẮT (ISR) -----------------
void IRAM_ATTR handleInterrupt() {
  buttonPressed = true;   // ISR ngắn gọn, chỉ đặt cờ
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== BAI 1: Ngat ngoai + PWM LED (ESP32 / PlatformIO) ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Thiết lập kênh PWM (LEDC) cho LED: tần số 5000Hz, độ phân giải 8-bit (0-255)
  ledcSetup(LED_CHANNEL, LED_FREQ, LED_RES_BITS);
  ledcAttachPin(LED_PIN, LED_CHANNEL);

  // Gắn ngắt ngoài: khi tín hiệu tại BUTTON_PIN chuyển từ HIGH -> LOW (FALLING)
  // thì gọi hàm handleInterrupt()
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, FALLING);

  Serial.println("San sang. Nhan nut de xem LED nhay nhanh bao hieu ngat.");
}

void loop() {
  static int brightness = 0, step = 1;

  // ---- Xử lý sự kiện ngắt (nhấn nút) ----
  if (buttonPressed) {
    buttonPressed = false;   // Xoá cờ ngay sau khi xử lý xong

    Serial.println(">> Phat hien ngat (nhan nut)!");

    // Nháy LED tắt/sáng nhanh vài lần để báo hiệu đã nhận ngắt
    for (int i = 0; i < 3; i++) {
      ledcWrite(LED_CHANNEL, 0);   delay(80);  // nháy tắt nhanh
      ledcWrite(LED_CHANNEL, 255); delay(80);  // nháy sáng nhanh
    }
  }

  // ---- Hiệu ứng "thở" (breathing LED) khi rảnh ----
  brightness += step;
  if (brightness <= 0 || brightness >= 255) step = -step;
  ledcWrite(LED_CHANNEL, brightness);
  delay(10);
}
