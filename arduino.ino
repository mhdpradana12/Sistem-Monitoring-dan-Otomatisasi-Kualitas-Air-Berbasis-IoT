/*************************************************************
 
  You’ll need:
   - Blynk IoT app (download from App Store or Google Play)
   - ESP32 board
   - Decide how to connect to Blynk
     (USB, Ethernet, Wi-Fi, Bluetooth, ...)

  There is a bunch of great example sketches included to show you how to get
  started. Think of them as LEGO bricks  and combine them as you wish.
  For example, take the Ethernet Shield sketch and combine it with the
  Servo example, or choose a USB sketch and add a code from SendData
  example.
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "..."
#define BLYNK_TEMPLATE_NAME         "..."
#define BLYNK_AUTH_TOKEN            "..."

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

//Menambahkan Library
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "...";
char pass[] = "...";

//Deklarasi Variabel Pin 
const int TEMPERATURE_PIN = 27; // Sensor Suhu DS18B20
const int TURBIDITY_PIN = 33; // Sensor Turbidity SEN0189
const int PH_PIN  = 32; // Sensor pH 4502c
const int PUMP_PIN = 5; // Relay IN1 (Pompa Air)
const int HEATER_PIN = 4; // Relay IN2 (Heater)
const int FAN_PIN = 2; // Relay IN3 (Kipas)

bool manualMode = false; //Variable Kontrol Manual

//Konfigurasi Sensor DS18B20
OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
float TEMPERATURE;

//Konfigurasi Sensor pH 4502c
int ADC_PH;
double VOLTAGE_PH;
float RESULT_PH = 0;
float PH_STEP;
float PH4;
float PH7;

//Konfigurasi Sensor Turbidity SEN0189
int ADC_TURBIDITY;
double VOLTAGE_TURBIDITY;
float NTU_TURBIDITY;
float convertToNTU (int ADC_TURBIDITY) {
  int ADC_CLEAN = 1600;
  int ADC_DIRTY = 700;
  float ntu = (ADC_TURBIDITY - ADC_CLEAN) * (100.0 / (ADC_DIRTY - ADC_CLEAN));
  return ntu;
}

// Konfigurasi Notifikasi ke Blynk
float PREVIOUS_TEMPERATURE = 0.0;
float PREVIOUS_TURBIDITY = 0.0;
float PREVIOUS_PH = 0.0;

BlynkTimer timer; //Mengatur time Blynk

//Fungsi Mengirim Data Sensor ke Blynk
void send_sensor() 
{
  Blynk.virtualWrite(V1, TEMPERATURE);
  Blynk.virtualWrite(V2, NTU_TURBIDITY);
  Blynk.virtualWrite(V3, String(RESULT_PH, 1));
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  
  sensors.begin();

  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(PH_PIN, INPUT);

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  Blynk.virtualWrite(V4, HIGH);
  Blynk.virtualWrite(V5, HIGH);
  Blynk.virtualWrite(V6, HIGH);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, send_sensor); 
  // You can also specify server:
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, IPAddress(192,168,1,100), 8080);
}

void loop()
{
  Blynk.run();

  // Program Sensor DS18B20
  sensors.requestTemperatures();
  TEMPERATURE = sensors.getTempCByIndex(0);
  Serial.print("Suhu: ");
  Serial.print(TEMPERATURE);
  Serial.println(" C");

  if (!manualMode) {
    if (TEMPERATURE != PREVIOUS_TEMPERATURE) {
      PREVIOUS_TEMPERATURE = TEMPERATURE;
      if (TEMPERATURE < 25.0) {
        Blynk.logEvent("suhu_air", String(" Rendah! Tº: ") + String(TEMPERATURE) + String(" ºC"));
        digitalWrite(HEATER_PIN, LOW);
        digitalWrite(FAN_PIN, HIGH);
      } else if (TEMPERATURE > 30.0) {
        Blynk.logEvent("suhu_air", String(" Tinggi! Tº: ") + String(TEMPERATURE) + String(" ºC"));
        digitalWrite(FAN_PIN, LOW);
        digitalWrite(HEATER_PIN, HIGH);
      } else {
        digitalWrite(HEATER_PIN, HIGH);
        digitalWrite(FAN_PIN, HIGH);
      }
    }
  }
 
  // Program Sensor pH 4502c
  ADC_PH = analogRead(PH_PIN);
  PH4 = 3.1;
  PH7 = 2.75;
  //PH9 = 2.4;
  VOLTAGE_PH = 5 / 4095.0 * ADC_PH;
  PH_STEP = (PH4 - PH7) / 3;
  //PH_STEP = (PH9 - PH4) / 5;
  RESULT_PH = 7.00 + ((PH7 - VOLTAGE_PH) / PH_STEP);
  //RESULT_PH = 4.00 + ((PH9 - VOLTAGE_PH) / PH_STEP);
  // Serial.print("Nilai ADC pH: ");
  // Serial.println(ADC_PH);
  // Serial.print("Nilai Voltage pH: ");
  // Serial.println(VOLTAGE_PH);
  Serial.print("Nilai pH: ");
  Serial.println(RESULT_PH);

  if (!manualMode) {
    if (RESULT_PH != PREVIOUS_PH) {
      PREVIOUS_PH = RESULT_PH;
      if (RESULT_PH < 6.0) {
        Blynk.logEvent("keasaman_air", String(" Rendah! pH: ") + String(RESULT_PH));
        digitalWrite(BUZZER_PIN, HIGH);
      } else if (RESULT_PH > 8.1){
        Blynk.logEvent("keasaman_air", String(" Tinggi! pH: ") + String(RESULT_PH));
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
  }

  // Program Sensor Turbidity SEN0189
  ADC_TURBIDITY = analogRead(TURBIDITY_PIN);
  VOLTAGE_TURBIDITY = 3.3 / 1024.0 * ADC_TURBIDITY;
  NTU_TURBIDITY = convertToNTU(ADC_TURBIDITY);
  // Serial.print("ADC Turbidity: ");
  // Serial.println(ADC_TURBIDITY);
  // Serial.print("Voltage Turbidity: ");
  // Serial.println(VOLTAGE_TURBIDITY);
  Serial.print("Kekeruhan NTU: ");
  Serial.println(NTU_TURBIDITY);

  if (!manualMode) {
    if (NTU_TURBIDITY != PREVIOUS_TURBIDITY) {
      PREVIOUS_TURBIDITY = NTU_TURBIDITY; // Menyimpan nilai kekeruhan saat ini sebagai kekeruhan sebelumnya
      // Memberikan notifikasi jika kualitas air keruh 
      if (NTU_TURBIDITY > 50.0) {
        Blynk.logEvent("keruh_air", String(" Keruh! NTU: ") + String(NTU_TURBIDITY));
        digitalWrite(PUMP_PIN, LOW);
      } else {
        digitalWrite(PUMP_PIN, HIGH);
      }
    }
  }

  timer.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}

BLYNK_WRITE(V1) {
  int value = param.asInt(); // Menerima data tombol dari virtual pin V1
}

BLYNK_WRITE(V2) {
  int value = param.asInt(); // Menerima data tombol dari virtual pin V2
}

BLYNK_WRITE(V3) {
  float value = param.asFloat(); // Menerima data tombol dari virtual pin V3
}

BLYNK_WRITE(V4) {
  int value = param.asInt();
  if (value == 1) {
    manualMode = true;
    digitalWrite(PUMP_PIN, LOW);
  } else {
    manualMode = false;
  }
}

BLYNK_WRITE(V5) {
  int value = param.asInt();
  if (value == 1) {
    manualMode = true;
    digitalWrite(HEATER_PIN, LOW);
  } else {
    manualMode = false;
  }
}

BLYNK_WRITE(V6) {
  int value = param.asInt();
  if (value == 1) {
    manualMode = true;
    digitalWrite(FAN_PIN, LOW);
  } else {
    manualMode = false;
  }
}
