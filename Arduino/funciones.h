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
  else if (strncmp(comando, "KEY:", 4) == 0) {//Analiza con que empieza el string recibido
    // Simular presión de tecla desde Processing
   char tecla = comando[4];//Extrae el caracter en la posición 4 del string 
    if (pidendoContrasena) {
      manejarContrasena(tecla);
    } else {
      manejarComando(tecla);
    }
  }
  else if (strncmp(comando, "SET_AUTO:", 9) == 0) {
    // Habilitar/deshabilitar auto-activación
    origenAccion = "SOFTWARE";
    int valor = atoi(&comando[9]);
    autoActivacionHabilitada = (valor == 1);
    enviarEvento(autoActivacionHabilitada ? "AUTO_ACTIVACION_HABILITADA" : "AUTO_ACTIVACION_DESHABILITADA");
    enviarEstadoSerial();
  }
  else if (strncmp(comando, "SET_TIME:", 9) == 0) {
    // Configurar tiempo de inactividad en minutos
    origenAccion = "SOFTWARE";
    int minutos = atoi(&comando[9]);
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
  Serial.println(TIEMPO_INACTIVIDAD_PARA_ACTIVAR / 60000); // Enviar en minutos
}

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