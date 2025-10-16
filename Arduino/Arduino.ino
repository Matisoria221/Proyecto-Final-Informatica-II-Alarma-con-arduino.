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
String comandoSerial = "";
bool comandoCompleto = false;

// Clase para Sensor PIR
class SensorPIR {
private:
  int pin;
public:
  SensorPIR(int p) {
    pin = p;
    pinMode(pin, INPUT);
  }

  bool detectarMovimiento() {
    if (digitalRead(pin) == HIGH) {
      ultimaDeteccion = millis();
      return true;
    }
    return false;
  }
};

// Objetos de sensores PIR
SensorPIR pir1(SENSOR_PIR1_PIN);
SensorPIR pir2(SENSOR_PIR2_PIN);

// Prototipos de funciones
void mostrarEstado();
void manejarComando(char tecla);
void solicitarContrasena(char nuevaAccion);
void manejarContrasena(char tecla);
void verificarContrasena();
void revisarSensores();
void mostrarMensaje(String mensaje, int tiempo);
void sonarBuzzerIntermitente();
void verificarAutoActivacion();
void procesarComandoSerial();
void enviarEstadoSerial();
void enviarEvento(String evento);

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
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      comandoCompleto = true;
    } else {
      comandoSerial += c;
    }
  }
  
  // Procesar comando serial si está completo
  if (comandoCompleto) {
    procesarComandoSerial();
    comandoSerial = "";
    comandoCompleto = false;
  }
  
  // Leer teclado físico
  char tecla = pad.getKey();
  if (tecla != NO_KEY) {
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

// ====================================================================
// Funciones de Comunicación Serial
// ====================================================================

void procesarComandoSerial() {
  comandoSerial.trim();
  
  if (comandoSerial == "GET_STATUS") {
    enviarEstadoSerial();
  }
  else if (comandoSerial.startsWith("KEY:")) {//Analiza con que empieza el string recibido
    // Simular presión de tecla desde Processing
    char tecla = comandoSerial.charAt(4);//Extrae el caracter en la posición 4 del string 
    if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
  }
  else if (comandoSerial.startsWith("SET_AUTO:")) {//Analiza con que empieza el string recibido
    // Habilitar/deshabilitar auto-activación
    int valor = comandoSerial.substring(9).toInt();
    autoActivacionHabilitada = (valor == 1);
    enviarEvento(autoActivacionHabilitada ? "AUTO_ACTIVACION_HABILITADA" : "AUTO_ACTIVACION_DESHABILITADA");
    enviarEstadoSerial();
  }
  else if (comandoSerial.startsWith("SET_TIME:")) {
    // Configurar tiempo de inactividad en minutos
    int minutos = comandoSerial.substring(9).toInt();
    TIEMPO_INACTIVIDAD_PARA_ACTIVAR = (unsigned long)minutos * 60000;
    enviarEvento("TIEMPO_INACTIVIDAD_CONFIGURADO:" + String(minutos));
    enviarEstadoSerial();
  }
}

void enviarEstadoSerial() {
  // Formato: STATUS:alarmaActivada,alarmaDisparada,autoActivacionHabilitada,tiempoInactividad
  Serial.print("STATUS:");
  Serial.print(alarmaActivada ? "Activa" : "Inactiva");
  Serial.print(",");
  Serial.print(alarmaDisparada ? "Disparada" : "No disparada");
  Serial.print(",");
  Serial.print(autoActivacionHabilitada ? "Autoactivacion ON" : "Autoactivacion OFF");
  Serial.print(",");
  Serial.println(TIEMPO_INACTIVIDAD_PARA_ACTIVAR / 60000 ); // Enviar en minutos
}

void enviarEvento(String evento) {
  Serial.print("EVENT:");
  Serial.println(evento);
}

// ====================================================================
// Funciones de Alarma y Sensores
// ====================================================================
void revisarSensores() {
  if (pir1.detectarMovimiento() || pir2.detectarMovimiento()) {
    if (!alarmaDisparada) {
      alarmaDisparada = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INTRUSION!");
      lcd.setCursor(0, 1);
      lcd.print("D=Desactivar");
      enviarEvento("INTRUSION_DETECTADA");
      enviarEstadoSerial();
    }
  }
}

void verificarAutoActivacion() {
  if (!alarmaActivada && !pidendoContrasena) {
    if (millis() - ultimaDeteccion > TIEMPO_INACTIVIDAD_PARA_ACTIVAR) {
      alarmaActivada = true;
      alarmaDisparada = false;
      mostrarMensaje("AUTO-ACTIVADA", 2000);
      enviarEvento("ALARMA_AUTO_ACTIVADA");
      enviarEstadoSerial();
    }
  }
}

void sonarBuzzerIntermitente() {
  static unsigned long tiempoAnterior = 0;
  const long intervalo = 200;
  static bool buzzerEstado = LOW;

  if (millis() - tiempoAnterior >= intervalo) {
    tiempoAnterior = millis();
    buzzerEstado = !buzzerEstado;
    digitalWrite(BUZZER_PIN, buzzerEstado);
  }
}

// ====================================================================
// Funciones de Keypad y Contraseña
// ====================================================================
void manejarComando(char tecla) {
  if (tecla == 'A' && !alarmaActivada) {
    solicitarContrasena('A');
  }
  else if (tecla == 'D' && (alarmaActivada || alarmaDisparada)) {
    solicitarContrasena('D');
  }
  else if (tecla == 'A' && alarmaActivada) {
    mostrarMensaje("Ya esta activa", 1500);
  }
  else if (tecla == 'D' && !alarmaActivada) {
    mostrarMensaje("Ya esta inactiva", 1500);
  }
}

void solicitarContrasena(char nuevaAccion) {
  accion = nuevaAccion;
  pidendoContrasena = true;
  cont = 0;
  memset(ingresado, 0, sizeof(ingresado));
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese clave:");
  lcd.setCursor(0, 1);
  lcd.print("Clave: ");
  
  enviarEvento(nuevaAccion == 'A' ? "SOLICITANDO_ACTIVACION" : "SOLICITANDO_DESACTIVACION");
}

void manejarContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    ingresado[cont] = tecla;
    cont++;
        
    lcd.setCursor(6 + cont - 1, 1);
    lcd.print('*');
        
    if (cont == 4) {
      verificarContrasena();
    }
  }
  else if (tecla == '#') {
    pidendoContrasena = false;
    enviarEvento("INGRESO_CANCELADO");
    mostrarEstado();
  }
}

