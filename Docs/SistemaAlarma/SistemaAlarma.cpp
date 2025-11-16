/**
 * @file SistemaAlarma.cpp
 * @brief Implementación de la librería del Sistema de Alarma
 * @version 2.0
 * @date 2024
 * 
 * Este archivo contiene la implementación completa de todas las clases
 * declaradas en SistemaAlarma.h. Debe colocarse en la carpeta de la librería
 * junto con el archivo de cabecera.
 */

#include "SistemaAlarma.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// Variables globales externas (definidas en el sketch principal)
extern unsigned long ultimaDeteccion;

// ====================================================================
// IMPLEMENTACIÓN: SensorPIR
// ====================================================================

bool SensorPIR::detectarMovimiento() {
  if (digitalRead(pin) == HIGH) {
    ultimaDeteccion = millis();
    return true;
  }
  return false;
}

// ====================================================================
// IMPLEMENTACIÓN: GestorBuzzer
// ====================================================================

GestorBuzzer::GestorBuzzer(byte p, unsigned int interval) 
  : pin(p), intervalo(interval), tiempoAnterior(0), buzzerEstado(LOW) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void GestorBuzzer::apagar() {
  digitalWrite(pin, LOW);
  buzzerEstado = LOW;
}

void GestorBuzzer::actualizar() {
  if (millis() - tiempoAnterior >= intervalo) {
    tiempoAnterior = millis();
    buzzerEstado = !buzzerEstado;
    digitalWrite(pin, buzzerEstado);
  }
}

void GestorBuzzer::setIntervalo(unsigned int interval) {
  intervalo = interval;
}

// ====================================================================
// IMPLEMENTACIÓN: GestorEEPROM
// ====================================================================

void GestorEEPROM::guardarContrasena(char* contrasena) {
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(ADDR_PASS + i, contrasena[i]);
  }
  EEPROM.write(ADDR_MAGIC, MAGIC_VALUE);
}

bool GestorEEPROM::cargarContrasena(char* contrasena) {
  if (EEPROM.read(ADDR_MAGIC) == MAGIC_VALUE) {
    // Cargar contraseña guardada
    for (byte i = 0; i < 4; i++) {
      contrasena[i] = EEPROM.read(ADDR_PASS + i);
    }
    return true;
  } else {
    // Primera vez, contraseña por defecto
    contrasena[0] = '1';
    contrasena[1] = '2';
    contrasena[2] = '3';
    contrasena[3] = '4';
    guardarContrasena(contrasena);
    return false;
  }
}

// ====================================================================
// IMPLEMENTACIÓN: GestorDisplay
// ====================================================================

GestorDisplay::GestorDisplay(LiquidCrystal_I2C* lcdRef) 
  : lcd(lcdRef), tiempoMensaje(0), duracionMensaje(0), mostrandoMensaje(false) {
}

void GestorDisplay::mostrarEstado(bool alarmaActivada, bool alarmaDisparada) {
  lcd->clear();
  lcd->setCursor(0, 0);
  
  if (alarmaDisparada) {
    lcd->print(F("ALRMA: INTRUSION!"));
  } else {
    if (alarmaActivada) {
      lcd->print(F("ALARMA: ACTIVADA"));
    } else {
      lcd->print(F("ALARMA: INACTIVA"));
    }
  }
  
  lcd->setCursor(0, 1);
  lcd->print(F("A=Act D=Desact"));
}

void GestorDisplay::mostrarMensaje(const __FlashStringHelper* mensaje, unsigned int tiempo) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(mensaje);
  
  tiempoMensaje = millis();
  duracionMensaje = tiempo;
  mostrandoMensaje = true;
}

bool GestorDisplay::actualizar(bool alarmaActivada, bool alarmaDisparada) {
  if (mostrandoMensaje && (millis() - tiempoMensaje >= duracionMensaje)) {
    mostrandoMensaje = false;
    mostrarEstado(alarmaActivada, alarmaDisparada);
    return true;
  }
  return false;
}

void GestorDisplay::mostrarIngresoContrasena(byte asteriscos) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(F("Ingrese clave:"));
  lcd->setCursor(0, 1);
  lcd->print(F("Clave: "));
  
  for (byte i = 0; i < asteriscos; i++) {
    lcd->print('*');
  }
}

void GestorDisplay::mostrarCambioContrasena(byte fase, byte asteriscos, byte intentos) {
  lcd->clear();
  lcd->setCursor(0, 0);
  
  if (fase == 0) {
    lcd->print(F("Clave actual:"));
    lcd->setCursor(0, 1);
    lcd->print(F("Clave: "));
    
    for (byte i = 0; i < asteriscos; i++) {
      lcd->print('*');
    }
  } else if (fase == 1) {
    lcd->print(F("Nueva clave:"));
    lcd->setCursor(0, 1);
    lcd->print(F("Clave: "));
    
    for (byte i = 0; i < asteriscos; i++) {
      lcd->print('*');
    }
  }
}

