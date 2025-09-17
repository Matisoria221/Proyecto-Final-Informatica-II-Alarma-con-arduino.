#include <keypad.h>//libreria para teclado matricial
#include <Wire.h>//libreria para comunicación I2C
#include <LiquidCrystal_I2C.h>//libreria para modulo LCD con I2

#define buzzer 13;
#define SensorPIR1 2;
#define SensorPIR2 3;
#define Sensor3 12;
const int filas 4;
const int columnas 4;
char teclado[filas][columnas]{
  {"1","2,""3","A"}
  {"4","5","6","B"}
  {"7","8","9","C"}
  {"*","0","#","D"}
};
byte filaspins [filas] = {8,9,10,11};
byte columnaspins [columnas] = {4,5,6,7};

Keypad pad = Keypad(makeKeymap(teclado),filaspins,columnaspins,filas,columnas); 
//Crear el objeto lcd  dirección  0x3F y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x3F,16,2);
char lectura;

void setup() {
  Serial.begin(9600);
  pinMoode(buzzer,OUTPUT);
  pinMode(SensorPIR1,INPUT);
  pinMode(SensorPIR2,INPUT);
  pinMode(Sensor3,INPUT);
  // Inicializar el LCD
  lcd.init(); 
  // Encender la luz de fondo
  lcd.backlight();
  // Mostrar mensaje inicial
  lcd.setCursor(0, 0);
  lcd.print("Sistema Listo");
  lcd.setCursor(0, 1);
  lcd.print("Ingrese codigo + A:");
}

void loop() {
  // Leer tecla presionada
  lectura = pad.getKey();
  
  // Validar que se haya leído una tecla válida
  if (lectura != NO_KEY) {
    // Limpiar la línea donde se mostrará la tecla
    lcd.setCursor(0, 1);
    lcd.print("Tecla: ");
    
    // Mostrar la tecla presionada
    lcd.print(lectura);
    
    // También enviar por serial para debug
    Serial.print("Tecla presionada: ");
    Serial.println(lectura);
    
    // Pequeña pausa para evitar lecturas múltiples
    delay(200);
  }
}