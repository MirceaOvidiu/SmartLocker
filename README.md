# Smart Locker

[![Smart Locker Demo](https://img.youtube.com/vi/Mho3pYF5UIk/0.jpg)](https://www.youtube.com/watch?v=Mho3pYF5UIk)

*Click the image above to watch the demo video*

**Version:** 11.05  
*1st assignment version. General details, hardware implementation, and most software implementation are done. Work on other milestones is in progress.*

---

## Introduction

**Smart Locker** is a secure, Arduino-based safe inspired by EasyBox lockers. It is designed to protect valuable items and can be adapted to lock/unlock doors, drawers, and more.

## Description and Purpose

Smart Locker provides a robust solution for securing items using a combination of a servo-driven locking mechanism, keypad, RFID authentication, and sensors. Its modular design allows for easy adaptation to various locking needs.

## Functionality

- Unlock with correct PIN or RFID card
- 3 wrong PINs trigger the buzzer alarm
- Status displayed for each PIN attempt (“Correct!” / “X tries remaining”)
- PIR sensor detects presence near the keypad
- Weight sensor detects forced entry (high pressure triggers alarm)
- PIN reset possible via RFID card
- Servo motor actuates the lock mechanism

## General Description

A servo motor with a rack-and-pinion mechanism locks/unlocks the door. Users can enter a PIN via the keypad or use an RFID card to open the locker. The system monitors for forced entry using a weight sensor and triggers an alarm if tampering is detected or after multiple failed attempts.

## Hardware Design

**Components:**
- SG90 Micro Servo Motor
- 4×4 Matrix Keypad
- RFID-RC522 Communication Module
- LCD with Integrated I2C Module
- Arduino UNO R3
- Breadboard & Jumper Wires
- Active Buzzer
- LED
- PIR Sensor
- Weight Sensor

**Note:** Some components (e.g., RC544 RFID, weight sensor module) may not be available in TinkerCad.
