//***************************************************************************************************************************************
/* Librería para el uso de la pantalla ILI9341 en modo 8 bits
   Basado en el código de martinayotte - https://www.stm32duino.com/viewtopic.php?t=637
   Adaptación, migración y creación de nuevas funciones: Pablo Mazariegos y José Morales
   Con ayuda de: José Guerra
   IE3027: Electrónica Digital 2 - 2019
*/

//***************************************************************************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <SPI.h>
#include <SD.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"

#define LCD_RST PD_0
#define LCD_CS PD_1
#define LCD_RS PD_2
#define LCD_WR PD_3
#define LCD_RD PE_1
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};

//DEFINICION DE BOTONES DE PRUEBA
//#define PUSH1  PF_4 //SW1
//#define PUSH2  PF_0 //SW2

#define B1_1 PA_6 // Boton 1 jugador 1
#define B2_1 PA_7 // Boton 2 jugador 1
#define B1_2 PE_3 // Boton 1 jugador 2
#define B2_2 PF_1 // Boton 2 jugador 2

//***************************************************************************************************************************************
// Functions Prototypes
//***************************************************************************************************************************************
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);

void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset);

void openSDformat(unsigned char L[], unsigned long SIZE, char* a);
int ACII_HEX(char *puntero);
void printDirectory(File dir, int numTabs);

//FUNCIONES DE ESTADOS
void E1(void);
void E2(void);
void E3(void);
void E4(void);
void E5(void);

//FUNCIONES PARA DESPLEGAR IMAGENES DE LA SD
void obtener_titulo1(void);
void obtener_titulo2(void);
void obtener_p1(void);
void obtener_p2(void);

//FUNCIONES PARA MOSTRAR PANTALLAS
void mostrar_pantalla_inicio(void);
void mostrar_menu_principal(void);
void mostrar_pantalla_juego(void);
void mostrar_seleccion_jugador(void);

//FUNCION PARA MOVER EL CURSOR DE SELECCION
void mover_seleccionador(void);
void seleccion_jugador(void);

//FUNCION DEL JUEGO
void juego(void);

//***************************************************************************************************************************************
// Variables Globales
//***************************************************************************************************************************************
//MODO MOSTRADO EN LA PANTALLA
char modo = 0;

//PANTALLA DE INICIO
char parpadeo = 1;
String text1;

//MENU PRINCIPAL
char one_time = 1;
char seleccionador = 1;
String text2, text3, text4;


//SELECCION DE JUGADOR
//se vuelven a utilizar text 1, text2, text3 y text4
int jugador1 = 0; // Estas variables indican el sable que se selecciona
int jugador2 = 0;

//PANTALLA DE JUEGO
char ganador = 0;

//RUTA PARA SD
File root;

//***************************************************************************************************************************************
// Inicialización
//***************************************************************************************************************************************
void setup() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
  Serial.begin(9600);
  SPI.setModule(0);      // COMUNICACIÓN SPI EN EL MÓDULO 0
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  Serial.println("Inicio");

  //INICIALIZACION DE LA MEMORIA SD
  pinMode(10, OUTPUT);

  if (!SD.begin(32)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");


  // INICIALIZACIÓN DE LA PANTALLA
  LCD_Init();

  //INICIALIZACION DE BOTONES DE PRUEBA
  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);

  // Botones
  pinMode (B1_1, INPUT_PULLUP);
  pinMode (B2_1, INPUT_PULLUP);
  pinMode (B1_2, INPUT_PULLUP);
  pinMode (B2_2, INPUT_PULLUP);

  // Pines para comunicacion con arduino
  pinMode(PC_4, OUTPUT); // bit 0
  pinMode(PC_5, OUTPUT); // bit 1

}
//***************************************************************************************************************************************
// Loop Infinito
//***************************************************************************************************************************************
void loop() {
  switch (modo) {
    case 0:
      E1();   //CORRESPONDE AL ESTADO DE LA PANTALLA DE INICIO
      break;
    case 1:
      E2();   //CORRESPONDE AL ESTADO DEL MENU PRINCIPAL
      break;
    case 2:
      E3();  //CORRESPONDE AL ESTADO DE SELECCION DE PERSONAJES
      break;
    case 3:
      E4();   //CORRESPONDE AL ESTADO DE LA EJECUCION DEL JUEGO
      break;
    case 4:   //PANTALLA DE GANADOR
      E5();
      break;
  }
}

//***************************************************************************************************************************************
//FUNCIONES DE ESTADOS
//***************************************************************************************************************************************

