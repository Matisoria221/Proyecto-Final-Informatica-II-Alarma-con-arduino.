import processing.serial.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.io.FileWriter;
import java.io.IOException;
import java.io.BufferedReader;

Serial puerto;
String puertoSerial = "COM5"; // CAMBIAR AL PUERTO CORRECTO

// Estado de la alarma
boolean alarmaActivada = false;
boolean alarmaDisparada = false;
boolean autoActivacionHabilitada = true;
int tiempoInactividad = 60; // minutos
// variables para cambio de contraseña 
String archivoContrasena = "password.txt";
String contrasenaActual = "1234";
boolean mostrandoCambioPass = false;
String tempPassActual = "";
String tempNuevaPass = "";
int faseCambioPass = 0; // 0 = verificar actual, 1 = ingresar nueva
int intentosFallidos = 0;
boolean bloqueadoCambioPass = false;
long tiempoBloqueo = 0;
final int TIEMPO_BLOQUEO_PASS = 30000; // 30 segundos
// Variables de interfaz
String[][] teclas = {
  {"1", "2", "3", "A"},
  {"4", "5", "6", " "},
  {"7", "8", "9", " "},
  {" ", "0", "#", "D"}
};
///
int teclaSize = 80;
int teclaMargin = 10;
int tecladoX = 50;
int tecladoY = 100;
// Botón de auto-activación
int autoButtonX = 450;
int autoButtonY = 100;
int autoButtonW = 200;
int autoButtonH = 50;
// Control de tiempo
int tiempoSliderX = 450;
int tiempoSliderY = 180;
int tiempoSliderW = 200;
int tiempoSliderH = 20;
// Botón de cambio de contraseña
int passButtonX = 450;
int passButtonY = 220;
int passButtonW = 200;
int passButtonH = 50;
boolean passButtonHover = false;
// Log de eventos con lista enlazada
class NodoEvento {
  String evento; // Datos
  NodoEvento siguiente; 
  
  NodoEvento(String evento) {
    this.evento = evento;
    this.siguiente = null; // Inicialmente no apunta a nadie
  }
}

class ListaEnlazadaEventos {
  NodoEvento cabeza; // Primer nodo de la lista
  NodoEvento cola; // Último nodo de la lista
  int tamanio; // Contador de elementos
  
  ListaEnlazadaEventos() {
    this.cabeza = null;
    this.cola = null;
    this.tamanio = 0;
  }
  void agregar(String evento) {
    NodoEvento nuevoNodo = new NodoEvento(evento);
    
    if (cabeza == null) {
      cabeza = nuevoNodo;
      cola = nuevoNodo;
    } else {
      cola.siguiente = nuevoNodo;
      cola = nuevoNodo;
    }
    tamanio++;
  }
  String obtener(int indice) {
    if (indice < 0 || indice >= tamanio) return null;
 
    NodoEvento actual = cabeza;
    for (int i = 0; i < indice; i++) {
      actual = actual.siguiente;
    }
    return actual.evento;
  } 
  int tamanio() {
    return tamanio;
  }
  void limpiar() {
    cabeza = null;
    cola = null;
    tamanio = 0;
  }
  void guardarEnArchivo(String nombreArchivo) {
    try {
      PrintWriter writer = new PrintWriter(new FileWriter(sketchPath(nombreArchivo), false));
      
      NodoEvento actual = cabeza;
      while (actual != null) {
        writer.println(actual.evento);
        actual = actual.siguiente;
      }
      
      writer.flush();
      writer.close();
      println("Lista completa guardada en archivo (" + tamanio + " eventos)");
    } catch (IOException e) {
      println("Error al guardar lista en archivo: " + e.getMessage());
    }
  }
  ArrayList<String> obtenerUltimos(int n) {
    ArrayList<String> ultimos = new ArrayList<String>();
    
    if (tamanio == 0) return ultimos;
    
    int inicio = max(0, tamanio - n);
    NodoEvento actual = cabeza;
    
    for (int i = 0; i < inicio; i++) {
      actual = actual.siguiente;
    }
    
    while (actual != null) {
      ultimos.add(actual.evento);
      actual = actual.siguiente;
    }
    
    return ultimos;
  }
}

