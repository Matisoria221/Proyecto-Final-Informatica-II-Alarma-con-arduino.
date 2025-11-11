// Clase para Sensor PIR
class SensorPIR {
private:
  byte pin;
public:
  SensorPIR(byte p) : pin(p) {
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
void manejarCambioContrasena(char tecla);
void verificarContrasena();
void revisarSensores();
void mostrarMensaje(const __FlashStringHelper* mensaje, unsigned int tiempo);
void sonarBuzzerIntermitente();
void verificarAutoActivacion();
void procesarComandoSerial(char* comando);
void enviarEstadoSerial();
void enviarEvento(const __FlashStringHelper* evento);
void guardarContrasenaEEPROM();

// ====================================================================
// Funciones de Comunicación Serial
// ====================================================================
void procesarComandoSerial(char* comando) {
  while (*comando == ' ' || *comando == '\r') comando++;
  
  if (strcmp(comando, "GET_STATUS") == 0) {
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "KEY:", 4) == 0) {
    char tecla = comando[4];
    if (cambiandoContrasena) {
      manejarCambioContrasena(tecla);
    } else if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
    delay(50);
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_AUTO:", 9) == 0) {
    int valor = atoi(&comando[9]);
    autoActivacionHabilitada = (valor == 1);
    enviarEvento(autoActivacionHabilitada ? F("AUTO_ACTIVACION_HABILITADA") : F("AUTO_ACTIVACION_DESHABILITADA"));
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_TIME:", 9) == 0) {
    int minutos = atoi(&comando[9]);
    TIEMPO_INACTIVIDAD_PARA_ACTIVAR = (unsigned long)minutos * 60000UL;
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_PASS:", 9) == 0) {
    // Recibir nueva contraseña desde Processing (solo cuando Processing la cambia)
    char nuevaPassTemp[4];
    for (byte i = 0; i < 4; i++) {
      nuevaPassTemp[i] = comando[9 + i];
    }
    
    // Verificar si es diferente a la actual para evitar loops
    bool esDiferente = false;
    for (byte i = 0; i < 4; i++) {
      if (nuevaPassTemp[i] != contrasena[i]) {
        esDiferente = true;
        break;
      }
    }
    
    if (esDiferente) {
      for (byte i = 0; i < 4; i++) {
        contrasena[i] = nuevaPassTemp[i];
      }
      guardarContrasenaEEPROM();
      enviarEvento(F("CONTRASENA_ACTUALIZADA_DESDE_SOFTWARE"));
      mostrarMensaje(F("Clave guardada!"), 2000);
    }
  }
}

void enviarEstadoSerial() {
  Serial.print(F("STATUS:"));
  Serial.print(alarmaActivada ? F("Activa") : F("Inactiva"));
  Serial.print(F(","));
  Serial.print(alarmaDisparada ? F("Disparada") : F("No disparada"));
  Serial.print(F(","));
  Serial.print(autoActivacionHabilitada ? F("Autoactivacion ON") : F("Autoactivacion OFF"));
  Serial.print(F(","));
  Serial.println(TIEMPO_INACTIVIDAD_PARA_ACTIVAR / 60000UL);
}

void enviarEvento(const __FlashStringHelper* evento) {
  Serial.print(F("EVENT:HARDWARE:"));
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
      lcd.print(F("INTRUSION!"));
      lcd.setCursor(0, 1);
      lcd.print(F("D=Desactivar"));
      enviarEvento(F("INTRUSION_DETECTADA"));
      enviarEstadoSerial();
    }
  }
}

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
      
      tiempoMensaje = millis();
      duracionMensaje = 2000;
      mostrandoMensaje = true;
      
      enviarEvento(F("ALARMA_AUTO_ACTIVADA"));
      enviarEstadoSerial();
      
      ultimaDeteccion = millis();
    }
  }
}

void sonarBuzzerIntermitente() {
  static unsigned long tiempoAnterior = 0;
  static bool buzzerEstado = LOW;

  if (millis() - tiempoAnterior >= 200) {
    tiempoAnterior = millis();
    buzzerEstado = !buzzerEstado;
    digitalWrite(BUZZER_PIN, buzzerEstado);
  }
}

