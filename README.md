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
- **Fungsi:** Membaca status **Button1** dan **Button2** secara terus-menerus.  
- **Core:** Button1 di Core 0, Button2 di Core 1  
- **Fitur tambahan:**  
  - Jika kedua tombol ditekan bersamaan >1 detik → **minta swap task OLED ke core lain**  
- **Output:** Status tombol tampil di **Serial Monitor**  

### 2. **OLED Task**
- **Fungsi:** Menampilkan informasi status sistem: tombol, encoder, potentiometer, servo, dan stepper.  
- **Core:** Bisa di-core 0 atau 1 (dapat di-swap dengan tombol).  
- **Penggunaan mutex:** Untuk menghindari konflik akses display saat swap core.  

### 3. **LEDs Task**
- **Fungsi:** Mengatur LED1, LED2, LED3  
  - LED1: brightness tergantung potensiometer  
  - LED2: inverse LED1  
  - LED3: blinking sebagai indikator status  
- **Core:** Tidak dipin khusus, FreeRTOS scheduler memilih core.  

### 4. **Buzzer Task**
- **Fungsi:**  
  - Beep saat tombol atau encoder ditekan  
  - Nada berbeda untuk Button1 dan Button2  
- **Core:** Tidak dipin khusus  

### 5. **Pot + Servo Task**
- **Fungsi:**  
  - Membaca nilai potensiometer  
  - Memetakan nilai potensiometer ke sudut servo (0-180°)  
  - Mengatur kecepatan stepper berdasarkan potensiometer  
- **Core:** Tidak dipin khusus  

### 6. **Encoder Task**
- **Fungsi:** Membaca rotary encoder (KY-040)  
  - Mengubah posisi stepper (arah & kecepatan)  
  - Membaca tombol encoder  
- **Core:** Tidak dipin khusus  

### 7. **Stepper Task**
- **Fungsi:** Menggerakkan stepper melalui driver A4988  
  - Mengatur arah & kecepatan sesuai nilai dari encoder atau potensiometer  
- **Core:** Tidak dipin khusus  

### 8. **Orchestrator (loop)**
- **Fungsi:**  
  - Memantau permintaan swap OLED  
  - Menghentikan task OLED lama, kemudian membuat task OLED baru di core berbeda  

---
