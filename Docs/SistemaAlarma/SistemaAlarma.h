/**
 * @file SistemaAlarma.h
 * @brief Librería del Sistema de Alarma con Sensores PIR
 * @version 2.0
 * @date 2024
 * 
 * Esta librería implementa un sistema completo de alarma con:
 * - Sensores de movimiento PIR
 * - Teclado matricial 4x4
 * - Display LCD I2C
 * - Buzzer de alerta
 * - Comunicación serial con interfaz Processing
 * - Gestión de contraseñas con EEPROM
 * - Auto-activación por inactividad
 */

#ifndef SISTEMA_ALARMA_H
#define SISTEMA_ALARMA_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// ====================================================================
// CLASE: SensorPIR
// ====================================================================
/**
 * @class SensorPIR
 * @brief Clase para manejar sensores de movimiento PIR
 * 
 * Encapsula la funcionalidad de un sensor PIR, permitiendo detectar
 * movimiento y actualizar el timestamp de última detección.
 */
class SensorPIR {
private:
  byte pin; ///< Pin digital al que está conectado el sensor

public:
  /**
   * @brief Constructor del sensor PIR
   * @param p Pin digital del sensor
   */
  SensorPIR(byte p) : pin(p) {
    pinMode(pin, INPUT);
  }

  /**
   * @brief Detecta movimiento en el sensor
   * @return true si se detecta movimiento, false en caso contrario
   * 
   * También actualiza la variable global ultimaDeteccion cuando
   * se detecta movimiento.
   */
  bool detectarMovimiento();
};

// ====================================================================
// CLASE: GestorBuzzer
// ====================================================================
/**
 * @class GestorBuzzer
 * @brief Clase para controlar el buzzer de manera no bloqueante
 * 
 * Permite hacer sonar el buzzer de forma intermitente sin bloquear
 * la ejecución del programa principal.
 */
class GestorBuzzer {
private:
  byte pin;                        ///< Pin digital del buzzer
  unsigned long tiempoAnterior;    ///< Timestamp del último cambio de estado
  bool buzzerEstado;               ///< Estado actual del buzzer (HIGH/LOW)
  unsigned int intervalo;          ///< Intervalo de intermitencia en ms

public:
  /**
   * @brief Constructor del gestor de buzzer
   * @param p Pin digital del buzzer
   * @param interval Intervalo de intermitencia en milisegundos (default: 200ms)
   */
  GestorBuzzer(byte p, unsigned int interval = 200);

  /**
   * @brief Apaga el buzzer
   */
  void apagar();

  /**
   * @brief Actualiza el estado del buzzer (intermitencia no bloqueante)
   * 
   * Debe llamarse repetidamente en el loop() para mantener la intermitencia
   */
  void actualizar();

  /**
   * @brief Establece el intervalo de intermitencia
   * @param interval Nuevo intervalo en milisegundos
   */
  void setIntervalo(unsigned int interval);
};

// ====================================================================
// CLASE: GestorEEPROM
// ====================================================================
/**
 * @class GestorEEPROM
 * @brief Clase para gestionar la contraseña en EEPROM
 * 
 * Maneja el almacenamiento persistente de la contraseña del sistema,
 * incluyendo verificación de integridad mediante un valor mágico.
 */
class GestorEEPROM {
private:
  static const byte ADDR_PASS = 0;      ///< Dirección base de la contraseña
  static const byte ADDR_MAGIC = 4;     ///< Dirección del valor mágico
  static const byte MAGIC_VALUE = 0xAB; ///< Valor mágico para verificación

public:
  /**
   * @brief Guarda la contraseña en EEPROM
   * @param contrasena Array de 4 caracteres con la contraseña
   */
  void guardarContrasena(char* contrasena);

  /**
   * @brief Carga la contraseña desde EEPROM
   * @param contrasena Array donde se almacenará la contraseña cargada
   * @return true si se cargó una contraseña válida, false si se usa la default
   * 
   * Si no hay contraseña guardada, establece "1234" como default
   */
  bool cargarContrasena(char* contrasena);
};

// ====================================================================
// CLASE: GestorDisplay
// ====================================================================
/**
 * @class GestorDisplay
 * @brief Clase para manejar el display LCD I2C
 * 
 * Centraliza todas las operaciones de visualización en el LCD,
 * incluyendo mensajes temporales no bloqueantes.
 */