void E1(void) {
  // THEME
  digitalWrite(PC_4, LOW);
  digitalWrite(PC_5, LOW);
  mostrar_pantalla_inicio();

  //CONDICION PARA PODER AVANZAR AL SIGUIENTE ESTADO
  if (!digitalRead(B1_1) || !digitalRead(B2_1) || !digitalRead(B1_2) || !digitalRead(B2_2)) {
    modo = 1;
    one_time = 1;
  }
}

void E2(void) {
  // IMPERIAL
  digitalWrite(PC_4, HIGH);
  digitalWrite(PC_5, LOW);
  mostrar_menu_principal();

  //CONDICION PARA PODER MOVER EL SELECCIONADOR
  if (!digitalRead(B2_1)) {
    seleccionador++;
    mover_seleccionador();
  }

  //CONDICIONES PARA PODER ESCOGER EL ESTADO A CUAL IR
  if ((!digitalRead(B1_1)) && (seleccionador == 1)) {
    modo = 2;   //SE MUEVE AL MENU PARA SELECCIONAR JUGADOR
    one_time = 1;
  }
  if ((!digitalRead(B1_1)) && (seleccionador == 2)) {
    modo = 0;   //REGRESA AL MENU PRINCIPAL
    one_time = 1;
  }
  if ((!digitalRead(B1_1)) && (seleccionador == 3)) {
    modo = 3;   //SE MUEVE HACIA EL JUEGO
    one_time = 1;
  }
}

void E3(void) {
  //APPROACH
  digitalWrite(PC_4, HIGH);
  digitalWrite(PC_5, HIGH);
  mostrar_seleccion_jugador();
  seleccion_jugador();

}

void E4(void) {
  // BATTLE
  digitalWrite(PC_4, LOW);
  digitalWrite(PC_5, HIGH);
  mostrar_pantalla_juego();
  juego();
}

void E5(void) {
  switch(ganador){
    case 0:
      FillRect(0, 0, 320, 240, 0x0180);
      LCD_Print("TIE",140, 105, 2, 0x0000, 0x0180);
      while(modo == 4){  
       LCD_Print("PRESS ANY BUTTON TO CONTINUE",50, 170, 1, 0x0000, 0x0180);
       //CONDICION PARA PODER AVANZAR AL SIGUIENTE ESTADO
        if (!digitalRead(PUSH1) || !digitalRead(PUSH2) || !digitalRead(B1_2) || !digitalRead(B2_2)) {
            modo = 1;
            one_time = 1;
        }
      }
      break;
    case 1:
      FillRect(0, 0, 320, 240, 0x7800);
      LCD_Print("P1 WINS",110, 105, 2, 0x0000, 0x7800);
      while(modo == 4){          
        LCD_Print("PRESS ANY BUTTON TO CONTINUE",50, 170, 1, 0x0000, 0x7800);
       //CONDICION PARA PODER AVANZAR AL SIGUIENTE ESTADO
        if (!digitalRead(PUSH1) || !digitalRead(PUSH2) || !digitalRead(B1_2) || !digitalRead(B2_2)) {
            modo = 1;
            one_time = 1;
        }
      }
      break;
    case 2:
      FillRect(0, 0, 320, 240, 0x000a);
      LCD_Print("P2 WINS",110, 105, 2, 0x0000, 0x000a);
      while(modo == 4){        
        LCD_Print("PRESS ANY BUTTON TO CONTINUE",50, 170, 1, 0x0000, 0x000a);
        //CONDICION PARA PODER AVANZAR AL SIGUIENTE ESTADO
        if (!digitalRead(B1_1) || !digitalRead(B2_1) || !digitalRead(B1_2) || !digitalRead(B2_2)) {
            modo = 1;
            one_time = 1;
        }
      }
      break;
  }
 
}
//***************************************************************************************************************************************
//FUNCIONES PARA DESPLEGAR IMAGENES DE LA SD
//***************************************************************************************************************************************

/*EN ESTAS FUNCIONES SE CREAN VECTORES COMO VARIABLES LOCALES Y SE LLENAN DE LA INFORMACION PARA CREAR LA IMAGEN, LA CUAL FUE GUARDADA EN
  LA SD. ESTO SE EFECTUA DE LA SIGUIENTE MANERA PARA AHORRAR ESPACION EN LA MEMEORIA RAM.
*/

//MUESTRA EL T1 EN LA PANTALLA DE INICIO
void obtener_titulo1(void) {
  unsigned char titulo1[11000] = {0};
  openSDformat(titulo1, 11001, "TITULO1.txt");
  LCD_Bitmap(100, 30, 110, 50, titulo1);
  one_time = 0;
}

