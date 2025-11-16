/**
 * @file Sistema_Alarma_Principal.ino
 * @brief Sistema de Alarma con Sensores PIR - Código Principal
 * @version 2.0
 * @date 2024
 * 
 * Sistema completo de alarma con:
 * - Teclado matricial 4x4
 * - Display LCD I2C 16x2
 * - 2 Sensores PIR de movimiento
 * - Buzzer de alerta
 * - Interfaz serial con Processing
 * - Almacenamiento de contraseña en EEPROM
 * - Auto-activación por inactividad configurable
 * - Bloqueo por intentos fallidos (30 segundos)
 */

// ====================================================================
// INCLUSIÓN DE LIBRERÍAS
// ====================================================================
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "SistemaAlarma.h"

// ====================================================================
// DEFINICIONES DE PINES
// ====================================================================
#define BUZZER_PIN 13        ///< Pin del buzzer
#define SENSOR_OTROS_PIN 12  ///< Pin sensor adicional (reservado)
#define SENSOR_PIR2_PIN 3    ///< Pin segundo sensor PIR
#define SENSOR_PIR1_PIN 2    ///< Pin primer sensor PIR

// ====================================================================
// CONFIGURACIÓN DEL TECLADO MATRICIAL
// ====================================================================
const byte FILAS = 4;
const byte COLUMNAS = 4;