void GestorDisplay::mostrarBloqueo(byte segundosRestantes) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(F("SISTEMA BLOQ!"));
  lcd->setCursor(0, 1);
  lcd->print(F("INTRUSO! "));
  lcd->print(segundosRestantes);
  lcd->print(F("s"));
}

void GestorDisplay::limpiar() {
  lcd->clear();
}

// ====================================================================
// IMPLEMENTACIÓN: GestorComunicacion
// ====================================================================

GestorComunicacion::GestorComunicacion() {
  memset(buffer, 0, sizeof(buffer));
}

void GestorComunicacion::enviarEstado(bool alarmaActivada, bool alarmaDisparada, 
                                      bool autoActivacion, unsigned long tiempoInactividad) {
  Serial.print(F("STATUS:"));
  Serial.print(alarmaActivada ? F("Activa") : F("Inactiva"));
  Serial.print(F(","));
  Serial.print(alarmaDisparada ? F("Disparada") : F("No disparada"));
  Serial.print(F(","));
  Serial.print(autoActivacion ? F("Autoactivacion ON") : F("Autoactivacion OFF"));
  Serial.print(F(","));
  Serial.println(tiempoInactividad / 60000UL);
}

void GestorComunicacion::enviarEvento(const __FlashStringHelper* evento) {
  Serial.print(F("EVENT:HARDWARE:"));
  Serial.println(evento);
}

void GestorComunicacion::enviarContrasenaInicial(char* contrasena) {
  Serial.print(F("INIT_PASS:"));
  for (byte i = 0; i < 4; i++) {
    Serial.print(contrasena[i]);
  }
  Serial.println();
}

void GestorComunicacion::enviarCambioContrasena(char* nuevaContrasena) {
  Serial.print(F("NEW_PASS:"));
  for (byte i = 0; i < 4; i++) {
    Serial.print(nuevaContrasena[i]);
  }
  Serial.println();
}

char* GestorComunicacion::leerComando() {
  if (Serial.available() > 0) {
    byte bytesLeidos = Serial.readBytesUntil('\n', buffer, 31);
    if (bytesLeidos > 0) {
      buffer[bytesLeidos] = '\0';
      // Limpiar espacios y retornos de carro
      char* cmd = buffer;
      while (*cmd == ' ' || *cmd == '\r') cmd++;
      return cmd;
    }
  }
  return NULL;
}

// ====================================================================
// IMPLEMENTACIÓN: GestorContrasena
// ====================================================================

GestorContrasena::GestorContrasena() 
  : contIngresado(0), intentosFallidos(0), intentosCambioPass(0), 
    faseCambioPass(0), bloqueado(false), bloqueadoCambioPass(false), 
    tiempoBloqueo(0) {
  memset(contrasena, 0, sizeof(contrasena));
  memset(ingresado, 0, sizeof(ingresado));
  memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
  memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
}

void GestorContrasena::setContrasena(char* nuevaPass) {
  for (byte i = 0; i < 4; i++) {
    contrasena[i] = nuevaPass[i];
  }
}

void GestorContrasena::getContrasena(char* dest) {
  for (byte i = 0; i < 4; i++) {
    dest[i] = contrasena[i];
  }
}

bool GestorContrasena::agregarDigito(char digito) {
  if (contIngresado < 4) {
    ingresado[contIngresado] = digito;
    contIngresado++;
    return (contIngresado == 4);
  }
  return false;
}

bool GestorContrasena::verificarContrasena() {
  bool correcta = true;
  
  for (byte i = 0; i < 4; i++) {
    if (ingresado[i] != contrasena[i]) {
      correcta = false;
      break;
    }
  }
  
  if (!correcta) {
    intentosFallidos++;
    if (intentosFallidos >= MAX_INTENTOS) {
      bloqueado = true;
      tiempoBloqueo = millis();
      intentosFallidos = 0;
    }
  } else {
    intentosFallidos = 0;
  }
  
  return correcta;
}

void GestorContrasena::reiniciarIngreso() {
  contIngresado = 0;
  memset(ingresado, 0, sizeof(ingresado));
}

byte GestorContrasena::getDigitosIngresados() {
  return contIngresado;
}

byte GestorContrasena::getIntentosFallidos() {
  return intentosFallidos;
}

bool GestorContrasena::estaBloqueado() {
  return bloqueado || bloqueadoCambioPass;
}

