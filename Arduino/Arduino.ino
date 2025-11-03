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

//Variables de contraseña
char ingresado[5] = "";
bool cambiandoContrasena = false;
char nuevaContrasena[5] = {0, 0, 0, 0, 0};
char contrasenaVerificacion[5] = {0, 0, 0, 0, 0};
int contNuevaPass = 0;
int faseCambioPass = 0; // 0 = verificar actual, 1 = ingresar nueva
int intentosCambioPass = 0;
//variables de estado 
bool bloqueadoCambioPass = false;
unsigned long tiempoBloqueoPass = 0;
const unsigned long DURACION_BLOQUEO_PASS = 30000; // 30 segundos
bool alarmaActivadaAntes = false; // Para restaurar estado después del bloqueo
unsigned long tiempoPulsacionC = 0;
const unsigned long TIEMPO_MANTENER_C = 3000; // 3 segundos
int cont = 0;
int intentos = 0;
bool alarmaActivada = false;
bool alarmaDisparada = false;
bool pidendoContrasena = false;
char accion = ' ';

// Auto-activación por inactividad
unsigned long ultimaDeteccion = 0;
unsigned long TIEMPO_INACTIVIDAD_PARA_ACTIVAR = 3600000; // 1 hora por defecto
bool autoActivacionHabilitada = false; // Controla si la auto-activación está habilitada

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
}