# ESP32-S3-TugasRTOS

**Author:** Zulkipli Siregar_3223600021  
**Platform:** ESP32-S3 DevKitC  
**Simulator:** [Wokwi](https://wokwi.com/projects/447248190789876737)  

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

## **Struktur Proyek**