//MUESTRA EL T2 EN LA PANTALLA DE INICIO
void obtener_titulo2(void) {
  unsigned char titulo2[13034] = {0};
  openSDformat(titulo2, 13035, "TITULO2.txt");
  LCD_Bitmap(80, 100, 133, 49, titulo2);
  one_time = 0;
}

//MUESTRA EL T1 EN LA PANTALLA DE SELECCION DEL JUGADOR
void obtener_titulo1_2(void) {
  unsigned char titulo1[11000] = {0};
  openSDformat(titulo1, 11001, "TITULO1.txt");
  LCD_Bitmap(105, 5, 110, 50, titulo1);
}

//***************************************************************************************************************************************
// Función para el juego
//***************************************************************************************************************************************
void juego(void) {
  unsigned char p1_salto[2720], p2_salto[2720], p1_ataque[3264], p2_ataque[3264], enemigo1[1120], enemigo2[3920];
  ganador = 0;
  bool mantener_salto_p1 = false;
  bool mantener_ataque_p1 = false;
  bool mantener_salto_p2 = false;
  bool mantener_ataque_p2 = false;
  uint8_t anim_salto_p1 = 0;
  uint8_t anim_ataque_p1 = 0;
  uint8_t anim_salto_p2 = 0;
  uint8_t anim_ataque_p2 = 0;
  uint8_t x1 = 190;   //POSICION DE PLAYER1
  uint8_t x2 = 130;   //POSICION DE PLAYER2
  uint8_t h1 = 135;   //ALTURA DE PLAYER1
  uint8_t h2 = 135;   //ALTURA DE PLAYER2
  uint8_t y1 = 0;     //VARIABLE PARA CONTROLAR SALTO DE PLAYER1
  uint8_t y2 = 0;     //VARIABLE PARA CONTROLAR SALTO DE PLAYER2
  uint8_t pj1 = 0;    //CONTADOR DE LOS PUNTOS DEL JUGADOR 1
  uint8_t pj2 = 0;    //CONTADOR DE PUNTOS DE JUGADOR 2
  uint8_t choque1 = 0;
  uint8_t choque2 = 0;
  uint8_t choque3 = 0;
  uint8_t choque4 = 0;

  //----VARIABLES DE ENEMIGOS-----
  uint8_t enemigo;
  enemigo = random(1, 5);     // Eleccion de un enemigo aleatorio
  bool enemigo_done = false;  // Indicador de que el enemigo llegó al final de su trayectoria

  uint8_t paso = 3;
  uint8_t y_e1 = 140;
  int x_e1_p1 = 319 ;   // Coordenada x del enemigo 1 del jugador 1
  uint8_t x_e1_p2 = 0;  // Coordenada x del enemigo 2 del jugador 2

  uint8_t paso2 = 2;
  uint8_t y_e2 = 130;
  int x_e2_p1 = 290 ;   // Coordenada x del enemigo 2 del jugador 1
  uint8_t x_e2_p2 = 0;  // Coordenada x del enemigo 2 del jugador 2


  //----VARIABLES DE TIEMPO-----
  unsigned long t_actual1 = millis();
  unsigned long delay_salto1 = 200;
  unsigned long t_actual2 = millis();
  unsigned long delay_salto2 = 200;

  openSDformat(enemigo1, 1121, "ENEMIGO1.txt");
  openSDformat(enemigo2, 3921, "ENEMIGO2.txt");

  if(jugador1 == jugador2){ // Si no se selecciono ningun jugador
    jugador1 = 0;
    jugador2 = 1;
  }

  switch (jugador1) {
    case 0:
      openSDformat(p1_salto, 2721, "P1S.txt");
      openSDformat(p1_ataque, 3265, "P1A.txt");
      break;
    case 1:
      openSDformat(p1_salto, 2721, "P2S.txt");
      openSDformat(p1_ataque, 3265, "P2A.txt");
      break;
    case 2:
      openSDformat(p1_salto, 2721, "P3S.txt");
      openSDformat(p1_ataque, 3265, "P3A.txt");
      break;
    case 3:
      openSDformat(p1_salto, 2721, "P4S.txt");
      openSDformat(p1_ataque, 3265, "P4A.txt");
      break;
  }
  switch (jugador2) {
    case 0:
      openSDformat(p2_salto, 2721, "P1S.txt");
      openSDformat(p2_ataque, 3265, "P1A.txt");
      break;
    case 1:
      openSDformat(p2_salto, 2721, "P2S.txt");
      openSDformat(p2_ataque, 3265, "P2A.txt");
      break;
    case 2:
      openSDformat(p2_salto, 2721, "P3S.txt");
      openSDformat(p2_ataque, 3265, "P3A.txt");
      break;
    case 3:
      openSDformat(p2_salto, 2721, "P4S.txt");
      openSDformat(p2_ataque, 3265, "P4A.txt");
      break;
  }
  //LCD_Sprite(x1, 135, 20, 34, p1_salto, 2, 0, 0, 0);
  //LCD_Sprite(x1, 135, 24, 34, p1_ataque, 2, 0, 0, 0);
  //LCD_Sprite(x2, 135, 24, 34, p2_ataque, 2, 0, 1, 0);
  //LCD_Sprite(x2, 135, 20, 34, p2_salto, 2, 0, 1, 0);
  while (modo == 3) {
    //------------------------CONDICIONES DE LOS BOTONES---------------------
    //PLAYER1
    if (!digitalRead(B1_1)) {
      mantener_salto_p1 = true;
      t_actual1 = millis();
    }
    if (!digitalRead(B2_1)) {
      mantener_ataque_p1 = true;
      t_actual1 = millis();
    }
    //PLAYER2 (AHORITE SE PRUEBA CON LOS BOTONES DE LA TIVA)
    if (!digitalRead(B1_2)) {
      mantener_salto_p2 = true;
      t_actual2 = millis();
    }
    if (!digitalRead(B2_2)) {
      mantener_ataque_p2 = true;
      t_actual2 = millis();
    }

    //-----------------EJECUCION DE MOVIMIENTOS DE PLAYER1-------------------
    if (mantener_salto_p1 == true) {

      if (millis() > t_actual1 + delay_salto1 ) {

        switch (y1) {
          case 0:
            y1++;
            anim_salto_p1 = 1;
            h1 = h1 - 30;
            FillRect(x1, h1 + 31, 20, 30, 0x00);
            break;
          case 1:
            y1++;
            anim_salto_p1 = 1;
            h1 = h1 - 15;
            FillRect(x1, h1 + 31, 20, 30, 0x00);
            break;
          case 2:
            y1++;
            anim_salto_p1 = 1;
            h1 = h1 + 15;
            FillRect(x1, h1 - 30, 20, 30, 0x00);
            break;
          case 3:
            y1 = 0;
            h1 = h1 + 30;
            FillRect(x1, h1 - 30, 20, 30, 0x00);
            anim_salto_p1 = 0;
            mantener_salto_p1 = false;
            break;
        }
        LCD_Sprite(x1, h1, 20, 34, p1_salto, 2, anim_salto_p1, 0, 0);
        t_actual1 = millis();
      }
    }

    if (mantener_ataque_p1 == true) {
      if (millis() > t_actual1 + (2 * delay_salto1) ) {
        switch (anim_ataque_p1) {
          case 0:
            anim_ataque_p1++;
            break;
          case 1:
            anim_ataque_p1 = 0;
            mantener_ataque_p1 = false;
            break;
        }
        LCD_Sprite(x1, h1, 24, 34, p1_ataque, 2, anim_ataque_p1, 0, 0);
        FillRect(x1+20, h1+20, 4,5, 0x0000);
        t_actual1 = millis();
      }
    }

    if ((mantener_salto_p1 == false) && (mantener_ataque_p1 == false)) {
      LCD_Sprite(x1, h1, 20, 34, p1_salto, 2, 0, 0, 0);
      //LCD_Sprite(x1, h1, 24, 34, p1_ataque, 2, 0, 0, 0);
    }

    //-----------------EJECUCION DE MOVIMIENTOS DE PLAYER2-------------------
    if (mantener_salto_p2 == true) {
      if (millis() > t_actual2 + delay_salto2 ) {
        switch (y2) {
          case 0:
            y2++;
            anim_salto_p2 = 1;
            h2 = h2 - 30;
            FillRect(x2, h2 + 31, 20, 30, 0x00);
            break;
          case 1:
            y2++;
            anim_salto_p2 = 1;
            h2 = h2 - 15;
            FillRect(x2, h2 + 32, 20, 17, 0x00);
            break;
          case 2:
            y2++;
            anim_salto_p2 = 1;
            h2 = h2 + 15;
            FillRect(x2, h2 - 30, 20, 30, 0x00);
            break;
          case 3:
            y2 = 0;
            h2 = h2 + 30;
            FillRect(x2, h2 - 30, 20, 30, 0x00);
            anim_salto_p2 = 0;
            mantener_salto_p2 = false;
            break;
        }
        LCD_Sprite(x2, h2, 20, 34, p2_salto, 2, anim_salto_p2, 1, 0);
        t_actual2 = millis();
      }
    }


    if (mantener_ataque_p2 == true) {
      if (millis() > t_actual2 + (2 * delay_salto2) ) {
        switch (anim_ataque_p2) {
          case 0:
            anim_ataque_p2++;
            break;
          case 1:
            anim_ataque_p2 = 0;
            mantener_ataque_p2 = false;
            break;
        }
        LCD_Sprite(x2, h2, 24, 34, p2_ataque, 2, anim_ataque_p2, 1, 0);
        FillRect(x2+20, h2+20, 4,3, 0x0000);
        t_actual2 = millis();
      }
    }

    if ((mantener_salto_p2 == false) && (mantener_ataque_p2 == false)) {
      LCD_Sprite(x2, h2, 20, 34, p2_salto, 2, 0, 1, 0);
      //LCD_Sprite(x2, h2, 24, 34, p2_ataque, 2, 0, 1, 0);
    }

    //----------------------------ENEMIGOS---------------------------------------

    if (enemigo_done == false) {
      switch (enemigo) {
        case 1:
          if(choque2 == 0){
            if (x_e1_p2 > 150) {
              x_e1_p2 = 0;
              FillRect(150, y_e1, 20, 14, 0x00);
              enemigo_done = true;
            }
            else {
              x_e1_p2 = x_e1_p2 + paso;
              delay(1);
              int anim_e1_p2 = (x_e1_p2 / 25) % 2;
              LCD_Sprite(x_e1_p2, y_e1, 20, 14, enemigo1, 2, anim_e1_p2, 0, 0);
              FillRect(x_e1_p2 - paso, y_e1, paso, 14, 0x00);
            }
          }
          else{
            FillRect(x_e1_p2, y_e1, 20, 14, 0x00);
            choque2 = 0;
            x_e1_p2 = 0;
            enemigo_done = true;
          }
          break;
        case 2:
          if(choque1 == 0){
            if (x_e1_p1 < 170) {
              x_e1_p1 = 319;
              FillRect(170, y_e1, 20, 14, 0x00);
              enemigo_done = true;
            }
            else {
              x_e1_p1 = x_e1_p1 - paso;
              delay(1);
              int anim_e1_p1 = (x_e1_p1 / 25) % 2;
              LCD_Sprite(x_e1_p1, y_e1, 20, 14, enemigo1, 2, anim_e1_p1, 1, 0);
              //FillRect(x_e1_p1+20, y_e1, paso, 14, 0x00);
            }
          }
          else{
            FillRect(x_e1_p1, y_e1, 20, 14, 0x00);
            choque1 = 0;
            x_e1_p1 = 319;
            enemigo_done = true;
          }
          break;
        case 3:
          if(choque4 == 0){
            if (x_e2_p2 > x2-15) {
              x_e2_p2 = 0;
              FillRect(x2-15, y_e2, 28, 35, 0x00);
              enemigo_done = true;
              pj2++;
            }
            else {
              x_e2_p2 = x_e2_p2 + paso2;
              delay(2);
              int anim_e2_p2 = (x_e2_p2 / 20) % 2;
              LCD_Sprite(x_e2_p2, y_e2, 28, 35, enemigo2, 2, anim_e2_p2, 0, 0);
              FillRect(x_e2_p2 - paso, y_e2, paso2, 35, 0x00);
            }
          }
          else{
            FillRect(x_e2_p2, y_e2, 28, 35, 0x00);
            choque4 = 0;
            x_e2_p2 = 0;
            enemigo_done = true;
          }
          break;
        case 4:
          if(choque3 == 0){
            if (x_e2_p1 < x1+15) {
              x_e2_p1 = 290;
              FillRect(x1+15, y_e2, 28, 35, 0x00);
              enemigo_done = true;
              pj1++;
            }
            else {
              x_e2_p1 = x_e2_p1 - paso2 - 2;
              delay(1);
              int anim_e2_p1 = (x_e2_p1 / 30) % 2;
              LCD_Sprite(x_e2_p1, y_e2, 28, 35, enemigo2, 2, anim_e2_p1, 1, 0);
              FillRect(x_e2_p1 + 28, y_e2, paso2, 35, 0x00);
            }
          }
          else{
            FillRect(x_e2_p1, y_e2, 28, 35, 0x00);
            choque3 = 0;
            x_e2_p1 = 290;
            enemigo_done = true;
          }
          break;
      }
    }else if (enemigo_done == true) {
      enemigo = random(1, 5);
      enemigo_done = false;
    }

    //-----------------------------------Condiciones de choque--------------------------------------------------------------------
    // Enemigo 1 p1
    if(x_e1_p1 <= (x1+20) && y_e1 <= (h1+34)){
      pj1++;
      choque1 = 1;
    }
   
    // Enemigo 1 p2
    if(x_e1_p2 >= x2-23 && y_e2 <= (h2+34)){
      pj2++;
      choque2 = 1;
    }

    if(anim_ataque_p1 == 1){
      if(x_e2_p1 <= x1+24 && x_e2_p1 >= x1+15){
        choque3 = 1;
      }
    }

    if(anim_ataque_p2 == 1){
      if(x_e2_p2 >= x2-23 && x_e2_p2 <= x2-15){
        choque4 = 1;
      }
    }
    //------------------------------------------------------MARCADOR------------------------------------------------------------
    LCD_Print("LIFES:", 15, 190, 2, 0x0280, 0xffff);

    switch(pj2){
      case 0:
        LCD_Print("5", 130, 190, 2, 0x000a, 0xffff);
        break;
      case 1:
        LCD_Print("4", 130, 190, 2, 0x000a, 0xffff);
        break;
      case 2:
        LCD_Print("3", 130, 190, 2, 0x000a, 0xffff);
        break;
      case 3:
        LCD_Print("2", 130, 190, 2, 0x000a, 0xffff);
        break;
      case 4:
        LCD_Print("1", 130, 190, 2, 0x000a, 0xffff);
        break;
      case 5:
        LCD_Print("0", 130, 190, 2, 0x000a, 0xffff);
        break;    
    }
    switch(pj1){
      case 0:
        LCD_Print("5", 190, 190, 2, 0x7800, 0xffff);
        break;
      case 1:
        LCD_Print("4", 190, 190, 2, 0x7800, 0xffff);
        break;
      case 2:
        LCD_Print("3", 190, 190, 2, 0x7800, 0xffff);
        break;
      case 3:
        LCD_Print("2", 190, 190, 2, 0x7800, 0xffff);
        break;
      case 4:
        LCD_Print("1", 190, 190, 2, 0x7800, 0xffff);
        break;
      case 5:
        LCD_Print("0", 190, 190, 2, 0x7800, 0xffff);
        break;    
    }
    //-------------------------------------------------------GANADOR------------------------------------------------------------
    if(pj1 == 5 && pj2 == 5){
      //empates
      ganador = 0;
      modo = 4;
    }
    else{
      if(pj1 == 5){
        // gana p2
        ganador = 2;
        modo = 4;
      }
      if(pj2 == 5){
        //gana p1
        ganador = 1;
        modo = 4;
      }
    }
  }

}

