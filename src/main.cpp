#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// RS485 Pin Configuration
#define RO D7   // RX (Dari RS485 ke ESP8266)
#define DI D5   // TX (Dari ESP8266 ke RS485)
#define RE_DE D6 // RE/DE untuk mode transmisi/penerimaan

SoftwareSerial rs485Serial(RO, DI); // RX, TX
ModbusMaster node;  // Buat objek Modbus

void preTransmission() {
  digitalWrite(RE_DE, HIGH);
  delay(2);
}

void postTransmission() {
  digitalWrite(RE_DE, LOW);
}

void setup() {
  Serial.begin(115200);  // Serial monitor
  rs485Serial.begin(9600); // Baudrate VFD

  pinMode(RE_DE, OUTPUT);
  digitalWrite(RE_DE, LOW);

  node.begin(1, rs485Serial);  // Slave ID = 1
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("ATV12 Controller Ready");
  Serial.println("Perintah:");
  Serial.println("f30 = forward 30Hz");
  Serial.println("r30 = reverse 30Hz");
  Serial.println("stop = berhenti");
}

void setFrequencyWithDirection(float freq, bool isForward) {
  uint16_t freqValue;
  if (isForward) {
    freqValue = (uint16_t)(freq * 10); // Konversi ke format VFD
  } else {
    freqValue = (uint16_t)(-freq * 10); // Nilai negatif untuk reverse
  }
  
  // Start sequence dengan arah
  uint8_t result;
  result = node.writeSingleRegister(8501, 6);  // Pre-charge
  delay(100);
  result = node.writeSingleRegister(8501, 7);  // Start
  delay(100);
  
  // Set frekuensi
  result = node.writeSingleRegister(8502, freqValue);
  delay(100);
  
  if (isForward) {
    result = node.writeSingleRegister(8501, 15);  // Forward run
    Serial.println("Mode: Forward");
  } else {
    result = node.writeSingleRegister(8501, 7);   // Reverse run
    Serial.println("Mode: Reverse");
  }
  
  if (result == node.ku8MBSuccess) {
    Serial.print("Frekuensi diatur ke: ");
    Serial.print(isForward ? freq : -freq);
    Serial.println(" Hz");
  } else {
    Serial.println("Gagal mengatur frekuensi!");
  }
}

void readFrequency() {
  uint8_t result = node.readHoldingRegisters(8503, 1); // Register 8503 untuk output frequency
  
  if (result == node.ku8MBSuccess) {
    int16_t freq = node.getResponseBuffer(0); // Menggunakan int16_t untuk nilai negatif
    float actualFreq = freq / 10.0;
    Serial.print("Frekuensi aktual: ");
    Serial.print(actualFreq);
    Serial.println(" Hz");
  } else {
    Serial.println("Gagal membaca frekuensi!");
  }
}

void stopMotor() {
  uint8_t result = node.writeSingleRegister(8501, 0);
  if (result == node.ku8MBSuccess) {
    Serial.println("Motor berhenti");
  } else {
    Serial.println("Gagal menghentikan motor!");
  }
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "stop") {
      stopMotor();
    } 
    else if (input.startsWith("f")) {  // Forward command
      float freq = input.substring(1).toFloat();
      if (freq >= 0 && freq <= 50) {
        setFrequencyWithDirection(freq, true);
      } else {
        Serial.println("Error: Frekuensi harus antara 0-50 Hz");
      }
    }
    else if (input.startsWith("r")) {  // Reverse command
      float freq = input.substring(1).toFloat();
      if (freq >= 0 && freq <= 50) {
        setFrequencyWithDirection(freq, false);
      } else {
        Serial.println("Error: Frekuensi harus antara 0-50 Hz");
      }
    }
  }
  
  // Baca frekuensi aktual setiap 2 detik
  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 2000) {
    readFrequency();
    lastRead = millis();
  }
}