// ====================================================================
// Funciones de Keypad y Contraseña
// ====================================================================
void manejarComando(char tecla) {
  ultimaDeteccion = millis();
  
  if (tecla == 'C' && !bloqueadoCambioPass) {
    // Iniciar cambio de contraseña con una sola pulsación
    cambiandoContrasena = true;
    faseCambioPass = 0;
    contNuevaPass = 0;
    intentosCambioPass = 0;
    mostrandoMensaje = false;
    
    memset(contrasenaVerificacion, 0, 4);
    memset(nuevaContrasena, 0, 4);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Clave actual:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Clave: "));
    
    enviarEvento(F("INICIANDO_CAMBIO_CONTRASENA"));
  }
  else if (tecla == 'A' && !alarmaActivada) {
    solicitarContrasena('A');
  }
  else if (tecla == 'D' && (alarmaActivada || alarmaDisparada)) {
    solicitarContrasena('D');
  }
  else if (tecla == 'A' && alarmaActivada) {
    mostrarMensaje(F("Ya esta activa"), 1500);
  }
  else if (tecla == 'D' && !alarmaActivada) {
    mostrarMensaje(F("Ya esta inactiva"), 1500);
  }
  else if (tecla == 'C' && bloqueadoCambioPass) {
    mostrarMensaje(F("Bloqueado!"), 1500);
  }
}

void solicitarContrasena(char nuevaAccion) {
  accion = nuevaAccion;
  pidendoContrasena = true;
  cont = 0;
  memset(ingresado, 0, 4);
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Ingrese clave:"));
  lcd.setCursor(0, 1);
  lcd.print(F("Clave: "));
  
  enviarEvento(nuevaAccion == 'A' ? F("SOLICITANDO_ACTIVACION") : F("SOLICITANDO_DESACTIVACION"));
}

void verificarCambioContrasena() {
  if (bloqueadoCambioPass) {
    if (millis() - tiempoBloqueoPass >= 30000UL) {
      bloqueadoCambioPass = false;
      intentosCambioPass = 0;
      
      alarmaActivada = alarmaActivadaAntes;
      alarmaDisparada = false;
      digitalWrite(BUZZER_PIN, LOW);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Bloqueo"));
      lcd.setCursor(0, 1);
      lcd.print(F("Finalizado"));
      
      tiempoMensaje = millis();
      duracionMensaje = 2000;
      mostrandoMensaje = true;
      
      enviarEvento(F("BLOQUEO_CAMBIO_PASS_FINALIZADO"));
      enviarEstadoSerial();
    } else {
      static unsigned long ultimaActualizacion = 0;
      if (millis() - ultimaActualizacion >= 1000) {
        ultimaActualizacion = millis();
        byte segundosRestantes = (30000UL - (millis() - tiempoBloqueoPass)) / 1000;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("SISTEMA BLOQ!"));
        lcd.setCursor(0, 1);
        lcd.print(F("INTRUSO! "));
        lcd.print(segundosRestantes);
        lcd.print(F("s"));
      }
      
      // Asegurar que el buzzer suene durante el bloqueo
      sonarBuzzerIntermitente();
    }
    return;
  }
}

void manejarCambioContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    if (faseCambioPass == 0) {
      contrasenaVerificacion[contNuevaPass] = tecla;
      contNuevaPass++;
      
      lcd.setCursor(7 + contNuevaPass - 1, 1);
      lcd.print('*');
      
      if (contNuevaPass == 4) {
        bool correcta = true;
        for (byte i = 0; i < 4; i++) {
          if (contrasenaVerificacion[i] != contrasena[i]) {
            correcta = false;
            break;
          }
        }
        
        if (correcta) {
          faseCambioPass = 1;
          contNuevaPass = 0;
          intentosCambioPass = 0;
          memset(nuevaContrasena, 0, 4);
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(F("Nueva clave:"));
          lcd.setCursor(0, 1);
          lcd.print(F("Clave: "));
          enviarEvento(F("CONTRASENA_ACTUAL_VERIFICADA"));
        } else {
          intentosCambioPass++;
          enviarEvento(F("VERIFICACION_FALLIDA_CAMBIO_PASS"));
          
          if (intentosCambioPass >= 3) {
            bloqueadoCambioPass = true;
            tiempoBloqueoPass = millis();
            
            alarmaActivadaAntes = alarmaActivada;
            alarmaActivada = true;
            alarmaDisparada = true;
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("SISTEMA BLOQ!"));
            lcd.setCursor(0, 1);
            lcd.print(F("INTRUSO! 30s"));
            
            enviarEvento(F("SISTEMA_BLOQUEADO_CAMBIO_PASS_ALARMA_ACTIVADA"));
            enviarEstadoSerial();
            
            cambiandoContrasena = false;
            faseCambioPass = 0;
            contNuevaPass = 0;
            memset(contrasenaVerificacion, 0, 4);
            memset(nuevaContrasena, 0, 4);
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Clave incorrecta"));
            lcd.setCursor(0, 1);
            lcd.print(F("Intentos: "));
            lcd.print(3 - intentosCambioPass);
            
            tiempoMensaje = millis();
            duracionMensaje = 2000;
            mostrandoMensaje = true;
            
            contNuevaPass = 0;
            memset(contrasenaVerificacion, 0, 4);
          }
        }
      }
    }
    else if (faseCambioPass == 1) {
      nuevaContrasena[contNuevaPass] = tecla;
      contNuevaPass++;
      
      lcd.setCursor(7 + contNuevaPass - 1, 1);
      lcd.print('*');
      
      if (contNuevaPass == 4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Guardando..."));
        
        // Copiar la nueva contraseña
        for (byte i = 0; i < 4; i++) {
          contrasena[i] = nuevaContrasena[i];
        }
        
        // Guardar en EEPROM
        guardarContrasenaEEPROM();
        
        // Enviar al Processing
        Serial.print(F("NEW_PASS:"));
        for (byte i = 0; i < 4; i++) {
          Serial.print(contrasena[i]);
        }
        Serial.println();
        
        mostrarMensaje(F("Clave cambiada!"), 2000);
        enviarEvento(F("CONTRASENA_CAMBIADA_EXITOSAMENTE"));
        
        cambiandoContrasena = false;
        faseCambioPass = 0;
        contNuevaPass = 0;
        intentosCambioPass = 0;
        memset(contrasenaVerificacion, 0, 4);
        memset(nuevaContrasena, 0, 4);
      }
    }
  }
  else if (tecla == '#') {
    cambiandoContrasena = false;
    faseCambioPass = 0;
    contNuevaPass = 0;
    intentosCambioPass = 0;
    memset(contrasenaVerificacion, 0, 4);
    memset(nuevaContrasena, 0, 4);
    mostrarMensaje(F("Cancelado"), 1500);
    enviarEvento(F("CAMBIO_CONTRASENA_CANCELADO"));
  }
}

void manejarContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    ingresado[cont] = tecla;
    cont++;
        
    lcd.setCursor(7 + cont - 1, 1);
    lcd.print('*');
        
    if (cont == 4) {
      verificarContrasena();
    }
  }
  else if (tecla == '#') {
    pidendoContrasena = false;
    enviarEvento(F("INGRESO_CANCELADO"));
    mostrarEstado();
  }
}

void verificarContrasena() {
  bool correcta = true;
  for (byte i = 0; i < 4; i++) {
    if (ingresado[i] != contrasena[i]) {
      correcta = false;
      break;
    }
  }
    
  if (correcta) {
    if (accion == 'A') {
      alarmaActivada = true;
      alarmaDisparada = false;
      mostrarMensaje(F("ACTIVADA!"), 2000);
      enviarEvento(F("ALARMA_ACTIVADA"));
    } else {
      alarmaActivada = false;
      alarmaDisparada = false;
      digitalWrite(BUZZER_PIN, LOW);
      mostrarMensaje(F("DESACTIVADA!"), 2000);
      enviarEvento(F("ALARMA_DESACTIVADA"));
      ultimaDeteccion = millis();
    }
    intentos = 0;
    enviarEstadoSerial();
  } else {
    intentos++;
    enviarEvento(F("CLAVE_INCORRECTA"));
    if (intentos >= 3) {
      mostrarMensaje(F("Bloqueado 10s"), 10000);
      enviarEvento(F("SISTEMA_BLOQUEADO"));
      intentos = 0;
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Clave Incorrecta"));
      lcd.setCursor(0, 1);
      lcd.print(F("Intentos: "));
      lcd.print(3 - intentos);
      
      tiempoMensaje = millis();
      duracionMensaje = 2000;
      mostrandoMensaje = true;
    }
  }
    
  pidendoContrasena = false;
}

// ====================================================================
// Funciones de Display
// ====================================================================
void mostrarEstado() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (alarmaDisparada) {
    lcd.print(F("ALRMA: INTRUSION!"));
  } else {
    if (alarmaActivada) {
      lcd.print(F("ALARMA: ACTIVADA"));
    } else {
      lcd.print(F("ALARMA: INACTIVA"));
    }
  }
  lcd.setCursor(0, 1);
  lcd.print(F("A=Act D=Desact"));
}

void mostrarMensaje(const __FlashStringHelper* mensaje, unsigned int tiempo) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mensaje);
  
  tiempoMensaje = millis();
  duracionMensaje = tiempo;
  mostrandoMensaje = true;
}