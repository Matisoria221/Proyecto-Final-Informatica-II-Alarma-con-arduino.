# 🛡️ Arduino Home Alarm System / Sistema de Alarma Domiciliaria con Arduino

## 📖 English

### 📌 Project Overview
This project is a **home security alarm system** based on an **Arduino Uno**, designed to detect motion through Hc-sr501 Sr501 PIR sensors and trigger an alarm signal. The system integrates a **4x4 matrix keypad**, a **16x2 I2C LCD display**, and a **buzzer** to simulate the alarm sound.

Communication with a computer is established through a serial connection, where a Processing program allows the user to **activate/deactivate the alarm, log all events with date and time**, and **set up automatic activation** within a specified time range through a **graphical user interface**.

### ⚙️ Components Used
- **Arduino Uno**  
- **2x PIR Motion Sensors HC-SR501**  
- **High-frequency Buzzer / Siren**  
- **16x2 Blue LCD Display (HD44780 with I2C module)**  
- **4x4 Matrix Keypad**  
- **Serial communication with a Processing application**
- **2N2222 transisto**
- **2 × 330 Ω resistor** 

## 🛠️ Main Features
- ✅ 2 Hc-sr501 Sr501 PIR motion sensors for intrusion detection.
- ✅ Siren as an acoustic alert system.
- ✅ **Bidirectional serial communication** between Arduino and Processing.
- ✅ Graphical interface in Processing to:
  - Manual activation/deactivation via keypad or PC interface.
  - Event logging (activation, motion detected, deactivation, etc.) with timestamps.
  - Automatic activation scheduling
- ✅ Modular structure to add more sensors or actuators.

 ## 🚀 How to Run
- 1_Upload the Arduino sketch to the board.
- 2_Open and run the Processing program.
- 3_Set the correct serial port in the code.
- 4_Power the system and test the keypad and PIR sensors.

## ⚙️ Installation & Usage
1. Clone this repository:
   ```bash
   git clone https://github.com/Matisoria221/Proyecto-Final-Informatica-II-Alarma-con-arduino..git

## Esquematic
<img width="511" height="503" alt="image" src="https://github.com/user-attachments/assets/a2d8df76-4687-4724-8952-7927e2b7f7c9" />



---
## 📖 Español

### 📌 Descripción del Proyecto
Este proyecto consiste en el desarrollo de una **alarma domiciliaria** basada en **Arduino Uno**, diseñada para detectar intrusiones mediante sensores PIR HC-SR501 y activar una señal de alarma. El sistema integra un **teclado matricial 4x4**, un **display LCD 16x2 con interfaz I2C**, y un **buzzer** que simula el sonido de la alarma.

La comunicación con una computadora se realiza por **puerto serial**, donde un programa desarrollado en **Processing** permite **activar o desactivar la alarma**, **registrar eventos** con **fecha y hora**, y **configurar la activación automática** dentro de un rango horario definido por el usuario.

### ⚙️ Componentes Utilizados
- **Arduino Uno**  
- **2x Sensores de Movimiento PIR HC-SR501**  
- **Sirena o Buzzer de alta frecuencia**  
- **Display LCD azul 16x2 (HD44780 con módulo I2C)**  
- **Teclado Matricial 4x4**  
- **Comunicación serial con software en Processing**
- **Transistor 2N2222**
- **2 resistencias de 330 Ω**
  
## 🛠️ Características principales
- ✅ 2 sensores de movimiento (Hc-sr501 Sr501 PIR) para detección de intrusos.  
- ✅ Sirena como sistema de aviso acústico.  
- ✅ Comunicación **bidireccional por puerto serie** entre Arduino y Processing.  
- ✅ Interfaz gráfica en Processing para:  
  - Activación/desactivación manual mediante teclado o interfaz gráfica en PC. 
  - Registro de eventos (activación, detección de movimiento, desactivación, etc.) con marca temporal. 
  - Configuración horaria para activación automática.  
- ✅ Estructura modular para añadir más sensores o actuadores.

## 🚀 Cómo ejecutar
- 1_Cargar el código de Arduino en la placa.
- 2_Ejecutar el programa de Processing en la PC.
- 3_Configurar el puerto serial correcto en el código.
- 4_Encender el sistema y probar las funciones del teclado y sensores PIR.  

## ⚙️ Instalación y uso
1. Clonar este repositorio:
   ```bash
   git clone https://github.com/Matisoria221/Proyecto-Final-Informatica-II-Alarma-con-arduino..git

## Esquemático
<img width="511" height="503" alt="image" src="https://github.com/user-attachments/assets/e8f54eb6-be41-42c9-8d5b-8d24b5acc7c1" />