//***************************************************************************************************************************************
//FUNCIONES PARA MOSTRAR PANTALLAS
//***************************************************************************************************************************************
void mostrar_pantalla_inicio(void) {
  if (one_time == 1) {
    LCD_Clear(0x00);

    text1 = "PRESS ANY BUTTON TO START";
    LCD_Print(text1, 50, 170, 1, 0xffff, 0x00);

    obtener_titulo1();
    obtener_titulo2();

    one_time = 0;
  } else if (one_time == 0) {
    text1 = "PRESS ANY BUTTON TO START";
    if (parpadeo == 1) {
      LCD_Print(text1, 50, 170, 1, 0xffff, 0x00);
      parpadeo = 0;
    } else if (parpadeo == 0) {
      LCD_Print(text1, 50, 170, 1, 0x00, 0x00);   //SE CAMBIA EL COLOR DEL TEXTO AL MISMO DEL FONDO
      parpadeo = 1;
    }
    delay(500);
  }
}

void mostrar_menu_principal(void) {

  if (one_time == 1) {

    LCD_Clear(0x00);
    text1 = "MENU PRINCIPAL";
    LCD_Print(text1, 40, 15, 2, 0xffff, 0x00);

    text2 = "Seleccion de personaje";
    LCD_Print(text2, 70, 70, 1, 0xffff, 0x3176);

    text3 = "Volver a pantalla de inicio";
    LCD_Print(text3, 50, 110, 1, 0xffff, 0x00);

    text4 = "Jugar";
    LCD_Print(text4, 130, 150, 1, 0xffff, 0x00);

    one_time = 0;
  } else if (one_time == 0) {
    delay(1);
  }

}