bool GestorContrasena::verificarBloqueo(bool& alarmaActivadaAntes) {
  if (bloqueadoCambioPass) {
    if (millis() - tiempoBloqueo >= TIEMPO_BLOQUEO_CAMBIO) {
      bloqueadoCambioPass = false;
      intentosCambioPass = 0;
      return true;
    }
  } else if (bloqueado) {
    if (millis() - tiempoBloqueo >= TIEMPO_BLOQUEO_NORMAL) {
      bloqueado = false;
      return true;
    }
  }
  return false;
}

byte GestorContrasena::getSegundosBloqueoRestantes() {
  if (bloqueadoCambioPass) {
    unsigned long tiempoTranscurrido = millis() - tiempoBloqueo;
    if (tiempoTranscurrido < TIEMPO_BLOQUEO_CAMBIO) {
      return (TIEMPO_BLOQUEO_CAMBIO - tiempoTranscurrido) / 1000;
    }
  } else if (bloqueado) {
    unsigned long tiempoTranscurrido = millis() - tiempoBloqueo;
    if (tiempoTranscurrido < TIEMPO_BLOQUEO_NORMAL) {
      return (TIEMPO_BLOQUEO_NORMAL - tiempoTranscurrido) / 1000;
    }
  }
  return 0;
}

void GestorContrasena::iniciarCambioContrasena() {
  faseCambioPass = 0;
  contIngresado = 0;
  intentosCambioPass = 0;
  memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
  memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
}

byte GestorContrasena::procesarCambioContrasena(char digito, GestorDisplay& display,
                                                GestorEEPROM& eeprom, 
                                                GestorComunicacion& comunicacion,
                                                bool& alarmaActivadaAntes) {
  if (faseCambioPass == 0) {
    // Fase 0: Verificar contraseña actual
    contrasenaVerificacion[contIngresado] = digito;
    contIngresado++;
    
    if (contIngresado == 4) {
      bool correcta = true;
      for (byte i = 0; i < 4; i++) {
        if (contrasenaVerificacion[i] != contrasena[i]) {
          correcta = false;
          break;
        }
      }
      
      if (correcta) {
        // Contraseña correcta, pasar a fase 1
        faseCambioPass = 1;
        contIngresado = 0;
        intentosCambioPass = 0;
        memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
        
        display.mostrarCambioContrasena(1, 0, 0);
        comunicacion.enviarEvento(F("CONTRASENA_ACTUAL_VERIFICADA"));
        return 0; // Continuar
      } else {
        // Contraseña incorrecta
        intentosCambioPass++;
        comunicacion.enviarEvento(F("VERIFICACION_FALLIDA_CAMBIO_PASS"));
        
        if (intentosCambioPass >= MAX_INTENTOS) {
          // Bloquear sistema
          bloqueadoCambioPass = true;
          tiempoBloqueo = millis();
          comunicacion.enviarEvento(F("SISTEMA_BLOQUEADO_CAMBIO_PASS_ALARMA_ACTIVADA"));
          return 2; // Bloqueado
        } else {
          // Mostrar error y reintentar
          contIngresado = 0;
          memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
          return 3; // Error, reintentar
        }
      }
    }
  } else if (faseCambioPass == 1) {
    // Fase 1: Ingresar nueva contraseña
    nuevaContrasena[contIngresado] = digito;
    contIngresado++;
    
    if (contIngresado == 4) {
      // Guardar nueva contraseña
      for (byte i = 0; i < 4; i++) {
        contrasena[i] = nuevaContrasena[i];
      }
      
      eeprom.guardarContrasena(contrasena);
      comunicacion.enviarCambioContrasena(contrasena);
      comunicacion.enviarEvento(F("CONTRASENA_CAMBIADA_EXITOSAMENTE"));
      
      // Resetear variables
      faseCambioPass = 0;
      contIngresado = 0;
      intentosCambioPass = 0;
      memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
      memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
      
      return 1; // Completado exitosamente
    }
  }
  
  return 0; // Continuar
}

void GestorContrasena::cancelarCambioContrasena() {
  faseCambioPass = 0;
  contIngresado = 0;
  intentosCambioPass = 0;
  memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
  memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
}

bool GestorContrasena::estaCambiandoContrasena() {
  return (faseCambioPass == 0 || faseCambioPass == 1) && contIngresado < 4;
}

byte GestorContrasena::getFaseCambio() {
  return faseCambioPass;
}

bool GestorContrasena::estaBloqueadoCambioPass() {
  return bloqueadoCambioPass;
}

void GestorContrasena::setBloqueo(bool estado, bool activarAlarma, bool& alarmaActivadaAntes) {
  if (estado) {
    bloqueadoCambioPass = true;
    tiempoBloqueo = millis();
  } else {
    bloqueadoCambioPass = false;
  }
}