/// Disposición del teclado
char TECLADO[FILAS][COLUMNAS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

/// Pines de las filas del teclado
byte FILAS_PINS[FILAS] = { 11, 10, 9, 8 };

/// Pines de las columnas del teclado
byte COLUMNAS_PINS[COLUMNAS] = { 7, 6, 5, 4 };

/// Objeto Keypad
Keypad pad = Keypad(makeKeymap(TECLADO), FILAS_PINS, COLUMNAS_PINS, FILAS, COLUMNAS);

// ====================================================================
// OBJETOS GLOBALES DEL SISTEMA
// ====================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);  ///< Display LCD I2C

// Gestores del sistema
GestorDisplay display(&lcd);
GestorBuzzer buzzer(BUZZER_PIN);
GestorEEPROM gestorEEPROM;
GestorComunicacion comunicacion;
GestorContrasena gestorPass;

// Sensores PIR
SensorPIR pir1(SENSOR_PIR1_PIN);
SensorPIR pir2(SENSOR_PIR2_PIN);

// ====================================================================
// VARIABLES DE ESTADO DEL SISTEMA
// ====================================================================
bool alarmaActivada = false;        ///< Estado de activación de la alarma
bool alarmaDisparada = false;       ///< Estado de disparo de la alarma
bool alarmaActivadaAntes = false;   ///< Estado previo (para restaurar después de bloqueo)
bool pidendoContrasena = false;     ///< Flag de solicitud de contraseña
bool cambiandoContrasena = false;   ///< Flag de cambio de contraseña en proceso
char accion = ' ';                  ///< Acción pendiente ('A'=Activar, 'D'=Desactivar)

// ====================================================================
// AUTO-ACTIVACIÓN POR INACTIVIDAD
// ====================================================================
unsigned long ultimaDeteccion = 0;                           ///< Timestamp última detección
unsigned long TIEMPO_INACTIVIDAD_PARA_ACTIVAR = 3600000UL;  ///< 60 minutos por defecto
bool autoActivacionHabilitada = false;                      ///< Flag de auto-activación

// ====================================================================
// PROTOTIPOS DE FUNCIONES
// ====================================================================
void manejarComando(char tecla);
void solicitarContrasena(char nuevaAccion);
void manejarContrasena(char tecla);
void manejarCambioContrasena(char tecla);
void verificarContrasena();
void revisarSensores();
void verificarAutoActivacion();
void procesarComandoSerial(char* comando);
void verificarBloqueoSistema();

// ====================================================================
// SETUP - INICIALIZACIÓN DEL SISTEMA
// ====================================================================
void setup() {
  // Iniciar comunicación serial
  Serial.begin(9600);
  
  // Configurar pin del buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Inicializar LCD
  lcd.init();
  lcd.backlight();

  // Cargar contraseña desde EEPROM
  char tempPass[4];
  bool cargada = gestorEEPROM.cargarContrasena(tempPass);
  gestorPass.setContrasena(tempPass);

  // Mensaje de inicio
  display.mostrarMensaje(F("Iniciando..."), 2000);
  
  // Inicializar variables de tiempo
  ultimaDeteccion = millis();

  // Esperar un momento antes de comunicar
  delay(2000);
  
  // Enviar estado inicial a Processing
  comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                           autoActivacionHabilitada, 
                           TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  
  // Enviar contraseña inicial para sincronización
  gestorPass.getContrasena(tempPass);
  comunicacion.enviarContrasenaInicial(tempPass);
  
  // Evento de inicio
  comunicacion.enviarEvento(F("SISTEMA_INICIADO"));
  
  // Mostrar estado inicial
  display.mostrarEstado(alarmaActivada, alarmaDisparada);
}

// ====================================================================
// LOOP PRINCIPAL
// ====================================================================
void loop() {
  // Actualizar display (mensajes temporales)
  display.actualizar(alarmaActivada, alarmaDisparada);

  // Verificar y manejar bloqueos
  if (gestorPass.estaBloqueado()) {
    verificarBloqueoSistema();
    return;  // No procesar nada más mientras está bloqueado
  }

  // Procesar comandos seriales
  char* comando = comunicacion.leerComando();
  if (comando != NULL) {
    procesarComandoSerial(comando);
  }

  // Leer tecla del keypad
  char tecla = pad.getKey();
  if (tecla != NO_KEY) {
    ultimaDeteccion = millis();  // Actualizar actividad
    
    if (cambiandoContrasena) {
      manejarCambioContrasena(tecla);
    } else if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
  }

  // Revisar sensores si la alarma está activada
  if (alarmaActivada && !gestorPass.estaBloqueado()) {
    revisarSensores();
  } else {
    // Verificar auto-activación por inactividad
    if (autoActivacionHabilitada && !gestorPass.estaBloqueado()) {
      verificarAutoActivacion();
    }
  }

  // Hacer sonar el buzzer si la alarma está disparada
  if (alarmaDisparada) {
    buzzer.actualizar();
  }
}

// ====================================================================
// FUNCIONES DE MANEJO DE COMANDOS
// ====================================================================

/**
 * @brief Maneja los comandos del teclado en modo normal
 * @param tecla Carácter de la tecla presionada
 */
void manejarComando(char tecla) {
  if (tecla == 'C' && !gestorPass.estaBloqueadoCambioPass()) {
    // Iniciar cambio de contraseña
    cambiandoContrasena = true;
    gestorPass.iniciarCambioContrasena();
    
    display.mostrarCambioContrasena(0, 0, 0);
    comunicacion.enviarEvento(F("INICIANDO_CAMBIO_CONTRASENA"));
  }
  else if (tecla == 'A' && !alarmaActivada) {
    // Solicitar activación
    solicitarContrasena('A');
  }
  else if (tecla == 'D' && (alarmaActivada || alarmaDisparada)) {
    // Solicitar desactivación
    solicitarContrasena('D');
  }
  else if (tecla == 'A' && alarmaActivada) {
    display.mostrarMensaje(F("Ya esta activa"), 1500);
  }
  else if (tecla == 'D' && !alarmaActivada) {
    display.mostrarMensaje(F("Ya esta inactiva"), 1500);
  }
  else if (tecla == 'C' && gestorPass.estaBloqueadoCambioPass()) {
    display.mostrarMensaje(F("Bloqueado!"), 1500);
  }
}

/**
 * @brief Solicita el ingreso de contraseña para una acción
 * @param nuevaAccion Acción a realizar ('A' o 'D')
 */
void solicitarContrasena(char nuevaAccion) {
  accion = nuevaAccion;
  pidendoContrasena = true;
  gestorPass.reiniciarIngreso();
  
  display.mostrarIngresoContrasena(0);
  
  if (nuevaAccion == 'A') {
    comunicacion.enviarEvento(F("SOLICITANDO_ACTIVACION"));
  } else {
    comunicacion.enviarEvento(F("SOLICITANDO_DESACTIVACION"));
  }
}

/**
 * @brief Maneja el ingreso de contraseña
 * @param tecla Carácter de la tecla presionada
 */
void manejarContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    bool completa = gestorPass.agregarDigito(tecla);
    display.mostrarIngresoContrasena(gestorPass.getDigitosIngresados());
    
    if (completa) {
      verificarContrasena();
    }
  }
  else if (tecla == '#') {
    // Cancelar
    pidendoContrasena = false;
    comunicacion.enviarEvento(F("INGRESO_CANCELADO"));
    display.mostrarEstado(alarmaActivada, alarmaDisparada);
  }
}