void mostrar_seleccion_jugador(void) {
  unsigned char padawan1[911] = {0};
  unsigned char padawan2[911] = {0};
  unsigned char padawan3[911] = {0};
  unsigned char padawan4[911] = {0};
  LCD_Clear(0x00);
  //LCD_Bitmap(105, 5, 110, 50, titulo1); //TITULO DE STAR WARS
  obtener_titulo1_2();

  text1 = "Select a lightsaber";
  LCD_Print(text1, 8, 70, 2, 0xffe7, 0x0000);
  openSDformat(padawan1, 912, "P1.txt");
  openSDformat(padawan2, 912, "P2.txt");
  openSDformat(padawan3, 912, "P3.txt");
  openSDformat(padawan4, 912, "P4.txt");
  LCD_Bitmap(54, 110, 19, 24, padawan1);
  LCD_Bitmap(120, 110, 19, 24, padawan2);
  LCD_Bitmap(186, 110, 19, 24, padawan3);
  LCD_Bitmap(252, 110, 19, 24, padawan4); // COLOCA LOS PADAWAN DE DIFERENTES ESPADAS
  text2 = "P1";
  text3 = "P2";
  LCD_Print(text2, 45, 140, 2, 0x7800, 0x0000);
  LCD_Print(text3, 45, 165, 2, 0x000a, 0x0000);
}

