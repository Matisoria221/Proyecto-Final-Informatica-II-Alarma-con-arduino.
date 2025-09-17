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
char contrasena[] = "1234";//contraseña correcta
char ingresado[4];//contraseña ingresada
int cont=0;//variable auxiliar para contar la cantidad de digitos usados
int intentos=0;//contador de intentos incorrectos de alarma
bool estadoa = 0;//estado de la alarma 0=desactivada 1=activada

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
  lcd.print("Ingrese clave:");
}

void loop() {
  // Leer tecla presionada
  lectura = pad.getKey();
  // Validar que se haya leído una tecla válida
  if (lectura != NO_KEY) {
    ingresado[cont] = lectura;//Se guarda la tecla leida en contraseña ingresada
    Serial.print(ingresado[cont]); //Se imprime en nuestro monitor serial lo que este guardado en codigo[cont]
    cont++;
    if(cont==4){
      if(ingresado[0]==contrasena[0]&&ingresado[1]==contrasena[1]&&ingresado[2]==contrasena[2]&&ingresado[3]==contrasena[3]){
        lcd.setCursor(0,1);
        lcd.print("Clave Correcta");
        cont=0;
      }
      else{
        lcd.setCursor(0,0);
        lcd.print("Clave Incorrecta");
        cont=0;
        intentos++;
        lcd.setCursor(0,1);
        lcd.print("Intentos restantes:",intentos-1);
      }
    }
  }
}