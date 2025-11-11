# ESP32-S3-TugasRTOS

**Author:** Zulkipli Siregar_3223600021  
**Platform:** ESP32-S3 DevKitC  
**Simulator:** [Wokwi](https://wokwi.com/projects/447248190789876737)  
<img width="565" height="309" alt="image" src="https://github.com/user-attachments/assets/4c831d3e-1539-446c-9726-d2f2bf399a3e" />

---

## **Deskripsi**
Proyek ini adalah demo FreeRTOS pada ESP32-S3.  
Menjalankan beberapa task secara bersamaan:

- OLED (SSD1306 I2C)
- 3x LED
- 2x Button
- Buzzer
- Potentiometer
- Servo
- Stepper driver (A4988)
- Rotary encoder (KY-040)

Task dapat dijalankan di **core berbeda**.  
Ganti core OLED task dengan menekan **kedua tombol** selama 1 detik.

---

## **Pinout**

| Peripheral | Pin ESP32 |
|------------|-----------|
| OLED SDA   | 17        |
| OLED SCL   | 18        |
| LED1       | 6         |
| LED2       | 12        |
| LED3       | 19        |
| Buzzer     | 11        |
| Button1    | 7         |
| Button2    | 9         |
| Potentiometer | 13      |
| Servo      | 15        |
| Stepper STEP | 4       |
| Stepper DIR  | 8       |
| Encoder CLK | 5        |
| Encoder DT  | 20       |
| Encoder SW  | 21       |

---

## **Cara Menjalankan**
1. Buka proyek di [Wokwi](https://wokwi.com/projects/447248190789876737).
2. Klik **Start Simulation**.
3. Serial monitor akan menampilkan status task dan input tombol.
4. Tekan **Button1 + Button2** selama 1 detik untuk mengganti core OLED task.

---

## **Task & Fungsinya**

### 1. **Button Task**
- File: `task_button.ino`
- Fungsi: Membaca status kedua tombol dan mencetak ke serial monitor.  
- Special: Tekan kedua tombol >1 detik untuk mengganti core OLED.

### 2. **OLED Task**
- File: `task_oled.ino`
- Fungsi: Menampilkan informasi pada OLED.  
- Core dapat diganti menggunakan tombol panjang.

### 3. **LED Task**
- File: `task_led.ino`
- Fungsi: Menyalakan LED berdasarkan nilai potensiometer dan timer.

### 4. **Buzzer Task**
- File: `task_buzzer.ino`
- Fungsi: Mengeluarkan nada buzzer tergantung tombol yang ditekan.

### 5. **Potensiometer Task**
- File: `task_pot.ino`
- Fungsi: Membaca nilai analog dari potensiometer dan menyimpannya di variabel global.

### 6. **Servo Task**
- File: `task_servo.ino`
- Fungsi: Menggerakkan servo berdasarkan nilai potensiometer.

### 7. **Stepper Task**
- File: `task_stepper.ino`
- Fungsi: Menggerakkan motor stepper dengan kontrol sederhana.

### 8. **Encoder Task**
- File: `task_encoder.ino`
- Fungsi: Membaca posisi rotary encoder dan tombolnya.

---

## **Task Button**

#include <Arduino.h>

#define BUTTON1_PIN 7
#define BUTTON2_PIN 9

volatile bool button1Pressed = false;
volatile bool button2Pressed = false;

void TaskButton(void *pvParameters) {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  while (1) {
    bool state1 = digitalRead(BUTTON1_PIN) == LOW;
    bool state2 = digitalRead(BUTTON2_PIN) == LOW;

    if (state1 != button1Pressed) {
      button1Pressed = state1;
      Serial.printf("Button 1 %s | Core %d\n", state1 ? "DITEKAN" : "DILEPAS", xPortGetCoreID());
    }

    if (state2 != button2Pressed) {
      button2Pressed = state2;
      Serial.printf("Button 2 %s | Core %d\n", state2 ? "DITEKAN" : "DILEPAS", xPortGetCoreID());
    }

    // Detect long press kedua tombol >1s untuk swap core OLED
    if (state1 && state2) {
      TickType_t start = xTaskGetTickCount();
      while (digitalRead(BUTTON1_PIN) == LOW && digitalRead(BUTTON2_PIN) == LOW) {
        if ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS >= 1000) {
          Serial.println("Swap Core OLED diminta!");
          // Set flag global swap OLED di program utama
          extern volatile bool requestSwapOLED;
          requestSwapOLED = true;
          break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

https://github.com/user-attachments/assets/0fabaa48-871f-4fb6-a4b9-346515dd2d2e

---

## **Task OLED**

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SDA_PIN 17
#define OLED_SCL_PIN 18
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(128, 64, &Wire, -1);

SemaphoreHandle_t oledMutex = NULL;
SemaphoreHandle_t oledExitSem = NULL;
volatile bool oledKillReq = false;

void TaskOLED(void *pvParameters) {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found!");
    vTaskDelete(NULL);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  while (1) {
    if (oledKillReq) break;

    if (xSemaphoreTake(oledMutex, portMAX_DELAY)) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.printf("OLED Core: %d", xPortGetCoreID());
      display.display();
      xSemaphoreGive(oledMutex);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  if (oledExitSem) xSemaphoreGive(oledExitSem);
  vTaskDelete(NULL);
}

OLED Core 0 :

https://github.com/user-attachments/assets/40d02e90-66b4-4efb-b445-430853b2c387

OLED Core 1 :

https://github.com/user-attachments/assets/644a627e-831d-4ac2-a4f3-e6e8af08a319

---

## **Task LED**

#include <Arduino.h>

#define LED1_PIN 6
#define LED2_PIN 12
#define LED3_PIN 19

#define POT_PIN 13  // gunakan pot untuk demo LED1/2

extern volatile int potValue;

void TaskLED(void *pvParameters) {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  while (1) {
    int b = map(potValue, 0, 4095, 0, 255);
    digitalWrite(LED1_PIN, b > 128 ? HIGH : LOW);
    digitalWrite(LED2_PIN, b <= 128 ? HIGH : LOW);
    digitalWrite(LED3_PIN, (millis()/500)%2 ? HIGH : LOW);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

LED Core 0 :

https://github.com/user-attachments/assets/bf544e55-945d-4165-a986-0483cf5a1575

LED Core 1 :

https://github.com/user-attachments/assets/65c2fcf8-01c4-49fa-9a1b-750d1fa2374e

---

## **Task Buzzer**

#include <Arduino.h>

#define BUZZER_PIN 11

extern volatile bool button1Pressed;
extern volatile bool button2Pressed;

void TaskBuzzer(void *pvParameters) {
  pinMode(BUZZER_PIN, OUTPUT);

  while (1) {
    if (button1Pressed) tone(BUZZER_PIN, 1000);
    else if (button2Pressed) tone(BUZZER_PIN, 1800);
    else noTone(BUZZER_PIN);

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

Buzzer Core 0 :

https://github.com/user-attachments/assets/bb952614-227c-4a63-bb53-b67e7c95150d

Buzzer Core 1 :

https://github.com/user-attachments/assets/07e4ea76-98ef-48d3-b28c-556dd27f822a

---

## **Task Potentiometer**

#include <Arduino.h>

#define POT_PIN 13

volatile int potValue = 0;

void TaskPot(void *pvParameters) {
  while (1) {
    int val = analogRead(POT_PIN);
    potValue = val;
    Serial.printf("POT Value: %d | Core %d\n", val, xPortGetCoreID());
    vTaskDelay(120 / portTICK_PERIOD_MS);
  }
}

Potentiometer Core 0 :

https://github.com/user-attachments/assets/348a7344-fff5-428a-b8bd-b96f9abd5b95

Potentiometer Core 1 :

https://github.com/user-attachments/assets/176d42dd-ba33-4bb5-818d-4073435d4bf3

---

## **Task Servo**

#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO_PIN 15

volatile int servoAngle = 90;
Servo myservo;

void TaskServo(void *pvParameters) {
  myservo.attach(SERVO_PIN);
  myservo.write(90);

  while (1) {
    int angle = map(analogRead(13), 0, 4095, 0, 180);
    servoAngle = angle;
    myservo.write(angle);
    vTaskDelay(120 / portTICK_PERIOD_MS);
  }
}

Servo Core 0 :

https://github.com/user-attachments/assets/c23c4d7e-3b85-457f-84e6-ab9a65b71eba

Servo Core 1 :

https://github.com/user-attachments/assets/f6424be5-ef3e-4ea0-8945-a8ff33eea754

---

## **Task Stepper**

#include <Arduino.h>

#define STEP_PIN 4
#define DIR_PIN 8

volatile int stepperDir = 1;
volatile int stepperSpeed = 200;

void TaskStepper(void *pvParameters) {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  while (1) {
    int spd = abs(stepperSpeed);
    if (spd <= 0) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      continue;
    }

    digitalWrite(DIR_PIN, stepperDir > 0 ? HIGH : LOW);
    float period_ms = 1000.0 / spd;
    int halfPulse_us = max(50, int((period_ms*1000)/2));

    digitalWrite(STEP_PIN, HIGH);
    ets_delay_us(halfPulse_us);
    digitalWrite(STEP_PIN, LOW);
    ets_delay_us(halfPulse_us);

    vTaskDelay(0 / portTICK_PERIOD_MS);
  }
}

Stepper Core 0 :

https://github.com/user-attachments/assets/2ed69603-097b-47f9-b5a8-25d59236d42b

Stepper Core 1 :

https://github.com/user-attachments/assets/4c139331-b40e-45a7-9a67-dbf2521fd7ff

---

## **Task Encoder**

#include <Arduino.h>

#define ENC_CLK 5
#define ENC_DT  20
#define ENC_SW  21

volatile long encoderPos = 0;
volatile bool encoderBtnPressed = false;
int lastEncA = 0;

void TaskEncoder(void *pvParameters) {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lastEncA = digitalRead(ENC_CLK);

  while (1) {
    int a = digitalRead(ENC_CLK);
    int b = digitalRead(ENC_DT);

    if (a != lastEncA) {
      encoderPos += (b != a) ? 1 : -1;
    }
    lastEncA = a;

    encoderBtnPressed = digitalRead(ENC_SW) == LOW;

    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

Encoder Core 0 :

https://github.com/user-attachments/assets/fb4730ef-65cc-4c5e-a816-a08bbd19a2e7

Encoder Core 1 :

https://github.com/user-attachments/assets/a2f98fec-7364-4605-8458-1eee8f4e7ffa
