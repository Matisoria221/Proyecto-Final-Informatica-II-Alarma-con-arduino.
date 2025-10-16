import processing.serial.*;
import java.text.SimpleDateFormat;
import java.util.Date;

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

// Log de eventos
ArrayList<String> eventos = new ArrayList<String>();
PrintWriter logWriter;
String archivoLog = "alarma_log.txt";

// Variables de estado de interfaz
int teclaPresionada = -1;
boolean autoButtonHover = false;
boolean sliderDragging = false;
//Imagen de fondo 
PImage miImagen;

void setup() {
  size(800, 600);
  miImagen = loadImage("Fondo.jpg"); 
  // Inicializar puerto serial
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
  
  // Cargar log existente
  cargarLogAnterior();
  
  // Solicitar estado inicial
  delay(2000);
  if (puerto != null) {
    puerto.write("GET_STATUS\n");
  }
}

void draw() {
 image(miImagen, 0, 0);
  
  // Título
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(24);
  text("SISTEMA DE CONTROL DE ALARMA", width/2, 35);
  
  // Dibujar teclado
  dibujarTeclado();
  
  // Dibujar estado de la alarma
  dibujarEstadoAlarma();
  
  // Dibujar controles
  dibujarControles();
  
  // Dibujar log de eventos
  dibujarLogEventos();
}

void dibujarTeclado() {
  textSize(20);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int x = tecladoX + j * (teclaSize + teclaMargin);
      int y = tecladoY + i * (teclaSize + teclaMargin);
      
      // Determinar color de la tecla
      String tecla = teclas[i][j];
      if (teclaPresionada == i * 4 + j) {
        fill(0, 0, 0);
      } else if (tecla.equals("A")) {
        fill(0, 150, 0);
      } else if (tecla.equals("B")) {
        fill(100, 100, 100);
      } else if (tecla.equals("C")) {
        fill(100, 100, 100);
      } else if (tecla.equals("D")) {
        fill(200, 0, 0);
      } else if (tecla.equals("#")) {
        fill(200, 0, 0);
      } else if (tecla.equals("*")) {
        fill(100, 100, 100);
      } else {
        fill(0,0,250);
      }
      
      // Dibujar tecla
      stroke(0);
      strokeWeight(2);
      rect(x, y, teclaSize, teclaSize, 5);
      
      // Dibujar texto
      fill(255);
      textAlign(CENTER, CENTER);
      text(tecla, x + teclaSize/2, y + teclaSize/2);
    }
  }
  
  // Etiquetas de funciones
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
  
  // Caja de estado
  stroke(255);
  strokeWeight(3);
  if (alarmaDisparada) {
    fill(200, 0, 0);
  } else if (alarmaActivada) {
    fill(0, 150, 0);
  } else {
    fill(100, 100, 100);
  }
  rect(x, y, 300, 120, 10);
  
  // Texto de estado
  fill(255);
  textSize(18);
  textAlign(CENTER, CENTER);
  
  if (alarmaDisparada) {
    text("⚠ INTRUSIÓN ⚠", x + 150, y + 40);
    text("DETECTADA", x + 150, y + 70);
  } else if (alarmaActivada) {
    text("ALARMA", x + 150, y + 40);
    text("ACTIVADA", x + 150, y + 70);
  } else {
    text("ALARMA", x + 150, y + 40);
    text("INACTIVA", x + 150, y + 70);
  }
}