void verificarContrasena() {
  bool correcta = true;
  for (int i = 0; i < 4; i++) {
    if (ingresado[i] != contrasena[i]) {
      correcta = false;
      break;
    }
  }
    
  if (correcta) {
    if (accion == 'A') {
      alarmaActivada = true;
      alarmaDisparada = false;
      mostrarMensaje("ACTIVADA!", 2000);
      enviarEvento("ALARMA_ACTIVADA");
    } else {
      alarmaActivada = false;
      alarmaDisparada = false;
      digitalWrite(BUZZER_PIN, LOW);
      mostrarMensaje("DESACTIVADA!", 2000);
      enviarEvento("ALARMA_DESACTIVADA");
      ultimaDeteccion = millis();
    }
    intentos = 0;
    enviarEstadoSerial();
  } else {
    intentos++;
    enviarEvento("CLAVE_INCORRECTA:INTENTO_" + String(intentos));
    if (intentos >= 3) {
      mostrarMensaje("Bloqueado 10s", 10000);
      enviarEvento("SISTEMA_BLOQUEADO");
      intentos = 0;
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Clave Incorrecta");
      lcd.setCursor(0, 1);
      lcd.print("Intentos: ");
      lcd.print(3 - intentos);
      delay(2000);
    }
  }
    
  pidendoContrasena = false;
  mostrarEstado();
}

// ====================================================================
// Funciones de Display
// ====================================================================
void mostrarEstado() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (alarmaDisparada) {
    lcd.print("ALRMA: INTRUSION!");
  } else {
    if (alarmaActivada) {
      lcd.print("ALARMA: ACTIVADA");
    } else {
      lcd.print("ALARMA: INACTIVA");
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("A=Act D=Desact");
}

void mostrarMensaje(String mensaje, int tiempo) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mensaje);
  delay(tiempo);
  mostrarEstado();
}