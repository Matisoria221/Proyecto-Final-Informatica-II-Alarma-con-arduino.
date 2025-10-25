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

// Variables de interfaz
String[][] teclas = {
  {"1", "2", "3", "A"},
  {"4", "5", "6", "B"},
  {"7", "8", "9", "C"},
  {"*", "0", "#", "D"}
};

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
int tiempoSliderY = 200;
int tiempoSliderW = 200;
int tiempoSliderH = 20;

// Log de eventos con lista enlazada
class NodoEvento {
  String evento;
  NodoEvento siguiente;
  
  NodoEvento(String evento) {
    this.evento = evento;
    this.siguiente = null;
  }
}

class ListaEnlazadaEventos {
  NodoEvento cabeza;
  NodoEvento cola;
  int tamanio;
  
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
  }
}

void draw() {
  background(0);
  if (miImagen != null) {
    image(miImagen, 0, 0);
  }
 
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(24);
  text("SISTEMA DE CONTROL DE ALARMA", width/2, 35);
  
  dibujarTeclado();
  dibujarEstadoAlarma();
  dibujarControles();
  dibujarLogEventos();
}

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
  
  if (mouseX > tiempoSliderX && mouseX < tiempoSliderX + tiempoSliderW &&
      mouseY > tiempoSliderY - 10 && mouseY < tiempoSliderY + tiempoSliderH + 10) {
    sliderDragging = true;
    actualizarSlider();
  }
}

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
    // Formato Arduino: STATUS:Activa,Disparada,Autoactivacion ON,60
    String[] partes = split(datos.substring(7), ",");
    if (partes.length >= 4) {
      alarmaActivada = partes[0].trim().equals("Activa");
      alarmaDisparada = partes[1].trim().equals("Disparada");
      autoActivacionHabilitada = partes[2].trim().contains("ON");
      tiempoInactividad = int(trim(partes[3]));
      
      println("Parsed - Activada: " + alarmaActivada + ", Disparada: " + alarmaDisparada);
    }
  } else if (datos.startsWith("EVENT:")) {
    String evento = datos.substring(6);
    agregarEvento(evento);
  }
}

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

void keyReleased() {
  teclaPresionada = -1;
}