ListaEnlazadaEventos listaEventos;
String archivoLog = "alarma_log.txt";
// Variables de estado de interfaz
int teclaPresionada = -1;
boolean autoButtonHover = false;
boolean sliderDragging = false;
PImage miImagen;

void setup() {
  listaEventos = new ListaEnlazadaEventos();
  size(800, 600);
  
  miImagen = loadImage("Fondo.jpg");
  if (miImagen == null) {
    println("No se encontró Fondo.jpg");
  }
  
  // NUEVO: Cargar contraseña desde archivo
  cargarContrasena();
  
  println("Puertos disponibles:");
  printArray(Serial.list());
  
  try {
    puerto = new Serial(this, puertoSerial, 9600);
    puerto.bufferUntil('\n');
    agregarEvento("Conexión establecida con Arduino");
  } catch (Exception e) {
    println("Error al conectar con el puerto serial: " + e.getMessage());
    agregarEvento("ERROR: No se pudo conectar con Arduino");
  }
  
  cargarLogAnterior();
  
  delay(2000);
  if (puerto != null) {
    puerto.write("GET_STATUS\n");
    // NUEVO: Enviar contraseña a Arduino
    //enviarContrasenaArduino();
  }
}
void draw() {
  background(0);
  if (miImagen != null) {
    image(miImagen, 0, 0);
  }
  
  // Verificar desbloqueo automático
  if (bloqueadoCambioPass && (millis() - tiempoBloqueo >= TIEMPO_BLOQUEO_PASS)) {
    bloqueadoCambioPass = false;
    intentosFallidos = 0;
    agregarEvento("SOFTWARE: Bloqueo de cambio de contraseña finalizado");
  }
 
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(24);
  text("SISTEMA DE CONTROL DE ALARMA", width/2, 35);
  
  dibujarTeclado();
  dibujarEstadoAlarma();
  dibujarControles();
  dibujarLogEventos();
  
  // NUEVO: Mostrar diálogo de cambio de contraseña si está activo
  if (mostrandoCambioPass) {
    dibujarDialogoCambioPass();
  }
}
///
void cargarContrasena() {
  try {
    BufferedReader reader = createReader(archivoContrasena);
    String line = reader.readLine();
    if (line != null && line.length() == 4) {
      contrasenaActual = line.trim();
      println("Contraseña cargada: ****");
    } else {
      println("Formato de contraseña inválido, usando 1234");
      contrasenaActual = "1234";
      guardarContrasena();
    }
    reader.close();
  } catch (Exception e) {
    println("No se encontró archivo de contraseña, creando uno nuevo con 1234");
    contrasenaActual = "1234";
    guardarContrasena();
  }
}
///
void guardarContrasena() {
  try {
    PrintWriter writer = new PrintWriter(new FileWriter(sketchPath(archivoContrasena), false));
    writer.println(contrasenaActual);
    writer.flush();
    writer.close();
    println("Contraseña guardada en archivo");
  } catch (IOException e) {
    println("Error al guardar contraseña: " + e.getMessage());
  }
}
///
void enviarContrasenaArduino() {
  if (puerto != null) {
    puerto.write("SET_PASS:" + contrasenaActual + "\n");
    println("Contraseña enviada a Arduino: ****");
  }
}
///
void iniciarCambioContrasena() {
  // Verificar si está bloqueado
  if (bloqueadoCambioPass) {
    long tiempoRestante = (TIEMPO_BLOQUEO_PASS - (millis() - tiempoBloqueo)) / 1000;
    if (tiempoRestante > 0) {
      agregarEvento("SOFTWARE: Intento de cambio bloqueado (" + tiempoRestante + "s restantes)");
      return;
    } else {
      bloqueadoCambioPass = false;
      intentosFallidos = 0;
    }
  }
  
  faseCambioPass = 0;
  tempPassActual = "";
  tempNuevaPass = "";
  mostrandoCambioPass = true;
  agregarEvento("SOFTWARE: Iniciando cambio de contraseña");
}
///
void procesarCambioContrasena(String tecla) {
  if (tecla.matches("[0-9]")) {
    if (faseCambioPass == 0) {
      // Fase 1: Verificar contraseña actual
      tempPassActual += tecla;
      
      if (tempPassActual.length() == 4) {
        if (tempPassActual.equals(contrasenaActual)) {
          // Contraseña correcta, pasar a fase 2
          faseCambioPass = 1;
          tempNuevaPass = "";
          intentosFallidos = 0;
          agregarEvento("SOFTWARE: Contraseña actual verificada");
        } else {
          // Contraseña incorrecta
          intentosFallidos++;
          agregarEvento("SOFTWARE: Verificación fallida (Intento " + intentosFallidos + "/3)");
          
          if (intentosFallidos >= 3) {
            // Bloquear cambio de contraseña
            bloqueadoCambioPass = true;
            tiempoBloqueo = millis();
            agregarEvento("SOFTWARE: Cambio de contraseña bloqueado por 30 segundos");
            mostrandoCambioPass = false;
            faseCambioPass = 0;
            tempPassActual = "";
            tempNuevaPass = "";
          } else {
            // Reintentar
            tempPassActual = "";
          }
        }
      }
    } else if (faseCambioPass == 1) {
      // Fase 2: Ingresar nueva contraseña
      tempNuevaPass += tecla;
      
      if (tempNuevaPass.length() == 4) {
        // Nueva contraseña completa
        contrasenaActual = tempNuevaPass;
        guardarContrasena();
        enviarContrasenaArduino();
        agregarEvento("SOFTWARE: Contraseña cambiada exitosamente");
        mostrandoCambioPass = false;
        faseCambioPass = 0;
        tempPassActual = "";
        tempNuevaPass = "";
        intentosFallidos = 0;
      }
    }
  } else if (tecla.equals("#")) {
    // Cancelar
    mostrandoCambioPass = false;
    faseCambioPass = 0;
    tempPassActual = "";
    tempNuevaPass = "";
    agregarEvento("SOFTWARE: Cambio de contraseña cancelado");
  }
}
///
void dibujarDialogoCambioPass() {
  // Fondo semitransparente
  fill(0, 0, 0, 200);
  rect(0, 0, width, height);
  
  // Cuadro de diálogo
  fill(50, 50, 50);
  stroke(255);
  strokeWeight(3);
  rect(width/2 - 150, height/2 - 100, 300, 200, 10);
  
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(18);
  text("CAMBIAR CONTRASEÑA", width/2, height/2 - 70);
  
  textSize(14);
  if (faseCambioPass == 0) {
    // Fase 1: Verificar contraseña actual
    text("Ingrese contraseña actual:", width/2, height/2 - 40);
    
    // Mostrar intentos fallidos
    if (intentosFallidos > 0) {
      fill(255, 100, 100);
      textSize(12);
      text("Intentos fallidos: " + intentosFallidos + "/3", width/2, height/2 - 15);
      fill(255);
      textSize(14);
    }
  } else {
    // Fase 2: Ingresar nueva contraseña
    fill(100, 255, 100);
    text("✓ Contraseña verificada", width/2, height/2 - 40);
    fill(255);
    text("Ingrese nueva contraseña:", width/2, height/2 - 15);
  }
  
  // Mostrar asteriscos
  String display = "";
  String passActual = faseCambioPass == 0 ? tempPassActual : tempNuevaPass;
  
  for (int i = 0; i < passActual.length(); i++) {
    display += "* ";
  }
  for (int i = passActual.length(); i < 4; i++) {
    display += "_ ";
  }
  
  textSize(24);
  text(display, width/2, height/2 + 20);
  
  textSize(12);
  fill(200);
  text("Presione # para cancelar", width/2, height/2 + 70);
}
///
void dibujarTeclado() {
  textSize(20);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int x = tecladoX + j * (teclaSize + teclaMargin);
      int y = tecladoY + i * (teclaSize + teclaMargin);
      
      String tecla = teclas[i][j];
      if (teclaPresionada == i * 4 + j) {
        fill(50, 50, 50);
      } else if (tecla.equals("A")) {
        fill(0, 150, 0);
      } else if (tecla.equals("D")) {
        fill(200, 0, 0);
      } else if (tecla.equals("#")) {
        fill(150, 100, 0);
      } else if (tecla.equals(" ")) {
        fill(150);
      } else if (tecla.equals("B") || tecla.equals("C") || tecla.equals("*")) {
        fill(100, 100, 100);
      } else {
        fill(0, 0, 250);
      }
      
      stroke(0);
      strokeWeight(2);
      rect(x, y, teclaSize, teclaSize, 5);
      
      fill(255);
      textAlign(CENTER, CENTER);
      text(tecla, x + teclaSize/2, y + teclaSize/2);
    }
  }
  
  fill(200);
  textSize(12);
  textAlign(LEFT);
  text("A = Activar", tecladoX, tecladoY + 4 * (teclaSize + teclaMargin) + 20);
  text("D = Desactivar", tecladoX, tecladoY + 4 * (teclaSize + teclaMargin) + 40);
  text("# = Cancelar", tecladoX, tecladoY + 4 * (teclaSize + teclaMargin) + 60);
}
///
void dibujarEstadoAlarma() {
  int x = 450;
  int y = 300;
  
  stroke(255);
  strokeWeight(3);
  
  // Debugging
  println("Estado - Activada: " + alarmaActivada + ", Disparada: " + alarmaDisparada);
  
  // Prioridad: Disparada > Activada > Inactiva
  if (alarmaDisparada) {
    fill(255, 0, 0);
    rect(x, y, 300, 120, 10);
    
    fill(255);
    textSize(18);
    textAlign(CENTER, CENTER);
    text("¡INTRUSIÓN!", x + 150, y + 40);
    text("DETECTADA", x + 150, y + 70);
  } else if (alarmaActivada) {
    fill(0, 150, 0);
    rect(x, y, 300, 120, 10);
    
    fill(255);
    textSize(18);
    textAlign(CENTER, CENTER);
    text("ALARMA", x + 150, y + 40);
    text("ACTIVADA", x + 150, y + 70);
  } else {
    fill(100, 100, 100);
    rect(x, y, 300, 120, 10);
    
    fill(255);
    textSize(18);
    textAlign(CENTER, CENTER);
    text("ALARMA", x + 150, y + 40);
    text("INACTIVA", x + 150, y + 70);
  }
}
///
void dibujarControles() {
  autoButtonHover = mouseX > autoButtonX && mouseX < autoButtonX + autoButtonW &&
                    mouseY > autoButtonY && mouseY < autoButtonY + autoButtonH;
    
  stroke(255);
  strokeWeight(2);

  if (autoActivacionHabilitada) {
    fill(autoButtonHover ? color(0, 200, 0) : color(0, 150, 0));
  } else {
    fill(autoButtonHover ? color(200, 0, 0) : color(150, 0, 0));
  }

  rect(autoButtonX, autoButtonY, autoButtonW, autoButtonH, 5);
    
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(14);
  text("Auto-Activación: " + (autoActivacionHabilitada ? "ON" : "OFF"),
       autoButtonX + autoButtonW/2, autoButtonY + autoButtonH/2);
  
  // NUEVO: Botón de cambio de contraseña
  passButtonHover = mouseX > passButtonX && mouseX < passButtonX + passButtonW &&
                    mouseY > passButtonY && mouseY < passButtonY + passButtonH;
  
  // Cambiar color si está bloqueado
  if (bloqueadoCambioPass) {
    fill(passButtonHover ? color(150, 50, 50) : color(100, 30, 30));
  } else {
    fill(passButtonHover ? color(100, 100, 200) : color(70, 70, 150));
  }
  rect(passButtonX, passButtonY, passButtonW, passButtonH, 5);
  
  fill(255);
  if (bloqueadoCambioPass) {
    int segundosRestantes = (int)((TIEMPO_BLOQUEO_PASS - (millis() - tiempoBloqueo)) / 1000);
    text("Bloqueado (" + segundosRestantes + "s)", passButtonX + passButtonW/2, passButtonY + passButtonH/2);
  } else {
    text("Cambiar Contraseña", passButtonX + passButtonW/2, passButtonY + passButtonH/2);
  }
      
  // Slider de tiempo
  fill(255);
  textAlign(LEFT);
  textSize(16);
  text("Tiempo de Inactividad: " + tiempoInactividad + " min", tiempoSliderX, tiempoSliderY - 10);
  
  stroke(200);
  strokeWeight(2);
  fill(70);
  rect(tiempoSliderX, tiempoSliderY, tiempoSliderW, tiempoSliderH, 3);
  
  float sliderPos = map(tiempoInactividad, 1, 180, tiempoSliderX, tiempoSliderX + tiempoSliderW);
  fill(0, 150, 200);
  noStroke();
  ellipse(sliderPos, tiempoSliderY + tiempoSliderH/2, 20, 20);
}
///
void dibujarLogEventos() {
  fill(255);
  textAlign(LEFT);
  textSize(14);
  text("Log de Eventos (Total: " + listaEventos.tamanio() + "):", 450, 440);
  
  stroke(200);
  strokeWeight(1);
  fill(30);
  rect(450, 460, 330, 120);
  
  ArrayList<String> ultimosEventos = listaEventos.obtenerUltimos(5);
  
  textSize(10);
  int y = 470;
  for (int i = 0; i < ultimosEventos.size(); i++) {
    String evento = ultimosEventos.get(i);
    
    if (evento.contains("HARDWARE")) {
      fill(100, 200, 255);
    } else if (evento.contains("SOFTWARE")) {
      fill(100, 255, 100);
    } else {
      fill(200);
    }
    
    if (evento.length() > 48) {
      evento = evento.substring(0, 45) + "...";
    }
    text(evento, 455, y);
    y += 20;
  }
  
  textSize(9);
  fill(150);
  text("Azul=Teclado | Verde=Interfaz", 455, 575);
}