void mostrar_pantalla_juego(void) {
  unsigned char fondo[3199] = {0};
  openSDformat(fondo, 3200, "Fondo.txt");
  if (one_time == 1) {
    for (int x = 0; x < 319; x++) {     // ESTRELLAS
      LCD_Bitmap(x, 0, 40, 40, fondo);
      LCD_Bitmap(x, 40, 40, 40, fondo);

      x += 39;//PINTA EL GRID
    }
    FillRect(0, 80, 320, 170, 0x00);   // SUPERFICIE NEGRA
    FillRect(0, 170, 320, 240, 0xffff);   // SUPERFICIE BLANCA

    one_time = 0;

  } else if (one_time == 0) {
    delay(1);
  }
}
//***************************************************************************************************************************************
//FUNCION PARA MOVER EL CURSOR DE SELECCION
//***************************************************************************************************************************************
void mover_seleccionador(void) {
  switch (seleccionador) {
    case 1:
      LCD_Print(text2, 70, 70, 1, 0xffff, 0x3176);
      LCD_Print(text3, 50, 110, 1, 0xffff, 0x00);
      LCD_Print(text4, 130, 150, 1, 0xffff, 0x00);
      break;

    case 2:
      LCD_Print(text2, 70, 70, 1, 0xffff, 0x00);
      LCD_Print(text3, 50, 110, 1, 0xffff, 0x3176);
      LCD_Print(text4, 130, 150, 1, 0xffff, 0x00);
      break;

    case 3:
      LCD_Print(text2, 70, 70, 1, 0xffff, 0x00);
      LCD_Print(text3, 50, 110, 1, 0xffff, 0x00);
      LCD_Print(text4, 130, 150, 1, 0xffff, 0x3176);
      break;

    case 4:
      seleccionador = 0;
      break;

  }
}