class GestorDisplay {
private:
  LiquidCrystal_I2C* lcd;          ///< Puntero al objeto LCD
  unsigned long tiempoMensaje;     ///< Timestamp de inicio de mensaje temporal
  unsigned int duracionMensaje;    ///< Duración del mensaje temporal en ms
  bool mostrandoMensaje;           ///< Flag de mensaje temporal activo

public:
  /**
   * @brief Constructor del gestor de display
   * @param lcdRef Referencia al objeto LiquidCrystal_I2C
   */
  GestorDisplay(LiquidCrystal_I2C* lcdRef);

  /**
   * @brief Muestra el estado actual de la alarma
   * @param alarmaActivada Estado de activación de la alarma
   * @param alarmaDisparada Estado de disparo de la alarma
   */
  void mostrarEstado(bool alarmaActivada, bool alarmaDisparada);

  /**
   * @brief Muestra un mensaje temporal
   * @param mensaje Mensaje a mostrar (usar F() macro)
   * @param tiempo Duración en milisegundos
   */
  void mostrarMensaje(const __FlashStringHelper* mensaje, unsigned int tiempo);

  /**
   * @brief Actualiza el display (maneja mensajes temporales)
   * @param alarmaActivada Estado de activación de la alarma
   * @param alarmaDisparada Estado de disparo de la alarma
   * @return true si se debe actualizar el display, false si está mostrando mensaje
   */
  bool actualizar(bool alarmaActivada, bool alarmaDisparada);

  /**
   * @brief Muestra la pantalla de ingreso de contraseña
   * @param asteriscos Número de asteriscos a mostrar (dígitos ingresados)
   */
  void mostrarIngresoContrasena(byte asteriscos);

  /**
   * @brief Muestra la pantalla de cambio de contraseña
   * @param fase Fase del cambio (0=verificar actual, 1=nueva)
   * @param asteriscos Número de asteriscos a mostrar
   * @param intentos Número de intentos fallidos
   */
  void mostrarCambioContrasena(byte fase, byte asteriscos, byte intentos);

  /**
   * @brief Muestra pantalla de bloqueo con countdown
   * @param segundosRestantes Segundos restantes de bloqueo
   */
  void mostrarBloqueo(byte segundosRestantes);

  /**
   * @brief Limpia el display
   */
  void limpiar();
};

// ====================================================================
// CLASE: GestorComunicacion
// ====================================================================
/**
 * @class GestorComunicacion
 * @brief Clase para manejar la comunicación serial con Processing
 * 
 * Gestiona el protocolo de comunicación bidireccional con la
 * interfaz gráfica en Processing.
 */
class GestorComunicacion {
private:
  char buffer[32]; ///< Buffer para comandos seriales entrantes

public:
  /**
   * @brief Constructor del gestor de comunicación
   */
  GestorComunicacion();

  /**
   * @brief Envía el estado completo del sistema
   * @param alarmaActivada Estado de activación
   * @param alarmaDisparada Estado de disparo
   * @param autoActivacion Estado de auto-activación
   * @param tiempoInactividad Tiempo de inactividad en minutos
   */
  void enviarEstado(bool alarmaActivada, bool alarmaDisparada, 
                    bool autoActivacion, unsigned long tiempoInactividad);

  /**
   * @brief Envía un evento al sistema externo
   * @param evento Descripción del evento (usar F() macro)
   */
  void enviarEvento(const __FlashStringHelper* evento);

  /**
   * @brief Envía la contraseña inicial al conectar
   * @param contrasena Array de 4 caracteres con la contraseña
   */
  void enviarContrasenaInicial(char* contrasena);

  /**
   * @brief Envía notificación de cambio de contraseña
   * @param nuevaContrasena Array de 4 caracteres con la nueva contraseña
   */
  void enviarCambioContrasena(char* nuevaContrasena);

  /**
   * @brief Procesa comandos seriales entrantes
   * @return Puntero al comando procesado, NULL si no hay comando
   * 
   * Lee datos del puerto serial de forma no bloqueante
   */
  char* leerComando();
};

// ====================================================================
// CLASE: GestorContrasena
// ====================================================================
/**
 * @class GestorContrasena
 * @brief Clase para manejar la lógica de contraseñas
 * 
 * Gestiona la verificación, cambio y bloqueo de contraseñas,
 * incluyendo el manejo de intentos fallidos.
 */
