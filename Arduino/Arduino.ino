//inclusión de librerias generales
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Definiciones de pines
#define BUZZER_PIN 13
#define SENSOR_OTROS_PIN 12
#define SENSOR_PIR2_PIN 3
#define SENSOR_PIR1_PIN 2

// Constantes para Keypad
const int FILAS = 4;
const int COLUMNAS = 4;
char TECLADO[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte FILAS_PINS[FILAS] = {11, 10, 9, 8};
byte COLUMNAS_PINS[COLUMNAS] = {7, 6, 5, 4};

// Objeto Keypad
Keypad pad = Keypad(makeKeymap(TECLADO), FILAS_PINS, COLUMNAS_PINS, FILAS, COLUMNAS);

// Objeto LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Contraseña y variables de estado
char contrasena[] = "1234";
char ingresado[5] = "";
int cont = 0;
int intentos = 0;
bool alarmaActivada = false;
bool alarmaDisparada = false;
bool pidendoContrasena = false;
char accion = ' ';

// Auto-activación por inactividad
unsigned long ultimaDeteccion = 0;
unsigned long TIEMPO_INACTIVIDAD_PARA_ACTIVAR = 3600000; // 1 hora por defecto
bool autoActivacionHabilitada = true; // Controla si la auto-activación está habilitada

// Variables para comunicación serial
char bufferSerial[64];
String origenAccion = "HARDWARE"; // Puede ser "HARDWARE" o "SOFTWARE"
//Inclución de librería propia (Se añade al final por el uso de variables globales)
#include "Funciones.h"

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
    
  lcd.init();
  lcd.backlight();
  mostrarEstado();
  ultimaDeteccion = millis();
  
  // Enviar estado inicial
  delay(1000);
  enviarEstadoSerial();
  enviarEvento("SISTEMA_INICIADO");
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
  // Leer comandos seriales
  if (Serial.available() > 0) {
    size_t bytesLeidos = Serial.readBytesUntil('\n', bufferSerial, 63);
    if (bytesLeidos > 0) {
      bufferSerial[bytesLeidos] = '\0';
      procesarComandoSerial(bufferSerial);
    }
  }
 
  // Leer teclado físico
  char tecla = pad.getKey();
  if (tecla != NO_KEY) {
    origenAccion = "HARDWARE"; // Marcar que viene del teclado físico
    if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
  }

   // Revisar sensores solo si alarma está activada
  if (alarmaActivada) {
    revisarSensores();
  } else {
    if (autoActivacionHabilitada) {
      verificarAutoActivacion();
    }
  }

  // Hacer sonar el buzzer si la alarma está disparada
  if (alarmaDisparada) {
    sonarBuzzerIntermitente();
  }
}
