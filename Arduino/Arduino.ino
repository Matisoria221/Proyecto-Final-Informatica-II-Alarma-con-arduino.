#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definiciones de pines
#define BUZZER_PIN 13
#define SENSOR_PIR1_PIN 2
#define SENSOR_PIR2_PIN 3
#define SENSOR_OTROS_PIN 12 // Usaremos este pin para otros sensores o un sensor PIR genérico

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
bool alarmaActivada = false; // Indica si el sistema está armado
bool alarmaDisparada = false; // Indica si se detectó una intrusión
bool pidendoContrasena = false;
char accion = ' '; // 'A'=activar, 'D'=desactivar

// Auto-activación por inactividad
unsigned long ultimaDeteccion = 0;
const unsigned long TIEMPO_INACTIVIDAD_PARA_ACTIVAR = 3600000; // 1 hora en milisegundos

// Clase para Sensor PIR
class SensorPIR {
private:
  int pin;
public:
  SensorPIR(int p) {
    pin = p;
    pinMode(pin, INPUT);
  }

  // Verifica si hay movimiento y actualiza el tiempo de última detección
  bool detectarMovimiento() {
    if (digitalRead(pin) == HIGH) {
      ultimaDeteccion = millis(); // Actualiza el tiempo de la última detección
      return true;
    }
    return false;
  }
};

// Objetos de sensores PIR usando la clase
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

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
    
  lcd.init();
  lcd.backlight();
  mostrarEstado();
  // Inicializar ultimaDeteccion con el tiempo actual para evitar activación inmediata
  ultimaDeteccion = millis(); 
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
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
    // Si la alarma NO está activada, verificar si debe auto-activarse
    verificarAutoActivacion();
  }

  // Hacer sonar el buzzer si la alarma está disparada
  if (alarmaDisparada) {
    sonarBuzzerIntermitente();
  }
}

// ====================================================================
// Funciones de Alarma y Sensores
// ====================================================================
void revisarSensores() {
  // Us0 los métodos de la clase para revisar los sensores.
  // El método 'detectarMovimiento' se encarga de actualizar 'ultimaDeteccion'
  if (pir1.detectarMovimiento() || pir2.detectarMovimiento()) {
    // Intrusion detectada
    if (!alarmaDisparada) { // Solo si no estaba ya disparada
      alarmaDisparada = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INTRUSION!");
      lcd.setCursor(0, 1);
      lcd.print("D=Desactivar");
      Serial.println("ALARMA DISPARADA: MOVIMIENTO DETECTADO");
    }
    // El buzzer se maneja en loop() mientras alarmaDisparada es true
  }
}

void verificarAutoActivacion() {
  // Si la alarma no está activada y no se está pidiendo contraseña
  if (!alarmaActivada && !pidendoContrasena) {
    // Comprobar si ha pasado el tiempo de inactividad
    if (millis() - ultimaDeteccion > TIEMPO_INACTIVIDAD_PARA_ACTIVAR) {
      // Auto-activar alarma
      alarmaActivada = true;
      alarmaDisparada = false; // Asegurar que no está disparada
      mostrarMensaje("AUTO-ACTIVADA", 2000);
      Serial.println("ALARMA AUTO-ACTIVADA POR INACTIVIDAD");
    }
  }
}

void sonarBuzzerIntermitente() {
  // El buzzer suena de forma intermitente (patrón de 200ms encendido/apagado)
  static unsigned long tiempoAnterior = 0;
  const long intervalo = 200; // 200 milisegundos para el pulso
  static bool buzzerEstado = LOW;

  if (millis() - tiempoAnterior >= intervalo) {
    tiempoAnterior = millis(); // Guarda el tiempo actual

    if (buzzerEstado == LOW) {
      buzzerEstado = HIGH;
    } else {
      buzzerEstado = LOW;
    }
    
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
  else if (tecla == 'D' && (alarmaActivada || alarmaDisparada)) { // Permitir desactivar si está activa o disparada
    solicitarContrasena('D');
  }
  //Contempla casos redundantes o posibles errores del usuario
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
  memset(ingresado, 0, sizeof(ingresado));//Asigna memoría en bytes 
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese clave:");
  lcd.setCursor(0, 1);
  lcd.print("Clave: ");
}

void manejarContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    ingresado[cont] = tecla;
    cont++;
        
    // Mostrar asteriscos
    lcd.setCursor(6 + cont - 1, 1);
    lcd.print('*');
        
    if (cont == 4) {
      verificarContrasena();
    }
  }
  else if (tecla == '#') {
    // Cancelar
    pidendoContrasena = false;
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
      alarmaDisparada = false; // Resetear el estado disparado
      mostrarMensaje("ACTIVADA!", 2000);
    } else {
      alarmaActivada = false;
      alarmaDisparada = false; // Resetear el estado disparado
      digitalWrite(BUZZER_PIN, LOW); // Apagar buzzer si estaba sonando
      mostrarMensaje("DESACTIVADA!", 2000);
      // Al desactivar, actualizamos la última detección para reiniciar el conteo
      ultimaDeteccion = millis(); 
    }
    intentos = 0;
  } else {
    intentos++;
    if (intentos >= 3) {
      mostrarMensaje("Bloqueado 5s", 5000);
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