void mousePressed() {
  if (mostrandoCambioPass) {
    // No permitir otras interacciones mientras se cambia la contraseña
    return;
  }
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int x = tecladoX + j * (teclaSize + teclaMargin);
      int y = tecladoY + i * (teclaSize + teclaMargin);
      
      if (mouseX > x && mouseX < x + teclaSize && mouseY > y && mouseY < y + teclaSize) {
        teclaPresionada = i * 4 + j;
        String tecla = teclas[i][j];
        enviarTecla(tecla);
        return;
      }
    }
  }
  
  if (autoButtonHover) {
    autoActivacionHabilitada = !autoActivacionHabilitada;
    if (puerto != null) {
      puerto.write("SET_AUTO:" + (autoActivacionHabilitada ? "1" : "0") + "\n");
    }
    agregarEvento("SOFTWARE: Auto-activación " + (autoActivacionHabilitada ? "habilitada" : "deshabilitada"));
  }
  
  // NUEVO: Botón de cambio de contraseña
  if (passButtonHover && !bloqueadoCambioPass) {
    iniciarCambioContrasena();
  }
  
  if (mouseX > tiempoSliderX && mouseX < tiempoSliderX + tiempoSliderW &&
      mouseY > tiempoSliderY - 10 && mouseY < tiempoSliderY + tiempoSliderH + 10) {
    sliderDragging = true;
    actualizarSlider();
  }
}
///
void mouseReleased() {
  teclaPresionada = -1;
  if (sliderDragging) {
    sliderDragging = false;
    if (puerto != null) {
      puerto.write("SET_TIME:" + tiempoInactividad + "\n");
    }
    agregarEvento("SOFTWARE: Tiempo de inactividad configurado a " + tiempoInactividad + " min");
  }
}

