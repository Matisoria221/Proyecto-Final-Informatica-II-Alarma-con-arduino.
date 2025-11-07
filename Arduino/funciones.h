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
void manejarCambioContrasena(char tecla);
void verificarContrasena();
void revisarSensores();
void mostrarMensaje(String mensaje, int tiempo);
void sonarBuzzerIntermitente();
void verificarAutoActivacion();
void procesarComandoSerial(char* comando);
void enviarEstadoSerial();
void enviarEvento(String evento);

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
    delay(100);
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_AUTO:", 9) == 0) {
    origenAccion = "SOFTWARE";
    int valor = atoi(&comando[9]);
    autoActivacionHabilitada = (valor == 1);
    enviarEvento(autoActivacionHabilitada ? "AUTO_ACTIVACION_HABILITADA" : "AUTO_ACTIVACION_DESHABILITADA");
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_TIME:", 9) == 0) {
    origenAccion = "SOFTWARE";
    int minutos = atoi(&comando[9]);
    TIEMPO_INACTIVIDAD_PARA_ACTIVAR = (unsigned long)minutos * 60000;
    enviarEvento("TIEMPO_INACTIVIDAD_CONFIGURADO:" + String(minutos));
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_PASS:", 9) == 0) {
    // Recibir nueva contraseña desde Processing
    for (int i = 0; i < 4; i++) {
      contrasena[i] = comando[9 + i];
    }
    enviarEvento("CONTRASENA_ACTUALIZADA_DESDE_SOFTWARE");
    mostrarMensaje("Clave guardada!", 2000);
  }
}
////
void enviarEstadoSerial() {
  // Formato: STATUS:alarmaActivada,alarmaDisparada,autoActivacionHabilitada,tiempoInactividad
  Serial.print("STATUS:");
  Serial.print(alarmaActivada ? "Activa" : "Inactiva");
  Serial.print(",");
  Serial.print(alarmaDisparada ? "Disparada" : "No disparada");
  Serial.print(",");
  Serial.print(autoActivacionHabilitada ? "Autoactivacion ON" : "Autoactivacion OFF");
  Serial.print(",");
  Serial.println(TIEMPO_INACTIVIDAD_PARA_ACTIVAR / 60000); // Enviar en minutos
}
////
void enviarEvento(String evento) {
  Serial.print("EVENT:");
  Serial.print(origenAccion);
  Serial.print(":");
  Serial.println(evento);
  // Resetear origen a HARDWARE por defecto después de enviar
  origenAccion = "HARDWARE";
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
////
void verificarAutoActivacion() {
  if (!alarmaActivada && !pidendoContrasena) {
    if (millis() - ultimaDeteccion > TIEMPO_INACTIVIDAD_PARA_ACTIVAR) {
      // Auto-activar la alarma
      alarmaActivada = true;
      alarmaDisparada = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AUTO-ACTIVADA!");
      lcd.setCursor(0, 1);
      lcd.print("Sistema armado");
      delay(2000);
      
      enviarEvento("ALARMA_AUTO_ACTIVADA");
      enviarEstadoSerial();
      mostrarEstado();
      
      // Resetear timer
      ultimaDeteccion = millis();
    }
  }
}
////
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
  ultimaDeteccion = millis();
  
  // Detectar tecla C mantenida para cambiar contraseña
  if (tecla == 'C') {
    if (tiempoPulsacionC == 0) {
      tiempoPulsacionC = millis();
    }
  }
  else if (tecla == 'A' && !alarmaActivada) {
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
////
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
////
void verificarCambioContrasena() {
  // Verificar si está bloqueado por intentos fallidos
  if (bloqueadoCambioPass) {
    if (millis() - tiempoBloqueoPass >= DURACION_BLOQUEO_PASS) {
      // Finalizar bloqueo
      bloqueadoCambioPass = false;
      intentosCambioPass = 0;
      
      // Restaurar estado anterior de la alarma
      alarmaActivada = alarmaActivadaAntes;
      alarmaDisparada = false;
      digitalWrite(BUZZER_PIN, LOW);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bloqueo");
      lcd.setCursor(0, 1);
      lcd.print("Finalizado");
      delay(2000);
      
      enviarEvento("BLOQUEO_CAMBIO_PASS_FINALIZADO");
      enviarEstadoSerial();
      mostrarEstado();
    } else {
      // Actualizar cuenta regresiva en LCD
      static unsigned long ultimaActualizacion = 0;
      if (millis() - ultimaActualizacion >= 1000) {
        ultimaActualizacion = millis();
        int segundosRestantes = (DURACION_BLOQUEO_PASS - (millis() - tiempoBloqueoPass)) / 1000;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SISTEMA BLOQ!");
        lcd.setCursor(0, 1);
        lcd.print("INTRUSO! ");
        lcd.print(segundosRestantes);
        lcd.print("s");
      }
    }
    return; // No permitir verificar tecla C mientras está bloqueado
  }
}
////
void manejarCambioContrasena(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    if (faseCambioPass == 0) {
      // Fase 1: Verificar contraseña actual
      contrasenaVerificacion[contNuevaPass] = tecla;
      contNuevaPass++;
      
      lcd.setCursor(6 + contNuevaPass - 1, 1);
      lcd.print('*');
      
      if (contNuevaPass == 4) {
        // Verificar si la contraseña actual es correcta
        bool correcta = true;
        for (int i = 0; i < 4; i++) {
          if (contrasenaVerificacion[i] != contrasena[i]) {
            correcta = false;
            break;
          }
        }
        
        if (correcta) {
          // Contraseña correcta, pasar a la fase de nueva contraseña
          faseCambioPass = 1;
          contNuevaPass = 0;
          intentosCambioPass = 0;
          memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Nueva clave:");
          lcd.setCursor(0, 1);
          lcd.print("Clave: ");
          enviarEvento("CONTRASENA_ACTUAL_VERIFICADA");
        } else {
          // Contraseña incorrecta
          intentosCambioPass++;
          enviarEvento("VERIFICACION_FALLIDA_CAMBIO_PASS:INTENTO_" + String(intentosCambioPass));
          
          if (intentosCambioPass >= 3) {
            // BLOQUEAR SISTEMA Y ACTIVAR ALARMA DE SEGURIDAD
            bloqueadoCambioPass = true;
            tiempoBloqueoPass = millis();
            
            // Guardar estado actual de la alarma
            alarmaActivadaAntes = alarmaActivada;
            
            // ACTIVAR ALARMA FORZADAMENTE
            alarmaActivada = true;
            alarmaDisparada = true;
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("SISTEMA BLOQ!");
            lcd.setCursor(0, 1);
            lcd.print("INTRUSO! 30s");
            
            enviarEvento("SISTEMA_BLOQUEADO_CAMBIO_PASS_ALARMA_ACTIVADA");
            enviarEstadoSerial();
            
            // Resetear y salir del modo cambio
            cambiandoContrasena = false;
            faseCambioPass = 0;
            contNuevaPass = 0;
            memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
            memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
          } else {
            // Mostrar intentos restantes
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Clave incorrecta");
            lcd.setCursor(0, 1);
            lcd.print("Intentos: ");
            lcd.print(3 - intentosCambioPass);
            delay(2000);
            
            // Reintentar
            contNuevaPass = 0;
            memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Clave actual:");
            lcd.setCursor(0, 1);
            lcd.print("Clave: ");
          }
        }
      }
    }
    else if (faseCambioPass == 1) {
      // Fase 2: Ingresar nueva contraseña
      nuevaContrasena[contNuevaPass] = tecla;
      contNuevaPass++;
      
      lcd.setCursor(6 + contNuevaPass - 1, 1);
      lcd.print('*');
      
      if (contNuevaPass == 4) {
        // Nueva contraseña completa
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Guardando...");
        delay(500);
        
        // Copiar la nueva contraseña
        for (int i = 0; i < 4; i++) {
          contrasena[i] = nuevaContrasena[i];
        }
        
        // Enviar al Processing para que la guarde
        Serial.print("NEW_PASS:");
        Serial.println(contrasena);
        
        mostrarMensaje("Clave cambiada!", 2000);
        enviarEvento("CONTRASENA_CAMBIADA_DESDE_HARDWARE");
        
        // Resetear variables
        cambiandoContrasena = false;
        faseCambioPass = 0;
        contNuevaPass = 0;
        intentosCambioPass = 0;
        memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
        memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
      }
    }
  }
  else if (tecla == '#') {
    // Cancelar cambio de contraseña
    cambiandoContrasena = false;
    faseCambioPass = 0;
    contNuevaPass = 0;
    intentosCambioPass = 0;
    memset(contrasenaVerificacion, 0, sizeof(contrasenaVerificacion));
    memset(nuevaContrasena, 0, sizeof(nuevaContrasena));
    mostrarMensaje("Cancelado", 1500);
    enviarEvento("CAMBIO_CONTRASENA_CANCELADO");
  }
}
////
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
////
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
////
void mostrarMensaje(String mensaje, int tiempo) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mensaje);
  delay(tiempo);
  mostrarEstado();
}