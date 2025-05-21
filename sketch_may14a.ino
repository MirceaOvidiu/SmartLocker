#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID pins
#define SS_PIN A0
#define RST_PIN A1
MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {9, 8, 7, 6};
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2};

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

LiquidCrystal_I2C lcd(0x3F, 16, 2);

Servo myservo;
const int load_cell = A3;
const int buzzer = A2;  

#define BUTTON_PIN 13

int PASSLEN = 5;
char data[5] = {0};
char password[5] = "1111";
byte data_count = 0;
char key;

byte tries = 3;
int alert = 0;

// Track lock state
bool isLocked = true;

// RFID card UID to toggle lock: 33 B9 87 1F
byte toggleUID[4] = {0x33, 0xB9, 0x87, 0x1F};

void setup(){
  Serial.begin(9600);
  
  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();
  
  myservo.attach(10);
  lockDoor(); // Start locked
  
  pinMode(buzzer, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Reader initialized.");
  
  // Print first-time instructions
  lcd.clear();
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop(){
  // --- Load cell reading and buzzer logic ---
  int loadCellValue = analogRead(load_cell);
  Serial.print("Load cell reading: ");
  Serial.println(loadCellValue);

  // If load cell reading drops under 35, sound alarm
  if (loadCellValue < 35) {
    tone(buzzer, 1000); // Sound buzzer at 1kHz
  } else if (alert == 0) {
    noTone(buzzer);     // Only stop buzzer if not in alert mode
  }

  // --- RFID logic ---
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (isToggleCard()) {
      if (alert == 1) {
        // Stop alert mode and buzzer
        alert = 0;
        tries = 3;
        noTone(buzzer);
        lcd.clear();
        lcd.print("RFID Authorized!");
        unlockDoor();
        isLocked = false;
        delay(1500);
        lcd.clear();
      } else {
        lcd.clear();
        if (isLocked) {
          lcd.print("RFID Unlocked!");
          unlockDoor();
          isLocked = false;
        } else {
          lcd.print("RFID Locked!");
          lockDoor();
          isLocked = true;
        }
        delay(1500);
        lcd.clear();
      }
    }
    // Halt PICC and stop encryption
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // --- Keypad and lock logic ---
  if(alert == 0) {
    lcd.setCursor(0,0);
    if(isLocked) {
      lcd.print("Enter password   ");
      lcd.setCursor(0,1);
      lcd.print("PIN: ");
    } else {
      lcd.print("Enter password   ");
      lcd.setCursor(0,1);
      lcd.print("PIN: ");
    }

    // Show the current typed password
    for (byte i = 0; i < data_count; i++) {
      lcd.setCursor(5 + i, 1);
      lcd.print(data[i]);
    }
    // Clear the rest of the line if user deletes chars
    for (byte i = data_count; i < PASSLEN-1; i++) {
      lcd.setCursor(5 + i, 1);
      lcd.print(" ");
    }

    key = keypad.getKey();
    if(key && key != '#') {
      if(data_count < PASSLEN-1) { // Prevent buffer overflow
        data[data_count] = key;
        data_count++;
      }
    }

    if(key == '#') {
      lcd.clear();

      data[data_count] = '\0'; // Null-terminate for strcmp

      if(!strcmp(data, password)) {
        if(isLocked) {
          lcd.print("Unlocked");
          unlockDoor();
          isLocked = false;
        } else {
          lcd.print("Locked");
          lockDoor();
          isLocked = true;
        }
        delay(1500);
      }
      else {
        lcd.print("Incorrect");
        tries--;
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);

        if(tries == 0) {
          lcd.print("No more tries");
          alert = 1; // Enter alert mode
        }
        else {
          lcd.print("You can try");
          lcd.setCursor(0, 1);
          lcd.print(tries);
          if(tries == 1) {
            lcd.print(" more time");
          }
          else {  
            lcd.print(" more times");
          }
        }
        delay(1000);
      }

      lcd.clear();

      // Clear password buffer
      while(data_count !=0) {
        data[--data_count] = 0; 
      }
    }
  }
  else {
    // Alert mode - buzzer ON, only RFID can stop
    lcd.setCursor(0,0);
    lcd.print("Alert!");
    lcd.setCursor(0, 1);
    lcd.print("Use RFID key");
    tone(buzzer, 500); 
    // Do not allow manual override or stop buzzer except with RFID
    delay(200); // Small delay to prevent CPU hogging
  }
}

// Function to unlock door with servo
void unlockDoor() {
  myservo.write(130);
}

// Function to lock door with servo
void lockDoor() {
  myservo.write(10);
}

// Function to check if the detected card is the toggle card
bool isToggleCard() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != toggleUID[i]) {
      return false;
    }
  }
  Serial.println("Toggle card detected!");
  return true;
}