class GestorContrasena {
private:
  char contrasena[4];              ///< Contraseña actual del sistema
  char ingresado[4];               ///< Contraseña ingresada temporalmente
  char nuevaContrasena[4];         ///< Nueva contraseña (durante cambio)
  char contrasenaVerificacion[4];  ///< Verificación de contraseña actual
  byte contIngresado;              ///< Contador de dígitos ingresados
  byte intentosFallidos;           ///< Contador de intentos fallidos
  byte intentosCambioPass;         ///< Intentos fallidos de cambio
  byte faseCambioPass;             ///< Fase del cambio de contraseña
  bool bloqueado;                  ///< Flag de sistema bloqueado
  bool bloqueadoCambioPass;        ///< Flag de bloqueo por cambio de pass
  unsigned long tiempoBloqueo;     ///< Timestamp de inicio de bloqueo
  
  static const unsigned int TIEMPO_BLOQUEO_NORMAL = 30000UL;  ///< 30 segundos
  static const unsigned int TIEMPO_BLOQUEO_CAMBIO = 30000UL;  ///< 30 segundos
  static const byte MAX_INTENTOS = 3; ///< Máximo de intentos antes de bloqueo

public:
  /**
   * @brief Constructor del gestor de contraseñas
   */
  GestorContrasena();

  /**
   * @brief Establece la contraseña del sistema
   * @param nuevaPass Array de 4 caracteres con la nueva contraseña
   */
  void setContrasena(char* nuevaPass);

  /**
   * @brief Obtiene la contraseña actual
   * @param dest Array destino donde copiar la contraseña
   */
  void getContrasena(char* dest);

  /**
   * @brief Agrega un dígito a la contraseña en ingreso
   * @param digito Carácter del dígito ('0'-'9')
   * @return true si la contraseña está completa (4 dígitos)
   */
  bool agregarDigito(char digito);

  /**
   * @brief Verifica si la contraseña ingresada es correcta
   * @return true si es correcta, false si es incorrecta
   * 
   * Maneja el conteo de intentos fallidos y bloqueo automático
   */
  bool verificarContrasena();

  /**
   * @brief Reinicia el ingreso de contraseña
   */
  void reiniciarIngreso();

  /**
   * @brief Obtiene el número de dígitos ingresados
   * @return Cantidad de dígitos ingresados (0-4)
   */
  byte getDigitosIngresados();

  /**
   * @brief Obtiene el número de intentos fallidos
   * @return Cantidad de intentos fallidos
   */
  byte getIntentosFallidos();

  /**
   * @brief Verifica si el sistema está bloqueado
   * @return true si está bloqueado, false en caso contrario
   */
  bool estaBloqueado();

  /**
   * @brief Verifica si el bloqueo ha expirado
   * @param alarmaActivadaAntes Estado previo de la alarma (para restaurar)
   * @return true si el bloqueo ha finalizado
   */
  bool verificarBloqueo(bool& alarmaActivadaAntes);

  /**
   * @brief Obtiene segundos restantes de bloqueo
   * @return Segundos restantes (0 si no está bloqueado)
   */
  byte getSegundosBloqueoRestantes();

  /**
   * @brief Inicia el proceso de cambio de contraseña
   */
  void iniciarCambioContrasena();

  /**
   * @brief Procesa un dígito durante el cambio de contraseña
   * @param digito Carácter del dígito ('0'-'9')
   * @param display Referencia al gestor de display
   * @param eeprom Referencia al gestor de EEPROM
   * @param comunicacion Referencia al gestor de comunicación
   * @param alarmaActivadaAntes Estado previo de la alarma
   * @return Estado: 0=continuar, 1=completado exitoso, 2=bloqueado, 3=error
   */
  byte procesarCambioContrasena(char digito, GestorDisplay& display,
                                GestorEEPROM& eeprom, 
                                GestorComunicacion& comunicacion,
                                bool& alarmaActivadaAntes);

  /**
   * @brief Cancela el proceso de cambio de contraseña
   */
  void cancelarCambioContrasena();

  /**
   * @brief Verifica si está en proceso de cambio de contraseña
   * @return true si está cambiando contraseña
   */
  bool estaCambiandoContrasena();

  /**
   * @brief Obtiene la fase actual de cambio de contraseña
   * @return 0=verificar actual, 1=ingresar nueva
   */
  byte getFaseCambio();

  /**
   * @brief Verifica si el bloqueo de cambio de contraseña está activo
   * @return true si está bloqueado por intentos de cambio
   */
  bool estaBloqueadoCambioPass();

  /**
   * @brief Establece el estado de bloqueo de cambio de contraseña
   * @param estado true para bloquear, false para desbloquear
   * @param activarAlarma true para activar alarma durante el bloqueo
   * @param alarmaActivadaAntes Estado previo de la alarma (para guardar)
   */
  void setBloqueo(bool estado, bool activarAlarma, bool& alarmaActivadaAntes);
};

#endif // SISTEMA_ALARMA_H