void seleccion_jugador(void) {
  int rebote1 = 0;
  int rebote2 = 0;
  int rebote3 = 0;
  int rebote4 = 0;
  int seleccionado1 = 0;
  int seleccionado2 = 0;

  while (modo == 2) {
    //45, 111, 178, 244 ---- 66
    // Jugador 1
    // Boton mover jugador
    if (digitalRead(B2_1) == LOW) {
      rebote1 = 1;  // Variable para antirrebote
    }

    if ((digitalRead(B2_1) == HIGH) && (rebote1 == 1) && !seleccionado1) {
      rebote1 = 0;
      jugador1++;
      for (int i = 45; i < 45 + 66 * jugador1; i++) {
        V_line(i, 140, 15, 0x0000);
      }
      if (jugador1 == 4) {
        jugador1 = 0;
      }
      LCD_Print(text2, 45 + 66 * jugador1, 140, 2, 0x7800, 0x0000);
    }

    // Boton de seleccionar jugador
    if (digitalRead(B1_1) == LOW) {
      rebote3 = 1;
    }

    if (digitalRead(B1_1) == HIGH && rebote3 == 1) {
      rebote3 = 0;
      LCD_Print(text2, 45 + 66 * jugador1, 140, 2, 0x7800, 0xfbae);
      seleccionado1 = 1;
    }

    // Jugador 2
    // Boton mover jugador
    if (digitalRead(B1_2) == LOW) {
      rebote2 = 1;  // Variable para antirrebote
    }

    if ((digitalRead(B1_2) == HIGH) && (rebote2 == 1) && !seleccionado2) {
      rebote2 = 0;
      jugador2++;
      for (int i = 45; i < 45 + 66 * jugador2; i++) {
        V_line(i, 165, 15, 0x0000);
      }
      if (jugador2 == 4) {
        jugador2 = 0;
      }
      LCD_Print(text3, 45 + 66 * jugador2, 165, 2, 0x000a, 0x0000);
    }

    // Boton de seleccionar jugador
    if (digitalRead(B2_2) == LOW) {
      rebote4 = 1;
    }

    if (digitalRead(B2_2) == HIGH && rebote4 == 1) {
      rebote4 = 0;
      LCD_Print(text3, 45 + 66 * jugador2, 165, 2, 0x000a, 0x7e9b);
      seleccionado2 = 1;
    }

    if (seleccionado1 == 1 && seleccionado2 == 1) { // Si ya ambos jugadores seleccionaron un sable
      if (jugador1 == jugador2) { // Revisa que no sea el mismo
        seleccionado1 = 0;
        seleccionado2 = 0;
        LCD_Print(text2, 45 + 66 * jugador1, 140, 2, 0x7800, 0x0000);
        LCD_Print(text3, 45 + 66 * jugador2, 165, 2, 0x000a, 0x0000);
      }
      else {
        modo = 3;
      }
    }
  }
}

