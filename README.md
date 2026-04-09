# 3D Time-of-Flight Scanning System

## Overview
This project implements a 3D scanning system using a Time-of-Flight (ToF) sensor and embedded microcontroller. The system captures distance measurements across multiple angles and reconstructs a 3D representation of the environment.

## Features
- 360° scanning using stepper motor control
- Distance measurement via VL53L1X ToF sensor (I2C)
- Real-time UART communication to PC (115200 baud)
- Multi-layer 3D reconstruction in MATLAB
- Hardware timing verification using Analog Discovery 3

## Hardware
- TM4C1294 Microcontroller
- VL53L1X ToF Sensor
- Stepper Motor + Driver
- On Board Push Buttons (PJ0, PJ1)

## Software
- Embedded C (Keil uVision)
- MATLAB (3D Visualization)

## How It Works
1. Press button to start scan
2. Motor rotates and captures 32 distance points
3. Data transmitted via UART
4. MATLAB converts to XYZ coordinates
5. 3D model is generated


## Author
Seher
