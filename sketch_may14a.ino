#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID pins
#define SS_PIN A0
#define RST_PIN A1
MFRC522 rfid(SS_PIN, RST_PIN);

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

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
LiquidCrystal_I2C lcd(0x3F, 16, 2);
Servo myservo;

const int load_cell = A3;
const int buzzer = A2;
#define BUTTON_PIN 13

int PASSLEN = 5;
char data[5] = {0};
char password[5] = "1111";
char newPin[5] = {0};
char confirmPin[5] = {0};
byte data_count = 0;
byte newPinCount = 0;
byte confirmPinCount = 0;
char key;

byte tries = 3;
bool alert = false;
bool isLocked = true;

// Master card UID: 62 47 E2 55
byte masterUID[4] = {0x62, 0x47, 0xE2, 0x55};
// User card UID: 33 B9 87 1F
byte userUID[4] = {0x33, 0xB9, 0x87, 0x1F};

enum State {
  STATE_IDLE,
  STATE_STATUS,
  STATE_ALERT,
  STATE_MASTER_MENU,
  STATE_NEW_PIN_FIRST,
  STATE_NEW_PIN_SECOND
};

State currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
const unsigned long statusDisplayTime = 1500;

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();
  
  myservo.attach(10);
  lockDoor();
  
  pinMode(buzzer, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  SPI.begin();
  rfid.PCD_Init();
  
  lcd.clear();
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  int loadCellValue = analogRead(load_cell);
  
  if (loadCellValue < 35 && !alert) {
    tone(buzzer, 1000);
  } else if (!alert) {
    noTone(buzzer);
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (alert) {
      if (isMasterCard()) {
        alert = false;
        tries = 3;
        noTone(buzzer);
        lcd.clear();
        lcd.print("RFID Authorized!");
        unlockDoor();
        isLocked = false;
        stateStartTime = millis();
        currentState = STATE_STATUS;
      }
    } else {
      if (isMasterCard()) {
        lcd.clear();
        lcd.print("A: Change PIN");
        lcd.setCursor(0,1);
        lcd.print("B: Exit");
        currentState = STATE_MASTER_MENU;
      } else if (isUserCard()) {
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
        stateStartTime = millis();
        currentState = STATE_STATUS;
      } else {
        lcd.clear();
        lcd.print("Wrong RFID card!");
        tone(buzzer, 2000);
        delay(3000);
        noTone(buzzer);
        lcd.clear();
        currentState = STATE_IDLE;
      }
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  switch (currentState) {
    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_STATUS:
      if (millis() - stateStartTime > statusDisplayTime) {
        lcd.clear();
        currentState = alert ? STATE_ALERT : STATE_IDLE;
      }
      break;

    case STATE_ALERT:
      lcd.setCursor(0,0);
      lcd.print("Alert!");
      lcd.setCursor(0,1);
      lcd.print("Use Master Card");
      tone(buzzer, 500);
      break;

    case STATE_MASTER_MENU:
      key = keypad.getKey();
      if (key == 'A') {
        lcd.clear();
        lcd.print("Enter new PIN:");
        lcd.setCursor(0,1);
        currentState = STATE_NEW_PIN_FIRST;
        newPinCount = 0;
        memset(newPin, 0, sizeof(newPin));
      } else if (key == 'B') {
        lcd.clear();
        currentState = STATE_IDLE;
      }
      break;

    case STATE_NEW_PIN_FIRST:
      key = keypad.getKey();
      if (key && key != '#') {
        if (newPinCount < PASSLEN-1) {
          newPin[newPinCount] = key;
          lcd.setCursor(newPinCount, 1);
          lcd.print(key);
          newPinCount++;
        }
      }
      if (key == '#') {
        lcd.clear();
        lcd.print("Confirm PIN:");
        lcd.setCursor(0,1);
        currentState = STATE_NEW_PIN_SECOND;
        confirmPinCount = 0;
        memset(confirmPin, 0, sizeof(confirmPin));
      }
      break;

    case STATE_NEW_PIN_SECOND:
      key = keypad.getKey();
      if (key && key != '#') {
        if (confirmPinCount < PASSLEN-1) {
          confirmPin[confirmPinCount] = key;
          lcd.setCursor(confirmPinCount, 1);
          lcd.print(key);
          confirmPinCount++;
        }
      }
      if (key == '#') {
        newPin[newPinCount] = '\0';
        confirmPin[confirmPinCount] = '\0';
        if (strcmp(newPin, confirmPin) == 0) {
          strcpy(password, newPin);
          lcd.clear();
          lcd.print("PIN Changed!");
        } else {
          lcd.clear();
          lcd.print("PINs don't match!");
        }
        delay(1000);
        lcd.clear();
        currentState = STATE_IDLE;
      }
      break;
  }
}

void handleIdleState() {
  lcd.setCursor(0,0);
  lcd.print("Enter password   ");
  lcd.setCursor(0,1);
  lcd.print("PIN: ");

  for (byte i = 0; i < data_count; i++) {
    lcd.setCursor(5 + i, 1);
    lcd.print(data[i]);
  }
  for (byte i = data_count; i < PASSLEN-1; i++) {
    lcd.setCursor(5 + i, 1);
    lcd.print(" ");
  }

  key = keypad.getKey();
  if (key && key != '#') {
    if (data_count < PASSLEN-1) {
      data[data_count] = key;
      data_count++;
    }
  }

  if (key == '#') {
    lcd.clear();
    data[data_count] = '\0';

    if (!strcmp(data, password)) {
      if (isLocked) {
        lcd.print("Unlocked");
        unlockDoor();
        isLocked = false;
      } else {
        lcd.print("Locked");
        lockDoor();
        isLocked = true;
      }
      stateStartTime = millis();
      currentState = STATE_STATUS;
    } else {
      lcd.print("Incorrect");
      tries--;
      if (tries == 0) {
        alert = true;
        currentState = STATE_ALERT;
      }
      stateStartTime = millis();
      currentState = STATE_STATUS;
    }
    while (data_count != 0) {
      data[--data_count] = 0;
    }
  }
}

void unlockDoor() {
  myservo.write(130);
}

void lockDoor() {
  myservo.write(10);
}

bool isMasterCard() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != masterUID[i]) return false;
  }
  return true;
}

bool isUserCard() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != userUID[i]) return false;
  }
  return true;
}