void dibujarControles() {
  // Botón de auto-activación
  autoButtonHover = mouseX > autoButtonX && mouseX < autoButtonX + autoButtonW &&
                  mouseY > autoButtonY && mouseY < autoButtonY + autoButtonH;
    
  stroke(255);
  strokeWeight(2);

  // Verde cuando está activa, rojo cuando está inactiva
  if (autoActivacionHabilitada) {
    fill(autoButtonHover ? color(0, 200, 0) : color(0, 150, 0)); // Verde más claro al hacer hover
  } else {
    fill(autoButtonHover ? color(200, 0, 0) : color(150, 0, 0)); // Rojo más claro al hacer hover
  }

  rect(autoButtonX, autoButtonY, autoButtonW, autoButtonH, 5);
    
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(14);
  text("Auto-Activación: " + (autoActivacionHabilitada ? "ON" : "OFF"),
      autoButtonX + autoButtonW/2, autoButtonY + autoButtonH/2);
      
  // Slider de tiempo
  fill(255);
  textAlign(LEFT);
  textSize(16);
  text("Tiempo de Inactividad: " + tiempoInactividad + " min", tiempoSliderX, tiempoSliderY - 10);
  
  // Barra del slider
  stroke(200);
  strokeWeight(2);
  fill(70);
  rect(tiempoSliderX, tiempoSliderY, tiempoSliderW, tiempoSliderH, 3);
  
  // Indicador del slider
  float sliderPos = map(tiempoInactividad, 1, 180, tiempoSliderX, tiempoSliderX + tiempoSliderW);
  fill(0, 150, 200);
  noStroke();
  ellipse(sliderPos, tiempoSliderY + tiempoSliderH/2, 20, 20);
}

void dibujarLogEventos() {
  fill(255);
  textAlign(LEFT);
  textSize(16);
  text("Log de Eventos:", 450, 440);
  
  // Caja de log
  stroke(200);
  strokeWeight(1);
  fill(20);
  rect(450, 460, 330, 120);
  
  // Mostrar últimos 5 eventos
  fill(255);
  textSize(12);
  int y = 470;
  for (int i = max(0, eventos.size() - 5); i < eventos.size(); i++) {
    String evento = eventos.get(i);
    if (evento.length() > 45) {
      evento = evento.substring(0, 42) + "...";
    }
    text(evento, 455, y);
    y += 20;
  }
}

void mousePressed() {
  // Verificar clic en teclado
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
  
  // Verificar clic en botón de auto-activación
  if (autoButtonHover) {
    autoActivacionHabilitada = !autoActivacionHabilitada;
    if (puerto != null) {
      puerto.write("SET_AUTO:" + (autoActivacionHabilitada ? "1" : "0") + "\n");
    }
  }
  
  // Verificar clic en slider
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
    // Enviar nuevo tiempo al Arduino
    if (puerto != null) {
      puerto.write("SET_TIME:" + tiempoInactividad + "\n");
    }
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
    agregarEvento("Tecla presionada: " + tecla);
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
  if (datos.startsWith("STATUS:")) {
    // Formato: STATUS:alarmaActivada,alarmaDisparada,autoActivacionHabilitada,tiempoInactividad
    String[] partes = split(datos.substring(7), ",");
    if (partes.length >= 4) {
      alarmaActivada = partes[0].equals("1");
      alarmaDisparada = partes[1].equals("1");
      autoActivacionHabilitada = partes[2].equals("1");
      tiempoInactividad = int(partes[3]);
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
  
  eventos.add(eventoCompleto);
  
  // Guardar en archivo
  guardarEvento(eventoCompleto);
  
  println(eventoCompleto);
}

void guardarEvento(String evento) {
  try {
    logWriter = new PrintWriter(new FileWriter(sketchPath(archivoLog), true));
    logWriter.println(evento);
    logWriter.flush();
    logWriter.close();
  } catch (IOException e) {
    println("Error al guardar evento: " + e.getMessage());
  }
}

void cargarLogAnterior() {
  try {
    BufferedReader reader = createReader(archivoLog);
    String line;
    while ((line = reader.readLine()) != null) {
      eventos.add(line);
    }
    reader.close();
  } catch (Exception e) {
    println("No se pudo cargar log anterior (puede ser la primera ejecución)");
  }
}

void keyPressed() {
  // Permitir usar el teclado físico de la PC también
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

// Importaciones necesarias para FileWriter
import java.io.FileWriter;
import java.io.IOException;
import java.io.BufferedReader;
