#include <keypad.h>

const int filas 4;
const int columnas 4;
char teclado[filas][columnas]{
  {"1","2,""3","A"}
  {"4","5","6","B"}
  {"7","8","9","C"}
  {"*","0","#","D"}
};
int filaspins [filas] {8,9,10,11};
int columnaspins [columnas] {4,5,6,7};

Keypad pad = Keypad(makeKeymap(teclado),filaspins,columnaspins,filas,columnas); 

char lectura;

void setup() {
  serial.begin(9600);

}

void loop() {
 lectura = pad.getKey(); 

}
