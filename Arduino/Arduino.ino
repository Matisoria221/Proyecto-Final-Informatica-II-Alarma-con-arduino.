//inclusión de librerias generales
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Definiciones de pines
#define BUZZER_PIN 13
#define SENSOR_OTROS_PIN 12
#define SENSOR_PIR2_PIN 3
#define SENSOR_PIR1_PIN 2

// Direcciones EEPROM
#define EEPROM_PASS_ADDR 0
#define EEPROM_MAGIC_ADDR 4
#define EEPROM_MAGIC_VALUE 0xAB

// Constantes para Keypad
const byte FILAS = 4;
const byte COLUMNAS = 4;
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

//Variables de contraseña
char contrasena[4];
char ingresado[4];
bool cambiandoContrasena = false;
char nuevaContrasena[4];
char contrasenaVerificacion[4];
byte contNuevaPass = 0;
byte faseCambioPass = 0;
byte intentosCambioPass = 0;

//variables de estado 
bool bloqueadoCambioPass = false;
unsigned long tiempoBloqueoPass = 0;
bool alarmaActivadaAntes = false;
byte cont = 0;
byte intentos = 0;
bool alarmaActivada = false;
bool alarmaDisparada = false;
bool pidendoContrasena = false;
char accion = ' ';

// Variables para mensajes no bloqueantes
unsigned long tiempoMensaje = 0;
unsigned int duracionMensaje = 0;
bool mostrandoMensaje = false;

// Auto-activación por inactividad
unsigned long ultimaDeteccion = 0;
unsigned long TIEMPO_INACTIVIDAD_PARA_ACTIVAR = 3600000UL;
bool autoActivacionHabilitada = false;

// Variables para comunicación serial (buffer reducido)
char bufferSerial[32];

//Inclución de librería propia (Se añade al final por el uso de variables globales)
#include "Funciones.h"

// ====================================================================
// FUNCIONES EEPROM
// ====================================================================
void guardarContrasenaEEPROM() {
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(EEPROM_PASS_ADDR + i, contrasena[i]);
  }
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
}

void cargarContrasenaEEPROM() {
  // Verificar si hay una contraseña guardada
  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VALUE) {
    // Cargar contraseña desde EEPROM
    for (byte i = 0; i < 4; i++) {
      contrasena[i] = EEPROM.read(EEPROM_PASS_ADDR + i);
    }
  } else {
    // Primera vez, establecer contraseña predeterminada 1234
    contrasena[0] = '1';
    contrasena[1] = '2';
    contrasena[2] = '3';
    contrasena[3] = '4';
    guardarContrasenaEEPROM();
  }
}

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Asegurar que el buzzer esté apagado
    
  lcd.init();
  lcd.backlight();
  
  // Cargar contraseña desde EEPROM
  cargarContrasenaEEPROM();
  
  // Inicializar estados de alarma
  alarmaActivada = false;
  alarmaDisparada = false;
  bloqueadoCambioPass = false;
  
  mostrarEstado();
  ultimaDeteccion = millis();
  
  // Enviar estado inicial y contraseña a Processing
  delay(1000);
  enviarEstadoSerial();
  
  // Enviar contraseña actual a Processing para sincronización inicial
  Serial.print(F("INIT_PASS:"));
  for (byte i = 0; i < 4; i++) {
    Serial.print(contrasena[i]);
  }
  Serial.println();
  
  enviarEvento(F("SISTEMA_INICIADO"));
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
  // Actualizar mensaje temporizado
  if (mostrandoMensaje && (millis() - tiempoMensaje >= duracionMensaje)) {
    mostrandoMensaje = false;
    mostrarEstado();
  }
  
  // Verificar bloqueo de cambio de contraseña
  if (bloqueadoCambioPass) {
    verificarCambioContrasena();
    return; // No hacer nada más mientras está bloqueado
  }
  
  // Leer comandos seriales (sin bloqueo)
  if (Serial.available() > 0) {
    byte bytesLeidos = Serial.readBytesUntil('\n', bufferSerial, 31);
    if (bytesLeidos > 0) {
      bufferSerial[bytesLeidos] = '\0';
      procesarComandoSerial(bufferSerial);
    }
  }
  
  // Leer tecla para acciones normales
  char tecla = pad.getKey();
  if (tecla != NO_KEY) {
    if (cambiandoContrasena) {
      manejarCambioContrasena(tecla);
    } else if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
  }
  
  // Revisar sensores
  if (alarmaActivada && !bloqueadoCambioPass) {
    revisarSensores();
  } else {
    if (autoActivacionHabilitada && !bloqueadoCambioPass) {
      verificarAutoActivacion();
    }
  }
  
  // Hacer sonar el buzzer
  if (alarmaDisparada) {
    sonarBuzzerIntermitente();
  }
}