//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************

void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DPINS[i], OUTPUT);
  }
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER)
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40 | 0x80 | 0x20 | 0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
  //  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);
  LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
    }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y + h, w, c);
  V_line(x  , y  , h, c);
  V_line(x + w, y  , h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y + i, w, c);
  }
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//***************************************************************************************************************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;

  if (fontSize == 1) {
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if (fontSize == 2) {
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }

  char charInput ;
  int cLength = text.length();
  Serial.println(cLength, DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength + 1];
  text.toCharArray(char_array, cLength + 1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1) {
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2) {
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 = x + width;
  y2 = y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k + 1]);
      //LCD_DATA(bitmap[k]);
      k = k + 2;
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 =   x + width;
  y2 =    y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  int k = 0;
  int ancho = ((width * columns));
  if (flip) {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width - 1 - offset) * 2;
      k = k + width * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k - 2;
      }
    }
  } else {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width + 1 + offset) * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k + 2;
      }
    }


  }
  digitalWrite(LCD_CS, HIGH);
}

//***************************************************************************************************************************************
// Función para leer de la SD y colocarlo dentro de una variable unsigned char
// FUNCION BRINDADA POR LOS ELECTRÓNICOS
//***************************************************************************************************************************************
void openSDformat(unsigned char L[], unsigned long SIZE, char* a) {
  File dataFile = SD.open(a);     // ABRE EL ARCHIVO SOLICITADO
  unsigned long i = 0;            //GENERA UN INDICADOR
  char L_HEX[] = {0, 0};          //HACE UN ARREGLO DE 2 NUMEROS POR POCICION
  int P2, P1;                     //ASIGNA UNA VARIABLE PARA CADA NUMERO
  if (dataFile) {                 //SI EXISTE
    do {
      L_HEX[0] = dataFile.read(); //LEE
      P1 = ACII_HEX(L_HEX);       //TRANSOFRMA
      L_HEX[0] = dataFile.read(); //LEE
      P2 = ACII_HEX(L_HEX);       //TRANSFORMA
      L[i] = (P1 << 4) | (P2 & 0xF);  //CONCATENA
      i++;                        //MUEVE EL INDICADOR
    } while (i < (SIZE + 1));
  }
  dataFile.close();                       // Se cierra archivo.
}
//***************************************************************************************************************************************
// TRANSFORMAR
//***************************************************************************************************************************************
int ACII_HEX(char *puntero) {
  int i = 0;
  for (;;) {
    char num = *puntero;
    if (num >= '0' && num <= '9') {
      i *= 16;
      i += num - '0';
    }
    else if (num >= 'a' && num <= 'f') {
      i *= 16;
      i += (num - 'a') + 10;
    }
    else break;
    puntero++;
  }
  return i;
}
//************************************************************************************************
// DIRECTORIO DE LA SD                                                                           *
//************************************************************************************************
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