void mouseDragged() {
  if (sliderDragging) {
    actualizarSlider();
  }
}

void actualizarSlider() {
  float newValue = constrain(mouseX, tiempoSliderX, tiempoSliderX + tiempoSliderW);
  tiempoInactividad = (int)map(newValue, tiempoSliderX, tiempoSliderX + tiempoSliderW, 1, 180);
}

void enviarTecla(String tecla) {
  if (puerto != null) {
    puerto.write("KEY:" + tecla + "\n");
    agregarEvento("SOFTWARE: Tecla presionada: " + tecla);
  }
}

void serialEvent(Serial p) {
  String datos = p.readStringUntil('\n');
  if (datos != null) {
    datos = trim(datos);
    procesarDatosSerial(datos);
  }
}

void procesarDatosSerial(String datos) {
  println("Recibido: " + datos);
  
  if (datos.startsWith("STATUS:")) {
    String[] partes = split(datos.substring(7), ",");
    if (partes.length >= 4) {
      alarmaActivada = partes[0].trim().equals("Activa");
      alarmaDisparada = partes[1].trim().equals("Disparada");
      autoActivacionHabilitada = partes[2].trim().contains("ON");
      tiempoInactividad = int(trim(partes[3]));
    }
  } 
  else if (datos.startsWith("EVENT:")) {
    String evento = datos.substring(6);
    agregarEvento(evento);
    
    // Detectar eventos de bloqueo desde Arduino
    if (evento.contains("SISTEMA_BLOQUEADO_CAMBIO_PASS_ALARMA_ACTIVADA")) {
      agregarEvento("SOFTWARE: Arduino bloqueado por intentos fallidos de cambio de contraseña");
    }
  }
  else if (datos.startsWith("INIT_PASS:")) {
    // Sincronización inicial de contraseña desde Arduino (al conectar)
    String passArduino = datos.substring(10).trim();
    println("DEBUG: Contraseña inicial de Arduino: " + passArduino.length() + " caracteres");
    
    if (passArduino.length() == 4) {
      // Verificar si es diferente a la que tenemos
      if (!passArduino.equals(contrasenaActual)) {
        println("DEBUG: Sincronizando contraseña de Arduino con Processing");
        contrasenaActual = passArduino;
        guardarContrasena();
        agregarEvento("SOFTWARE: Contraseña sincronizada desde Arduino");
      } else {
        println("DEBUG: Contraseñas ya sincronizadas");
        agregarEvento("SOFTWARE: Contraseñas sincronizadas correctamente");
      }
    }
  }
  else if (datos.startsWith("NEW_PASS:")) {
    // Recibir nueva contraseña desde Arduino (cuando se cambia desde teclado físico)
    String nuevaPass = datos.substring(9).trim();
    println("DEBUG: Contraseña recibida de Arduino: " + nuevaPass.length() + " caracteres");
    
    if (nuevaPass.length() == 4) {
      contrasenaActual = nuevaPass;
      guardarContrasena();
      // NO reenviar a Arduino aquí, Arduino ya la tiene
      agregarEvento("HARDWARE: Contraseña cambiada desde teclado físico exitosamente");
      println("DEBUG: Contraseña guardada en archivo");
    } else {
      println("ERROR: Contraseña recibida tiene longitud incorrecta: " + nuevaPass.length());
      agregarEvento("ERROR: Contraseña recibida con formato incorrecto");
    }
  }
  else if (datos.startsWith("DEBUG:")) {
    // Mensajes de depuración desde Arduino
    println("Arduino Debug: " + datos.substring(7));
  }
}
///
void agregarEvento(String evento) {
  SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
  String timestamp = sdf.format(new Date());
  String eventoCompleto = timestamp + " - " + evento;
  
  listaEventos.agregar(eventoCompleto);
  println(eventoCompleto);
  
  if (esEventoCritico(evento)) {
    println(">>> Evento crítico detectado: " + evento);
    println(">>> Guardando lista completa y limpiando...");
    
    listaEventos.guardarEnArchivo(archivoLog);
    listaEventos.limpiar();
    
    println(">>> Lista limpiada. Nuevo ciclo iniciado.");
  }
}

boolean esEventoCritico(String evento) {
  return evento.contains("ALARMA_ACTIVADA") ||
         evento.contains("ALARMA_DESACTIVADA") ||
         evento.contains("INTRUSION_DETECTADA") ||
         evento.contains("SISTEMA_BLOQUEADO");
}

void cargarLogAnterior() {
  try {
    BufferedReader reader = createReader(archivoLog);
    String line;
    int count = 0;
    while ((line = reader.readLine()) != null) {
      listaEventos.agregar(line);
      count++;
    }
    reader.close();
    println("Log anterior cargado: " + count + " eventos");
  } catch (Exception e) {
    println("No se pudo cargar log anterior (puede ser la primera ejecución)");
  }
}

void keyPressed() {
  String tecla = str(key).toUpperCase();
  
  if (mostrandoCambioPass) {
    // Modo cambio de contraseña
    procesarCambioContrasena(tecla);
    return;
  }
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (teclas[i][j].equals(tecla)) {
        enviarTecla(tecla);
        teclaPresionada = i * 4 + j;
        return;
      }
    }
  }
}
///
void keyReleased() {
  teclaPresionada = -1;
}