/**
 * @brief Verifica la contraseña ingresada y ejecuta la acción
 */
void verificarContrasena() {
  if (gestorPass.verificarContrasena()) {
    // Contraseña correcta
    if (accion == 'A') {
      alarmaActivada = true;
      alarmaDisparada = false;
      display.mostrarMensaje(F("ACTIVADA!"), 2000);
      comunicacion.enviarEvento(F("ALARMA_ACTIVADA"));
    } else {
      alarmaActivada = false;
      alarmaDisparada = false;
      buzzer.apagar();
      display.mostrarMensaje(F("DESACTIVADA!"), 2000);
      comunicacion.enviarEvento(F("ALARMA_DESACTIVADA"));
      ultimaDeteccion = millis();
    }
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  } else {
    // Contraseña incorrecta
    byte intentos = gestorPass.getIntentosFallidos();
    comunicacion.enviarEvento(F("CLAVE_INCORRECTA"));
    
    if (gestorPass.estaBloqueado()) {
      // Sistema bloqueado por 30 segundos
      alarmaActivadaAntes = alarmaActivada;
      alarmaActivada = true;
      alarmaDisparada = true;
      
      display.mostrarBloqueo(30);
      comunicacion.enviarEvento(F("SISTEMA_BLOQUEADO"));
      comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                               autoActivacionHabilitada, 
                               TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
    } else {
      // Mostrar intentos restantes
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Clave Incorrecta"));
      lcd.setCursor(0, 1);
      lcd.print(F("Intentos: "));
      lcd.print(3 - intentos);
      
      delay(2000);
      display.mostrarEstado(alarmaActivada, alarmaDisparada);
    }
  }
  
  pidendoContrasena = false;
  gestorPass.reiniciarIngreso();
}

/**
 * @brief Maneja el proceso de cambio de contraseña
 * @param tecla Carácter de la tecla presionada
 */
void manejarCambioContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    byte resultado = gestorPass.procesarCambioContrasena(tecla, display, 
                                                         gestorEEPROM, 
                                                         comunicacion,
                                                         alarmaActivadaAntes);
    
    // Actualizar display según la fase
    byte fase = gestorPass.getFaseCambio();
    display.mostrarCambioContrasena(fase, gestorPass.getDigitosIngresados(), 0);
    
    if (resultado == 1) {
      // Cambio completado exitosamente
      cambiandoContrasena = false;
      display.mostrarMensaje(F("Clave cambiada!"), 2000);
    }
    else if (resultado == 2) {
      // Sistema bloqueado por 30 segundos
      cambiandoContrasena = false;
      alarmaActivadaAntes = alarmaActivada;
      alarmaActivada = true;
      alarmaDisparada = true;
      
      display.mostrarBloqueo(30);
      comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                               autoActivacionHabilitada, 
                               TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
    }
    else if (resultado == 3) {
      // Error, mostrar intentos restantes
      byte intentos = 3 - gestorPass.getIntentosFallidos();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Clave incorrecta"));
      lcd.setCursor(0, 1);
      lcd.print(F("Intentos: "));
      lcd.print(intentos);
      
      delay(2000);
      display.mostrarCambioContrasena(0, 0, 0);
    }
  }
  else if (tecla == '#') {
    // Cancelar cambio de contraseña
    cambiandoContrasena = false;
    gestorPass.cancelarCambioContrasena();
    display.mostrarMensaje(F("Cancelado"), 1500);
    comunicacion.enviarEvento(F("CAMBIO_CONTRASENA_CANCELADO"));
  }
}

// ====================================================================
// FUNCIONES DE SENSORES Y AUTO-ACTIVACIÓN
// ====================================================================

/**
 * @brief Revisa los sensores PIR y dispara la alarma si detecta movimiento
 */
void revisarSensores() {
  if (pir1.detectarMovimiento() || pir2.detectarMovimiento()) {
    if (!alarmaDisparada) {
      alarmaDisparada = true;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("INTRUSION!"));
      lcd.setCursor(0, 1);
      lcd.print(F("D=Desactivar"));
      
      comunicacion.enviarEvento(F("INTRUSION_DETECTADA"));
      comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                               autoActivacionHabilitada, 
                               TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
    }
  }
}

/**
 * @brief Verifica si debe auto-activar la alarma por inactividad
 */
