/*
  ESP32-S3: multi-peripheral single-active-task + dual-button core-switch
  - Button1 (pin 7) reader task pinned to Core 0
  - Button2 (pin 9) reader task pinned to Core 1
  - Hold both buttons >= 1s -> toggle active core (0 <-> 1)
  - Serial menu selects which peripheral task runs (only one active)
  - Peripheral task is started pinned to activeCore
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// -------------- pin mapping (your pins) --------------
#define OLED_SDA_PIN 17
#define OLED_SCL_PIN 18
#define OLED_ADDR 0x3C

#define LED1_PIN 6
#define LED2_PIN 12
#define LED3_PIN 19

#define BUZZER_PIN 11

#define BUTTON1_PIN 7
#define BUTTON2_PIN 9

#define POT_PIN 13

#define SERVO_PIN 15

#define STEP_PIN 4
#define DIR_PIN 8

#define ENC_CLK 5
#define ENC_DT  20
#define ENC_SW  21

// -------------- devices / objects --------------
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Servo servo;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// -------------- task handles --------------
TaskHandle_t thPeripheral = NULL;   // currently active peripheral task
TaskHandle_t thBtn1 = NULL;
TaskHandle_t thBtn2 = NULL;
TaskHandle_t thSerial = NULL;       // optional serial menu task (we'll use loop for menu)

// -------------- shared state --------------
volatile bool button1Pressed = false;
volatile bool button2Pressed = false;
volatile int activeCore = 0;        // 0 or 1
volatile int selectedPeripheral = 0; // 0=none, 1..7 peripheral index

// Debounce / both-pressed detection
unsigned long bothPressedStart = 0;
bool bothPressedOngoing = false;

// ---------------- forward declarations ----------------
void startPeripheralOnActiveCore(int id);
void stopPeripheral();
void printMenu();

// ---------------- safe delete ----------------
void safeDeleteHandle(TaskHandle_t &h) {
  if (h != NULL) {
    vTaskDelete(h);
    h = NULL;
  }
}

// ================= Peripheral tasks =================

// 1) LED
void Task_LED(void *pv) {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  for (;;) {
    digitalWrite(LED1_PIN, HIGH); digitalWrite(LED2_PIN, LOW); digitalWrite(LED3_PIN, LOW);
    Serial.printf("[LED] LED1 ON | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(LED1_PIN, LOW); digitalWrite(LED2_PIN, HIGH);
    Serial.printf("[LED] LED2 ON | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(LED2_PIN, LOW); digitalWrite(LED3_PIN, HIGH);
    Serial.printf("[LED] LED3 ON | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

// 2) Buzzer
void Task_Buzzer(void *pv) {
  pinMode(BUZZER_PIN, OUTPUT);
  for (;;) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.printf("[BUZZER] ON | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(150));
    digitalWrite(BUZZER_PIN, LOW);
    Serial.printf("[BUZZER] OFF | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(350));
  }
}

// 3) Pot (read only)
void Task_Pot(void *pv) {
  for (;;) {
    int v = analogRead(POT_PIN);
    Serial.printf("[POT] %d | Core %d\n", v, xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// 4) OLED
void Task_OLED(void *pv) {
  // init i2c/display here so it only runs when task active
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] init failed");
    vTaskDelete(NULL);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  for (;;) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.printf("Peripheral ID: %d\nCore: %d\n", selectedPeripheral, xPortGetCoreID());
    display.display();
    Serial.printf("[OLED] update | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// 5) Encoder
int prevCLK = digitalRead(ENC_CLK);
int encValue = 0;

void Task_Encoder(void *pv) {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  for (;;) {
    int clk = digitalRead(ENC_CLK);
    if (clk != prevCLK) { // Deteksi perubahan
      if (digitalRead(ENC_DT) != clk) encValue++; 
      else encValue--;
      Serial.printf("[ENC] %d Core %d\n", encValue, xPortGetCoreID());
    }
    prevCLK = clk;
    vTaskDelay(1 / portTICK_PERIOD_MS); // cek cepat
  }
}

// 6) Servo
void Task_Servo(void *pv) {
  servo.attach(SERVO_PIN);
  for (;;) {
    for (int p=20; p<=160; p+=10) {
      servo.write(p);
      Serial.printf("[SERVO] %d | Core %d\n", p, xPortGetCoreID());
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    for (int p=160; p>=20; p-=10) {
      servo.write(p);
      Serial.printf("[SERVO] %d | Core %d\n", p, xPortGetCoreID());
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

// 7) Stepper (A4988 via STEP/DIR)
void Task_Stepper(void *pv) {
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(200);
  for (;;) {
    stepper.moveTo(300);
    while (stepper.distanceToGo()) stepper.run();
    Serial.printf("[STEPPER] forward | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(100));
    stepper.moveTo(-300);
    while (stepper.distanceToGo()) stepper.run();
    Serial.printf("[STEPPER] back | Core %d\n", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ---------------- Button reader tasks ----------------
// Task for BUTTON1 on CORE 0
void Task_Button1_Core0(void *pv) {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  Serial.printf("[BTN1] reader on core %d\n", xPortGetCoreID());
  bool last = digitalRead(BUTTON1_PIN);
  for (;;) {
    bool cur = (digitalRead(BUTTON1_PIN) == LOW);
    if (cur != button1Pressed) {
      button1Pressed = cur;
      Serial.printf("[BTN1] %s | Core %d\n", cur ? "PRESSED" : "RELEASED", xPortGetCoreID());
    }
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

// Task for BUTTON2 on CORE 1
void Task_Button2_Core1(void *pv) {
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  Serial.printf("[BTN2] reader on core %d\n", xPortGetCoreID());
  bool last = digitalRead(BUTTON2_PIN);
  for (;;) {
    bool cur = (digitalRead(BUTTON2_PIN) == LOW);
    if (cur != button2Pressed) {
      button2Pressed = cur;
      Serial.printf("[BTN2] %s | Core %d\n", cur ? "PRESSED" : "RELEASED", xPortGetCoreID());
    }
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

// ---------------- Peripheral control: start/stop pinned to activeCore ----------------
void startPeripheralOnActiveCore(int id) {
  // stop existing
  stopPeripheral();
  BaseType_t r = pdFAIL;
  const char *name = "PER";
  uint32_t stack = 3072;
  switch (id) {
    case 1: r = xTaskCreatePinnedToCore(Task_LED,    "LED",    3072, NULL, 1, &thPeripheral, activeCore); name="LED"; break;
    case 2: r = xTaskCreatePinnedToCore(Task_Buzzer, "BZR",    2048, NULL, 1, &thPeripheral, activeCore); name="BZR"; break;
    case 3: r = xTaskCreatePinnedToCore(Task_Pot,    "POT",    2048, NULL, 1, &thPeripheral, activeCore); name="POT"; break;
    case 4: r = xTaskCreatePinnedToCore(Task_OLED,   "OLED",   4096, NULL, 1, &thPeripheral, activeCore); name="OLED"; break;
    case 5: r = xTaskCreatePinnedToCore(Task_Encoder,"ENC",    2048, NULL, 1, &thPeripheral, activeCore); name="ENC"; break;
    case 6: r = xTaskCreatePinnedToCore(Task_Servo,  "SRV",    4096, NULL, 1, &thPeripheral, activeCore); name="SRV"; break;
    case 7: r = xTaskCreatePinnedToCore(Task_Stepper,"STP",    4096, NULL, 1, &thPeripheral, activeCore); name="STP"; break;
    default: return;
  }
  if (r == pdPASS) {
    Serial.printf("[SYSTEM] Started %s (id=%d) on core %d\n", name, id, activeCore);
    selectedPeripheral = id;
  } else {
    Serial.printf("[SYSTEM] Failed to start peripheral id=%d\n", id);
    selectedPeripheral = 0;
  }
}

void stopPeripheral() {
  if (thPeripheral != NULL) {
    // If it was servo or oled, detach/cleanup
    if (selectedPeripheral == 6) servo.detach();
    // OLED cleanup not strictly required; task deletes will free it
    safeDeleteHandle(thPeripheral);
    selectedPeripheral = 0;
    Serial.println("[SYSTEM] Peripheral stopped");
  }
}

// --------------- monitor both-press and orchestrator ---------------
void checkBothPressedAndToggleCore() {
  // called from main loop
  if (button1Pressed && button2Pressed) {
    if (!bothPressedOngoing) {
      bothPressedOngoing = true;
      bothPressedStart = millis();
    } else {
      if (millis() - bothPressedStart >= 1000) {
        // toggle core
        activeCore = (activeCore == 0) ? 1 : 0;
        Serial.printf("[SYSTEM] BOTH pressed >=1s -> toggled activeCore to %d\n", activeCore);
        // if a peripheral is active, restart it pinned to new core
        if (selectedPeripheral != 0) {
          Serial.println("[SYSTEM] Restarting peripheral on new core...");
          startPeripheralOnActiveCore(selectedPeripheral);
        }
        // debounce long-press so we don't toggle repeatedly
        bothPressedOngoing = false;
        // wait a bit
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }
  } else {
    bothPressedOngoing = false;
  }
}

// ---------------- Serial menu (simple, handled in loop) ----------------
void printMenu() {
  Serial.println("\n===== Peripheral Menu =====");
  Serial.println("1 - LED (rotate 3 LEDs)");
  Serial.println("2 - Buzzer");
  Serial.println("3 - Pot read");
  Serial.println("4 - OLED display");
  Serial.println("5 - Encoder");
  Serial.println("6 - Servo");
  Serial.println("7 - Stepper");
  Serial.println("0 - Stop peripheral");
  Serial.print("Select (0..7): ");
}

// --------------- setup & loop ---------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ESP32-S3 Multi-Peripheral with Dual-button Core Switch ===");
  Serial.printf("Initial active core = %d\n", activeCore);
  // prepare safe defaults
  pinMode(LED1_PIN, OUTPUT); digitalWrite(LED1_PIN, LOW);
  pinMode(LED2_PIN, OUTPUT); digitalWrite(LED2_PIN, LOW);
  pinMode(LED3_PIN, OUTPUT); digitalWrite(LED3_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);

  // create button reader tasks pinned to cores
  xTaskCreatePinnedToCore(Task_Button1_Core0, "BTN1_C0", 2048, NULL, 2, &thBtn1, 0);
  xTaskCreatePinnedToCore(Task_Button2_Core1, "BTN2_C1", 2048, NULL, 2, &thBtn2, 1);

  printMenu();
}

void loop() {
  // Serial menu handling (non-blocking)
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c <= '7') {
      int sel = c - '0';
      if (sel == 0) {
        stopPeripheral();
      } else {
        // start selected peripheral pinned to activeCore
        startPeripheralOnActiveCore(sel);
      }
      printMenu();
    }
  }

  // check both-pressed for core toggle
  checkBothPressedAndToggleCore();

  vTaskDelay(pdMS_TO_TICKS(50));
}