void verificarAutoActivacion() {
  if (!alarmaActivada && !pidendoContrasena) {
    if (millis() - ultimaDeteccion > TIEMPO_INACTIVIDAD_PARA_ACTIVAR) {
      alarmaActivada = true;
      alarmaDisparada = false;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("AUTO-ACTIVADA!"));
      lcd.setCursor(0, 1);
      lcd.print(F("Sistema armado"));
      
      delay(2000);
      display.mostrarEstado(alarmaActivada, alarmaDisparada);
      
      comunicacion.enviarEvento(F("ALARMA_AUTO_ACTIVADA"));
      comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                               autoActivacionHabilitada, 
                               TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
      
      ultimaDeteccion = millis();
    }
  }
}

// ====================================================================
// FUNCIÓN DE VERIFICACIÓN DE BLOQUEO
// ====================================================================

/**
 * @brief Verifica y maneja el estado de bloqueo del sistema
 * 
 * Actualiza el countdown en pantalla y libera el bloqueo cuando expira.
 * Durante el bloqueo, mantiene la alarma disparada y el buzzer sonando.
 */
void verificarBloqueoSistema() {
  if (gestorPass.verificarBloqueo(alarmaActivadaAntes)) {
    // Bloqueo finalizado
    alarmaActivada = alarmaActivadaAntes;
    alarmaDisparada = false;
    buzzer.apagar();
    
    display.mostrarMensaje(F("Bloqueo finalizado"), 2000);
    comunicacion.enviarEvento(F("BLOQUEO_FINALIZADO"));
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  } else {
    // Actualizar countdown cada segundo
    static unsigned long ultimaActualizacion = 0;
    if (millis() - ultimaActualizacion >= 1000) {
      ultimaActualizacion = millis();
      byte segundosRestantes = gestorPass.getSegundosBloqueoRestantes();
      display.mostrarBloqueo(segundosRestantes);
    }
    
    // Mantener buzzer sonando durante el bloqueo
    buzzer.actualizar();
  }
}

// ====================================================================
// FUNCIÓN DE PROCESAMIENTO DE COMANDOS SERIALES
// ====================================================================

/**
 * @brief Procesa comandos recibidos por serial desde Processing
 * @param comando Comando recibido (sin espacios iniciales)
 */
void procesarComandoSerial(char* comando) {
  if (strcmp(comando, "GET_STATUS") == 0) {
    // Solicitud de estado
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  }
  else if (strncmp(comando, "KEY:", 4) == 0) {
    // Simular pulsación de tecla desde Processing
    char tecla = comando[4];
    if (cambiandoContrasena) {
      manejarCambioContrasena(tecla);
    } else if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
    delay(50);
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  }
  else if (strncmp(comando, "SET_AUTO:", 9) == 0) {
    // Configurar auto-activación
    int valor = atoi(&comando[9]);
    autoActivacionHabilitada = (valor == 1);
    comunicacion.enviarEvento(autoActivacionHabilitada ? 
                             F("AUTO_ACTIVACION_HABILITADA") : 
                             F("AUTO_ACTIVACION_DESHABILITADA"));
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  }
  else if (strncmp(comando, "SET_TIME:", 9) == 0) {
    // Configurar tiempo de inactividad
    int minutos = atoi(&comando[9]);
    TIEMPO_INACTIVIDAD_PARA_ACTIVAR = (unsigned long)minutos * 60000UL;
    comunicacion.enviarEstado(alarmaActivada, alarmaDisparada, 
                             autoActivacionHabilitada, 
                             TIEMPO_INACTIVIDAD_PARA_ACTIVAR);
  }
  else if (strncmp(comando, "SET_PASS:", 9) == 0) {
    // Recibir nueva contraseña desde Processing
    char nuevaPassTemp[4];
    for (byte i = 0; i < 4; i++) {
      nuevaPassTemp[i] = comando[9 + i];
    }
    
    // Verificar si es diferente para evitar loops
    char passActual[4];
    gestorPass.getContrasena(passActual);
    
    bool esDiferente = false;
    for (byte i = 0; i < 4; i++) {
      if (nuevaPassTemp[i] != passActual[i]) {
        esDiferente = true;
        break;
      }
    }
    
    if (esDiferente) {
      gestorPass.setContrasena(nuevaPassTemp);
      gestorEEPROM.guardarContrasena(nuevaPassTemp);
      comunicacion.enviarEvento(F("CONTRASENA_ACTUALIZADA_DESDE_SOFTWARE"));
      display.mostrarMensaje(F("Clave guardada!"), 2000);
    }
  }
}