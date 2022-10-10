/*

Conversor de teclado ZX Spectrum a PS/2 
---------------------------------------

* Codigo base original por Quest
* Desarrollo inicial y primeros atajos de teclado por Neuro (@neurorulez)
* Mejoras, optimizaciones y nuevos atajos de teclado por @spark2k06
* Nueva tecla de funcion y otras teclas de ZX-Spectrum para teclado +UNO por David Carrion
* Inclusion de USB por Guillermo Asin Prieto

*/
#include <stdio.h>
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "Keyboard.h"
#include "config.h"
#include "keymaps.h"

#define HI 1
#define LO 0
#define _IN 1
#define _OUT 0

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00

//Teclas Pulsadas en el ultimo pase
// Pulsada              -> 0b00000001 = 0x01
// Mantenida            -> 0b00000010 = 0x02
// Liberada             -> 0b00000100 = 0x04
// CapsShift            -> 0b00001000 = 0x08 (Este flag solo se tiene en cuenta en modos distintos al de ZX)
// SymbolShift          -> 0b00010000 = 0x10 (Este flag solo se tiene en cuenta en modos distintos al de ZX)

KeyReport report;

uint8_t matriz[ROWS][COLS];
uint8_t opqa_cursors = 0;

unsigned char espera = 0;
unsigned char fnpulsada = 0;
unsigned char fnpulsando = 0;

uint8_t       keyxfnpulsada = 0;

KBMODE modo; //Modo teclado 0=ZX=NATIVO /Resto otros mapas
uint8_t cambiomodo = 0;
uint8_t soltarteclas;
uint8_t cs_counter = 0, ss_counter = 0;

unsigned char   hostdata = 0;
unsigned char   hostdataAnt;
unsigned char   codeset = 2;
unsigned char   antighosting = 0;
unsigned char   kbescucha = 0;
uint32_t        timeout_escucha = 0;
uint16_t        typematic = 0;
uint16_t        typematicfirst = 0;
unsigned char   typematic_code = 0;
uint8_t         typematic_codeaux = 0;
uint8_t         kbalt = 0;
uint8_t         prevf0 = 0;

//Teclas Modificadoras (para teclado spectrum)
unsigned char CAPS_SHIFT = KEY_LSHIFT;  //Caps Shift   (NO necesita E0)
//unsigned char SYMBOL_SHIFT = KEY_LCTRL; //Symbol Shift (NO necesita E0)
unsigned char SYMBOL_SHIFT = KEY_RCTRL; //Symbol Shift (NO necesita E0)
//unsigned char CAPS_SHIFT = KEY_LSHIFT;  //Caps Shift   (NO necesita E0)
//unsigned char SYMBOL_SHIFT = KEY_RCTRL; //Symbol Shift (NO necesita E0)

                                        //Fn (Funcion)
#define FN_ROW 8   
#define FN_COL 2  

                                        //Caps Shift (CAPS_SHIFT)
#define CAPS_SHIFT_ROW 5  
#define CAPS_SHIFT_COL 0  

                                        //Symbol Shift (SYMBOL_SHIFT)
#define SYMBOL_SHIFT_ROW 7   
#define SYMBOL_SHIFT_COL 1   

                                        //SPACE (Escape)
#define SPACE_ROW 7 
#define SPACE_COL 0 

                                        //ENTER
#define ENTER_ROW 6 
#define ENTER_COL 0 

                                        //Row 1..5
#define N1_N5_ROW 0
                                        //Cols 1..5
#define N1_COL 0 //
#define N2_COL 1 //
#define N3_COL 2 //
#define N4_COL 3 //
#define N5_COL 4 //

                                        //Row 6..0
#define N6_N0_ROW 3
                                        //Cols 6..0
#define N6_COL 4 //
#define N7_COL 3 //
#define N8_COL 2 //
#define N9_COL 1 //
#define N0_COL 0 //

                                        //Row Q-T
#define Q_T_ROW 1
                                        //Cols Q-T
#define Q_COL 0 //
#define W_COL 1 //
#define E_COL 2 //
#define R_COL 3 //
#define T_COL 4 //

                                        //Row Y-P
#define Y_P_ROW 4
                                        //Cols Y-P
#define Y_COL 4 //
#define U_COL 3 //
#define I_COL 2 //
#define O_COL 1 //
#define P_COL 0 //

                                        //Row A-G
#define A_G_ROW 2
                                        //Cols A-G
#define A_COL 0 //
#define S_COL 1 //
#define D_COL 2 //
#define F_COL 3 //
#define G_COL 4 //

                                        //Row H-L
#define H_L_ROW 6
                                        //Cols H-L
#define H_COL 4 //
#define J_COL 3 //
#define K_COL 2 //
#define L_COL 1 //

                                        //Row Z-V
#define Z_V_ROW 5 //
                                        //Cols Z-V
#define Z_COL 1 //
#define X_COL 2 //
#define C_COL 3 //
#define V_COL 4 //

                                        //Row B-M
#define B_M_ROW 7
                                        //Cols B-M
#define B_COL 4 //
#define N_COL 3 //
#define M_COL 2 //
#define SP_COL 0 //






uint8_t K[255], KE[255];

void setupKeymaps() {
  K[KEY_ESCAPE]=KEY_USB_ESCAPE;
  K[KEY_DELETE]=KEY_USB_DELETE;
  K[KEY_BACKSP]=KEY_USB_BACKSP;
  K[KEY_SCRLCK]=KEY_USB_SCRLCK;
  
  K[KEY_LCTRL]=KEY_USB_LCTRL;
  K[KEY_LALT]=KEY_USB_LALT;
  
                        //Especiales, requieren E0                          //Especiales, requieren E0
  K[KEY_RIGHT]=KEY_USB_RIGHT;
  K[KEY_LEFT]=KEY_USB_LEFT;
  K[KEY_DOWN]=KEY_USB_DOWN;
  K[KEY_UP]=KEY_USB_UP;
  K[KEY_RCTRL]=KEY_USB_RCTRL;
  K[KEY_RALT]=KEY_USB_RALT;
  K[KEY_LWIN]=KEY_USB_LWIN;
  K[KEY_RWIN]=KEY_USB_RWIN;
  K[KEY_APPS]=KEY_USB_APPS;
  K[KEY_PGUP]=KEY_USB_PGUP;
  K[KEY_PGDW]=KEY_USB_PGDW;
  K[KEY_HOME]=KEY_USB_HOME;
  K[KEY_END]=KEY_USB_END;
  K[KEY_INS]=KEY_USB_INS;
  K[KEY_DELETE]=KEY_USB_DELETE;
                        //Fin Especiales                          //Fin Especiales
  
  K[KEY_A]=KEY_USB_A;
  K[KEY_B]=KEY_USB_B;
  K[KEY_C]=KEY_USB_C;
  K[KEY_D]=KEY_USB_D;
  K[KEY_E]=KEY_USB_E;
  K[KEY_F]=KEY_USB_F;
  K[KEY_G]=KEY_USB_G;
  K[KEY_H]=KEY_USB_H;
  K[KEY_I]=KEY_USB_I;
  K[KEY_J]=KEY_USB_J;
  K[KEY_K]=KEY_USB_K;
  K[KEY_L]=KEY_USB_L;
  K[KEY_M]=KEY_USB_M;
  K[KEY_N]=KEY_USB_N;
  K[KEY_O]=KEY_USB_O;
  K[KEY_P]=KEY_USB_P;
  K[KEY_Q]=KEY_USB_Q;
  K[KEY_R]=KEY_USB_R;
  K[KEY_S]=KEY_USB_S;
  K[KEY_T]=KEY_USB_T;
  K[KEY_U]=KEY_USB_U;
  K[KEY_V]=KEY_USB_V;
  K[KEY_W]=KEY_USB_W;
  K[KEY_X]=KEY_USB_X;
  K[KEY_Y]=KEY_USB_Y;
  K[KEY_Z]=KEY_USB_Z;
  K[KEY_1]=KEY_USB_1;
  K[KEY_2]=KEY_USB_2;
  K[KEY_3]=KEY_USB_3;
  K[KEY_4]=KEY_USB_4;
  K[KEY_5]=KEY_USB_5;
  K[KEY_6]=KEY_USB_6;
  K[KEY_7]=KEY_USB_7;
  K[KEY_8]=KEY_USB_8;
  K[KEY_9]=KEY_USB_9;
  K[KEY_0]=KEY_USB_0;
  
  K[KEY_ENTER]=KEY_USB_ENTER;
  K[KEY_SPACE]=KEY_USB_SPACE;
  
  K[KEY_F1]=KEY_USB_F1;
  K[KEY_F2]=KEY_USB_F2;
  K[KEY_F3]=KEY_USB_F3;
  K[KEY_F4]=KEY_USB_F4;
  K[KEY_F5]=KEY_USB_F5;
  K[KEY_F6]=KEY_USB_F6;
  K[KEY_F7]=KEY_USB_F7;
  K[KEY_F8]=KEY_USB_F8;
  K[KEY_F9]=KEY_USB_F9;
  K[KEY_F10]=KEY_USB_F10;
  K[KEY_F11]=KEY_USB_F11;
  K[KEY_F12]=KEY_USB_F12;
  
  K[KEY_LSHIFT]=KEY_USB_LSHIFT;
  K[KEY_RSHIFT]=KEY_USB_RSHIFT;
  
  K[KEY_CAPS]=KEY_USB_CAPS;
  
  K[KEY_TAB]=KEY_USB_TAB;
  
  K[KEY_TLD]=KEY_USB_TLD;//Izxda del 1
  K[KEY_MENOS]=KEY_USB_MENOS;//Drcha del 0
  K[KEY_IGUAL]=KEY_USB_IGUAL;//Izda de Backspace
  K[KEY_ACORCHE]=KEY_USB_ACORCHE;//Drcha de la P
  K[KEY_CCORCHE]=KEY_USB_CCORCHE;//Siguiente a la de la Drcha de la P
  K[KEY_BKSLASH]=KEY_USB_BKSLASH;//Izda del Enter (Puede estar en la fila de la P o de la L
  K[KEY_PTOCOMA]=KEY_USB_PTOCOMA;//La Ñ
  K[KEY_COMILLA]=KEY_USB_COMILLA;//Derecha de la Ñ
  K[KEY_COMA]=KEY_USB_COMA;//Decha de la M
  K[KEY_PUNTO]=KEY_USB_PUNTO;//Siguiente del de la Derecha de la M
  K[KEY_SLASH]=KEY_USB_SLASH;//Izda del Shift Derecho
  K[KEY_LESS]=KEY_USB_LESS;

  K[KEY_NUMLCK]=KEY_USB_NUMLCK;
  
}

void pinSet(uint8_t pin, uint8_t bcd, uint8_t stat) //stat 1 = in, stat 0 = out
{
  switch (bcd) {
  case PB:  if (stat) DDRB &= ~_BV(pin); else DDRB |= _BV(pin); break;
  case PC:  if (stat) DDRC &= ~_BV(pin); else DDRC |= _BV(pin); break;
  case PD:  if (stat) DDRD &= ~_BV(pin); else DDRD |= _BV(pin); break;

  case PE:  if (stat) DDRE &= ~_BV(pin); else DDRE |= _BV(pin); break; // Pro Micro
  case PF:  if (stat) DDRF &= ~_BV(pin); else DDRF |= _BV(pin); break; // Pro Micro
  }
}

uint8_t pinStat(uint8_t pin, uint8_t bcd)
{
  switch (bcd) {
  case PB:  if (!(PINB & (1 << pin))) return 1; else return 0; break;
  case PC:  if (!(PINC & (1 << pin))) return 1; else return 0; break;
  case PD:  if (!(PIND & (1 << pin))) return 1; else return 0; break;

  case PE:  if (!(PINE & (1 << pin))) return 1; else return 0; break; // Pro Micro
  case PF:  if (!(PINF & (1 << pin))) return 1; else return 0; break; // Pro Micro
  }
  return 0;
}

void pinPut(uint8_t pin, uint8_t bcd, uint8_t stat) //stat 1 = HI, stat 0 = LO
{
  switch (bcd) {
  case PB:  if (!stat) PORTB &= ~_BV(pin); else PORTB |= _BV(pin); break;
  case PC:  if (!stat) PORTC &= ~_BV(pin); else PORTC |= _BV(pin); break;
  case PD:  if (!stat) PORTD &= ~_BV(pin); else PORTD |= _BV(pin); break;

  case PE:  if (!stat) PORTE &= ~_BV(pin); else PORTE |= _BV(pin); break; // Pro Micro
  case PF:  if (!stat) PORTF &= ~_BV(pin); else PORTF |= _BV(pin); break; // Pro Micro
  }
}

// Copypaste from Keyboard lib
void sendReport() {
  HID().SendReport(2, &report, sizeof(KeyReport));
}

void addToReport(uint8_t k) {
  uint8_t i;
  if (k >= 224) {
    report.modifiers |= 1 << (k - 224);
  } else if (report.keys[0] != k && report.keys[1] != k &&
             report.keys[2] != k && report.keys[3] != k &&
             report.keys[4] != k && report.keys[5] != k) {
    for (i = 0; i < 6; ++i) {
      if (report.keys[i] == 0) {
        report.keys[i] = k;
        break;
      }
    }
  }
}

void removeFromReport(uint8_t k) {
  uint8_t i;
  if (k >= 224) {
    report.modifiers &= ~(1 << (k - 224));
  } else {
    for (i = 0; i < 6; ++i) {
      if (report.keys[i] == k) {
        report.keys[i] = 0;
        break;
      }
    }
  }
}
////

void sendUSB(uint8_t k){
  uint8_t key=K[k];
  addToReport(key);
  sendReport();
}

void sendStopUSB(uint8_t k){
  uint8_t key=K[k];
  removeFromReport(key);
  sendReport();
}

void ps2Mode(uint8_t pin, uint8_t mode)
{
  if (mode) PS2_DDR &= ~_BV(pin); //Hi-Entrada 
  else     PS2_DDR |= _BV(pin);  //Low-Salilda
}

void ps2Init()
{
  PS2_PORT &= ~_BV(PS2_DAT); //A 0
  PS2_PORT &= ~_BV(PS2_CLK); //A 0
  ps2Mode(PS2_DAT, HI);
  ps2Mode(PS2_CLK, HI);
}

uint8_t ps2Stat()
{
  if (!(PS2_PIN & (1 << PS2_CLK)))
    return 1;
  if (!(PS2_PIN & (1 << PS2_DAT)))
    return 1;
  return 0;
}

uint8_t checkState(uint16_t tramo) {
  uint16_t cont = 0;

  while (cont++ < tramo) {
    if (!(PS2_PIN & (1 << PS2_DAT)))
      return 1;
    _delay_us(5);
  }
  return 0;
}

void _delay_us_4usteps(uint8_t us)
{
  while (0<us)
  {
    _delay_us(4);
    us -= 4;
  }
}

//En us, reloj y semireloj, para los flancos
//Uso normal: CK1 = 20, CK2 = 40 // Para codigo sin optimizar (x12) CK1 = 240, CK2 = 480.  //JOyPs2 CK1=15 CK2=30 //Nuevo JoyPs2 CK1=16 CK2=32 //Mio CK1=4 CK2=8
#define CK1 4
#define CK2 8
uint8_t CKm = 1;  //Multiplicador de CK1 y CK2

          //envio de datos ps/2 simulando reloj con delays.
void sendPS2(unsigned char code)
{
  //Para continuar las lineas deben estar en alto
  if (ps2Stat())
    return;   
  //while (ps2Stat());

  unsigned char parity = 1;
  uint8_t i = 0;

  //iniciamos transmision
  ps2Mode(PS2_DAT, LO);
  _delay_us_4usteps(CK1*CKm);

  ps2Mode(PS2_CLK, LO); //bit de comienzo
  _delay_us_4usteps(CK2*CKm);
  ps2Mode(PS2_CLK, HI);
  _delay_us_4usteps(CK1*CKm);
  //enviamos datos
  for (i = 0; i < 8; ++i)
  {
    if (code & (1 << i))
    {
      ps2Mode(PS2_DAT, HI);
      parity = parity ^ 1;
    }
    else ps2Mode(PS2_DAT, LO);

    _delay_us_4usteps(CK1*CKm);
    ps2Mode(PS2_CLK, LO);
    _delay_us_4usteps(CK2*CKm);
    ps2Mode(PS2_CLK, HI);
    _delay_us_4usteps(CK1*CKm);
  }

  // Enviamos bit de paridad
  if (parity) ps2Mode(PS2_DAT, HI);
  else        ps2Mode(PS2_DAT, LO);

  _delay_us_4usteps(CK1*CKm);
  ps2Mode(PS2_CLK, LO);
  _delay_us_4usteps(CK2*CKm);
  ps2Mode(PS2_CLK, HI);
  _delay_us_4usteps(CK1*CKm);

  //Bit de parada
  ps2Mode(PS2_DAT, HI);
  _delay_us_4usteps(CK1*CKm);
  ps2Mode(PS2_CLK, LO);
  _delay_us_4usteps(CK2*CKm);
  ps2Mode(PS2_CLK, HI);
  _delay_us_4usteps(CK1*CKm);

  _delay_us(50);    //fin
}

int getPS2(unsigned char *ret) //Lectura de PS2 para acceso bidireccional
{
    unsigned char data = 0x00;
    unsigned char p = 0x01;
    uint8_t i = 0;

    // discard the start bit
    while ((PS2_PIN & (1 << PS2_DAT)));
    while (!(PS2_PIN & (1 << PS2_CLK)));

    // Bit de comienzo
    _delay_us_4usteps(CK1*CKm);
    ps2Mode(PS2_CLK, LO);
    _delay_us_4usteps(CK2*CKm);
    ps2Mode(PS2_CLK, HI);
    _delay_us_4usteps(CK1*CKm);

    // read each data bit
    for (i = 0; i<8; i++) {
        if ((PS2_PIN & (1 << PS2_DAT))) {
            data = data | (1 << i);
            p = p ^ 1;
        }
        _delay_us_4usteps(CK1*CKm);
        ps2Mode(PS2_CLK, LO);
        _delay_us_4usteps(CK2*CKm);
        ps2Mode(PS2_CLK, HI);
        _delay_us_4usteps(CK1*CKm);
    }

    // read the parity bit  
    if (((PS2_PIN & (1 << PS2_DAT)) != 0) != p) {
        return -1;
    }
    _delay_us_4usteps(CK1*CKm);
    ps2Mode(PS2_CLK, LO);
    _delay_us_4usteps(CK2*CKm);
    ps2Mode(PS2_CLK, HI);
    _delay_us_4usteps(CK1*CKm);

    // send 'ack' bit
    ps2Mode(PS2_DAT, LO);
    _delay_us_4usteps(CK1*CKm);
    ps2Mode(PS2_CLK, LO);
    _delay_us_4usteps(CK2*CKm);
    ps2Mode(PS2_CLK, HI);
    ps2Mode(PS2_DAT, HI);
    _delay_us(100);

    *ret = data;
    return 0;
}

void imprimeversion() //Imprime la fecha de la version en modos que no sean ZX ni PC
{
  int n;
  char pausa = 50;
  if (!modo)
  {
    sendPS2(0xF0); sendPS2(CAPS_SHIFT); matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0;
    sendPS2(0xF0); sendPS2(SYMBOL_SHIFT); matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0;
    sendStopUSB(CAPS_SHIFT);sendStopUSB(SYMBOL_SHIFT);
  }

  sendPS2(KEY_SPACE); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_SPACE);
  sendUSB(KEY_SPACE);sendStopUSB(KEY_SPACE);
  
  for (n = 0; n<8; n++)
  {
    if (n == 2 || n == 4) { _delay_ms(pausa); sendPS2(KEY_PUNTO); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_PUNTO); }
    _delay_ms(pausa);
    sendPS2(versionKeyCodes[version[n]]);
    sendUSB(versionKeyCodes[version[n]]);
    _delay_ms(pausa);
    sendPS2(0xF0);
    sendPS2(versionKeyCodes[version[n]]);
    sendStopUSB(versionKeyCodes[version[n]]);
    _delay_ms(pausa);    
   
  }

  sendPS2(KEY_SPACE); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_SPACE);
  sendUSB(KEY_SPACE);sendStopUSB(KEY_SPACE);
  sendPS2(KEY_R); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_R);
  sendUSB(KEY_R);sendStopUSB(KEY_R);
  _delay_ms(pausa);
  sendPS2(versionKeyCodes[rama[0]]);
  sendUSB(versionKeyCodes[rama[0]]);
  _delay_ms(pausa);
  sendPS2(0xF0);
  sendPS2(versionKeyCodes[rama[0]]);
  sendStopUSB(versionKeyCodes[rama[0]]);
  _delay_ms(pausa);   
 
  fnpulsada = 1;
  fnpulsando = 1;
}

void eepromsave() //Imprime ' .CFGFLASHED' y guarda en la EEPROM el modo actual
{
  int n;
  char pausa = 50;
  if (!modo)
  {
    sendPS2(0xF0); sendPS2(CAPS_SHIFT); matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0;
    sendPS2(0xF0); sendPS2(SYMBOL_SHIFT); matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0;
    sendStopUSB(CAPS_SHIFT);sendStopUSB(SYMBOL_SHIFT);
  }
  eeprom_write_byte((uint8_t*)5, (uint8_t)modo);
  if (codeset == 2)
  {
    _delay_ms(pausa); sendPS2(KEY_SPACE); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_SPACE);
    _delay_ms(pausa); sendPS2(KEY_PUNTO); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_PUNTO);
    sendUSB(KEY_SPACE);sendUSB(KEY_PUNTO);
    sendStopUSB(KEY_SPACE);sendStopUSB(KEY_PUNTO);
    for (n = 0; n < 10; n++)
    {
      _delay_ms(pausa);
      sendPS2(eepromsavename[n]);
      sendUSB(eepromsavename[n]);
      _delay_ms(pausa);
      sendPS2(0xF0);
      sendPS2(eepromsavename[n]);
      sendStopUSB(eepromsavename[n]);
      _delay_ms(pausa);
    }
  }
  fnpulsada = 1;
  fnpulsando = 1;
}

void imprimecore(const uint8_t nomcore[]) //Imprime el nombre del core
{
  int n;
  char pausa = 100;

  if (!modo)
  {
    sendPS2(0xF0); sendPS2(CAPS_SHIFT); matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0;
    sendPS2(0xF0); sendPS2(SYMBOL_SHIFT); matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0;
    sendStopUSB(CAPS_SHIFT);sendStopUSB(SYMBOL_SHIFT);
  }

  _delay_ms(pausa); sendPS2(KEY_SPACE); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_SPACE);
  _delay_ms(pausa); sendPS2(KEY_PUNTO); _delay_ms(pausa); sendPS2(0xF0); sendPS2(KEY_PUNTO);
  sendUSB(KEY_SPACE);sendUSB(KEY_PUNTO);
  sendStopUSB(KEY_SPACE);sendStopUSB(KEY_PUNTO);
  for (n = 1; n<nomcore[0] + 1; n++)
  {
    _delay_ms(pausa);
    sendPS2(nomcore[n]);
    sendUSB(nomcore[n]);
    _delay_ms(pausa);
    sendPS2(0xF0);
    sendPS2(nomcore[n]);
    sendStopUSB(nomcore[n]);
    _delay_ms(pausa);
  }
  CKm = nomcore[nomcore[0] + 1]; //Valor de CKm en la configuracion de nomcore[]

}

//Inicializar Matriz
void matrixInit()
{
    uint8_t c, r;

    for (c = 0; c<COLS; c++)
    {
        pinSet(pinsC[c], bcdC[c], _IN);
        pinPut(pinsC[c], bcdC[c], HI);
    }

    for (r = 0; r < ROWS; r++)
    {
        pinSet(pinsR[r], bcdR[r], _IN);
    }
}

KBMODE cambiarmodo2(KBMODE modokb)
{
  kbescucha = 0;
  opqa_cursors = 0;
  if (modokb == zx)  CKm = nomZX[nomZX[0] + 1];
  if (modokb == cpc) CKm = nomCPC[nomCPC[0] + 1];
  if (modokb == msx) CKm = nomMSX[nomMSX[0] + 1];
  if (modokb == c64) CKm = nomC64[nomC64[0] + 1];
  if (modokb == at8) CKm = nomAT8[nomAT8[0] + 1];
  if (modokb == bbc) CKm = nomBBC[nomBBC[0] + 1];
  if (modokb == aco) CKm = nomACO[nomACO[0] + 1];
  if (modokb == ap2) CKm = nomAP2[nomAP2[0] + 1];
  if (modokb == vic) CKm = nomVIC[nomVIC[0] + 1];
  if (modokb == ori) CKm = nomORI[nomORI[0] + 1];
  if (modokb == sam) CKm = nomSAM[nomSAM[0] + 1];
  if (modokb == jup) CKm = nomJUP[nomJUP[0] + 1];
  if (modokb == fus) CKm = nomFUS[nomFUS[0] + 1];
  if (modokb == pc) { CKm = nomPC[nomPC[0] + 1]; kbescucha = 1; timeout_escucha = 0; codeset = 2; } // Iniciamos la escucha para que se pueda cambiar al core de PC/XT.
  if (modokb == kbext) { CKm = nomKBEXT[nomKBEXT[0] + 1]; kbescucha = 1; timeout_escucha = 0; codeset = 2; } // Iniciamos la escucha.
  if (modokb == pcxt) { CKm = nomPCXT[nomPCXT[0] + 1]; kbescucha = 0; codeset = 1; imprimecore(nomPCXT); } // Sin escucha activa para ser usado de forma simultanea junto a un teclado externo.

  return modokb;
}

KBMODE cambiarmodo(KBMODE modokb)
{
  KBMODE auxmodo = modo;
  kbescucha = 0;
  opqa_cursors = 0;
  if (modokb == zx)  imprimecore(nomZX);
  if (modokb == cpc) imprimecore(nomCPC);
  if (modokb == msx) imprimecore(nomMSX);
  if (modokb == c64) imprimecore(nomC64);
  if (modokb == at8) imprimecore(nomAT8);
  if (modokb == bbc) imprimecore(nomBBC);
  if (modokb == aco) imprimecore(nomACO);
  if (modokb == ap2) imprimecore(nomAP2);
  if (modokb == vic) imprimecore(nomVIC);
  if (modokb == ori) imprimecore(nomORI);
  if (modokb == sam) imprimecore(nomSAM);
  if (modokb == jup) imprimecore(nomJUP);
  if (modokb == fus) imprimecore(nomFUS);
  if (modokb == pc) { kbescucha = 1; timeout_escucha = 0; codeset = 2; imprimecore(nomPC); } // Iniciamos la escucha para que se pueda cambiar al core de PC/XT.
  if (modokb == pcxt) { kbescucha = 0; codeset = 1; imprimecore(nomPCXT); } // Sin escucha activa para ser usado de forma simultanea junto a un teclado externo.
  if (modokb == kbext) { kbescucha = 1; timeout_escucha = 0; codeset = 2; imprimecore(nomKBEXT); } // Iniciamos la escucha.
  
  if (modokb > kbext) modokb = auxmodo; // Si no se trata de un modo conocido, mantenemos el anterior.
                                                 
  //Uso normal: CK1 = 20, CK2 = 40 // Para codigo sin optimizar (x12) CK1 = 240, CK2 = 480.  //JOyPs2 CK1=15 CK2=30 //Mio CK1=4 CK2=8
  //if(modokb>0) CKm=4; else CKm=1; //Se coge del Nombrecore[]
  
  fnpulsada = 1;
  fnpulsando = 1;
  cambiomodo = 0; //para salir del bucle del cambiomodo
  
  return modokb;

}

void pulsaimprpant(unsigned char row, unsigned char col)
{
    sendPS2(0xE0); sendPS2(0x12); sendPS2(0xE0); sendPS2(0x7C);
    _delay_ms(100);
    sendPS2(0xE0); sendPS2(0xF0); sendPS2(0x12); sendPS2(0xE0); sendPS2(0xF0); sendPS2(0x7C);
}

void pulsafn(unsigned char row, unsigned char col, unsigned char key, unsigned char key_E0, unsigned char shift, unsigned char ctrl, unsigned char alt, unsigned char useg)
{
  if (espera) { _delay_us(5); espera = 0; }
  if (shift) { if (codeset == 2) { sendPS2(KEY_LSHIFT); sendUSB(KEY_LSHIFT); espera++; } else { sendPS2(KS1_LSHIFT); espera++; } }//El Shift no necesita E0
  if (ctrl) { if (codeset == 2) { sendPS2(0xE0); sendPS2(KEY_RCTRL); sendUSB(KEY_RCTRL);   espera++; } else { sendPS2(0xE0); sendPS2(KS1_RCTRL);  espera++; } }//Se manda E0 para el control derecho (que vale para ambos casos)
  if (alt) { if (codeset == 2) { sendPS2(KEY_LALT);  sendUSB(KEY_LALT);    espera++; } else { sendPS2(KS1_LALT);   espera++; } }//Usamos el Alt izdo siempre
  if (espera) { _delay_us(5); espera = 0; }
  if (key_E0) { sendPS2(0xE0); } //La tecla requiere modo E0 del PS2
  sendPS2(key);
  sendUSB(key);
  _delay_ms(150);//50);
  if (key_E0) { sendPS2(0xE0); }
  if (codeset == 2) sendPS2(0xF0);
  if (codeset == 2) sendPS2(key);  else sendPS2(key + KS1_RELEASE);
  sendStopUSB(key);
  if (!modo) matriz[row][col] = 0; //originalmente es incondicional, no comprueba el modo
  if (shift) { if (codeset == 2) { sendPS2(0xF0); sendPS2(KEY_LSHIFT); sendStopUSB(KEY_LSHIFT); } else { sendPS2(KS1_LSHIFT + KS1_RELEASE); } }
  if (ctrl) { if (codeset == 2) { sendPS2(0xE0); sendPS2(0xF0); sendPS2(KEY_RCTRL); sendStopUSB(KEY_RCTRL);  } else { sendPS2(0xE0); sendPS2(KS1_RCTRL + KS1_RELEASE); } }
  if (alt) { if (codeset == 2) { sendPS2(0xF0); sendPS2(KEY_LALT); sendStopUSB(KEY_LALT);   } else { sendPS2(KS1_RALT + KS1_RELEASE); } }
  _delay_us(5);
  fnpulsada = 1;
  fnpulsando = 1;
  kbalt = 0;
  soltarteclas = 1; // Forzamos a que despues todas las teclas esten soltadas para evitar que se quede pulsada la letra asociada al combo.  
  //keyxfnpulsada = 0;
}

unsigned char traducekey(unsigned char key, KBMODE modokb) // con esta funcion ahorramos muchas matrices de mapas y por tanto memoria dinamica del AVR
{
  // Se hace OR 0x80 al key que no requiera shift (KEY_F7 es el unico scancode incompatible, ya se resolveria en caso de necesidad)
  // combinaciones no usables, comun a todos los cores
  key =
    key == KEY_E ? 0 : key == KEY_W ? 0 : key == KEY_Q ? 0 :
    key == KEY_I ? 0 : key == KEY_SPACE ? 0 : key == KEY_ENTER ? 0 : key;

  if ((modokb == pc || modokb == pcxt || modokb == kbext) && codeset == 1) return key; // pcxt tiene su propio mapa con scancodes distintos

  // combinaciones numericas comunes a todos los cores
  if (key == KEY_1 || key == KEY_3 || key == KEY_4 || key == KEY_5) return key;

  // traduccion de key segun el core
  if (key != 0)
  {
    switch (modokb)
    {
      case cpc:
        key =
          key == KEY_2 ? KEY_ACORCHE | 0x80 : key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_G ? KEY_BKSLASH :
          key == KEY_F ? KEY_CCORCHE : key == KEY_D ? KEY_LESS | 0x80 : key == KEY_S ? KEY_ACORCHE : key == KEY_A ? KEY_LESS :
          key == KEY_Y ? KEY_CCORCHE | 0x80 : key == KEY_U ? KEY_BKSLASH | 0x80 : key == KEY_O ? KEY_COMILLA | 0x80 : key == KEY_P ? KEY_2 :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_X ? KEY_IGUAL : key == KEY_Z ? KEY_PTOCOMA | 0x80 :
          key == KEY_H ? KEY_IGUAL | 0x80 : key == KEY_J ? KEY_MENOS | 0x80 : key == KEY_K ? KEY_COMILLA : key == KEY_L ? KEY_MENOS :
          key == KEY_B ? KEY_PTOCOMA : key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case msx:
        key =
          key == KEY_2 ? KEY_COMILLA : key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_G ? KEY_ACORCHE :
          key == KEY_F ? KEY_TLD | 0x80 : key == KEY_D ? KEY_BKSLASH | 0x80 : key == KEY_S ? KEY_BKSLASH : key == KEY_A ? KEY_CCORCHE :
          key == KEY_6 ? KEY_COMILLA | 0x80 : key == KEY_7 ? KEY_PTOCOMA : key == KEY_8 ? KEY_0 : key == KEY_0 ? KEY_IGUAL | 0x80 :
          key == KEY_Y ? KEY_2 : key == KEY_U ? KEY_ACORCHE | 0x80 : key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_8 :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_IGUAL : key == KEY_H ? KEY_7 :
          key == KEY_J ? KEY_MENOS | 0x80 : key == KEY_K ? KEY_TLD : key == KEY_L ? KEY_6 : key == KEY_B ? KEY_9 :
          key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 : 0;
        break;

      case c64:
        key =
          key == KEY_2 ? KEY_ACORCHE | 0x80 : key == KEY_T ? KEY_COMA : key == KEY_R ? KEY_LESS : key == KEY_S ? KEY_MENOS :
          key == KEY_Y ? KEY_PTOCOMA : key == KEY_U ? KEY_COMILLA : key == KEY_O ? KEY_COMILLA | 0x80 : key == KEY_P ? KEY_2 :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_PTOCOMA | 0x80 : key == KEY_H ? KEY_BKSLASH | 0x80 :
          key == KEY_J ? KEY_MENOS | 0x80 : key == KEY_K ? KEY_F10 | 0x80 : key == KEY_L ? KEY_IGUAL | 0x80 : key == KEY_B ? KEY_CCORCHE | 0x80 :
          key == KEY_N ? KEY_LESS | 0x80 : key == KEY_M ? KEY_COMA | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case at8:
        key =
          key == KEY_2 ? KEY_8 : key == KEY_T ? KEY_IGUAL | 0x80 : key == KEY_R ? KEY_MENOS | 0x80 : key == KEY_W ? KEY_F12 | 0x80 :
          key == KEY_Q ? KEY_F11 | 0x80 : key == KEY_D ? KEY_COMILLA : key == KEY_S ? KEY_CCORCHE : key == KEY_Y ? KEY_COMA :
          key == KEY_U ? KEY_PUNTO : key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_2 : key == KEY_V ? KEY_SLASH | 0x80 :
          key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_PTOCOMA : key == KEY_H ? KEY_BKSLASH : key == KEY_J ? KEY_ACORCHE | 0x80 :
          key == KEY_K ? KEY_COMILLA | 0x80 : key == KEY_L ? KEY_CCORCHE | 0x80 : key == KEY_B ? KEY_BKSLASH | 0x80 : key == KEY_N ? KEY_COMA | 0x80 :
          key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case bbc:
        key =
          key == KEY_2 ? KEY_TLD | 0x80 : key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_W ? KEY_F12 | 0x80 :
          key == KEY_Q ? KEY_F12 : key == KEY_S ? KEY_LESS : key == KEY_A ? KEY_MENOS | 0x80 : key == KEY_O ? KEY_PTOCOMA | 0x80 :
          key == KEY_P ? KEY_2 : key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_X ? KEY_BKSLASH :
          key == KEY_Z ? KEY_COMILLA | 0x80 : key == KEY_H ? KEY_IGUAL | 0x80 : key == KEY_J ? KEY_BKSLASH | 0x80 : key == KEY_K ? KEY_PTOCOMA :
          key == KEY_L ? KEY_MENOS : key == KEY_B ? KEY_COMILLA : key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case aco:
        key =
          key == KEY_2 ? KEY_0 : key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_D ? KEY_BKSLASH | 0x80 :
          key == KEY_Y ? KEY_ACORCHE : key == KEY_U ? KEY_CCORCHE | 0x80 : key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_2 :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_X ? KEY_ACORCHE : key == KEY_Z ? KEY_COMILLA | 0x80 :
          key == KEY_J ? KEY_MENOS | 0x80 : key == KEY_K ? KEY_PTOCOMA : key == KEY_L ? KEY_MENOS : key == KEY_B ? KEY_COMILLA :
          key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case ap2:
        key =
          key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_6 ? KEY_7 : key == KEY_7 ? KEY_COMILLA | 0x80 :
          key == KEY_8 ? KEY_9 : key == KEY_9 ? KEY_0 : key == KEY_0 ? KEY_MENOS : key == KEY_Y ? KEY_ACORCHE | 0x80 :
          key == KEY_U ? KEY_CCORCHE | 0x80 : key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_COMILLA : key == KEY_V ? KEY_SLASH | 0x80 :
          key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_PTOCOMA : key == KEY_H ? KEY_6 : key == KEY_J ? KEY_MENOS | 0x80 :
          key == KEY_K ? KEY_IGUAL : key == KEY_L ? KEY_IGUAL | 0x80 : key == KEY_B ? KEY_8 : key == KEY_N ? KEY_COMA | 0x80 :
          key == KEY_M ? KEY_PUNTO | 0x80 : key == KEY_2 ? key : 0;
        break;

      case vic:
        key =
          key == KEY_2 ? KEY_ACORCHE | 0x80 : key == KEY_T ? KEY_COMA : key == KEY_R ? KEY_LESS : key == KEY_S ? KEY_IGUAL :
          key == KEY_Y ? KEY_PTOCOMA : key == KEY_U ? KEY_COMILLA : key == KEY_O ? KEY_COMILLA | 0x80 : key == KEY_P ? KEY_2 :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_PTOCOMA | 0x80 : key == KEY_J ? KEY_IGUAL | 0x80 :
          key == KEY_K ? KEY_MENOS | 0x80 : key == KEY_L ? KEY_BKSLASH | 0x80 : key == KEY_B ? KEY_CCORCHE | 0x80 : key == KEY_N ? KEY_LESS | 0x80 :
          key == KEY_M ? KEY_COMA | 0x80 :
          (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9 || key == KEY_0) ? key : 0;
        break;

      case ori:
        key =
          key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_G ? KEY_CCORCHE : key == KEY_F ? KEY_ACORCHE :
          key == KEY_D ? KEY_BKSLASH | 0x80 : key == KEY_S ? KEY_BKSLASH : key == KEY_6 ? KEY_7 : key == KEY_7 ? KEY_COMILLA | 0x80 :
          key == KEY_8 ? KEY_9 : key == KEY_9 ? KEY_0 : key == KEY_Y ? KEY_ACORCHE | 0x80 : key == KEY_U ? KEY_CCORCHE | 0x80 :
          key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_COMILLA : key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH :
          key == KEY_X ? KEY_MENOS : key == KEY_Z ? KEY_PTOCOMA : key == KEY_H ? KEY_6 : key == KEY_J ? KEY_MENOS | 0x80 :
          key == KEY_K ? KEY_IGUAL : key == KEY_L ? KEY_IGUAL | 0x80 : key == KEY_B ? KEY_8 : key == KEY_N ? KEY_COMA | 0x80 :
          key == KEY_M ? KEY_PUNTO | 0x80 : key == KEY_0 ? key : 0;
        break;

      case sam:
        key =
          key == KEY_2 ? KEY_BKSLASH : key == KEY_T ? KEY_LESS : key == KEY_R ? KEY_LESS | 0x80 : key == KEY_G ? KEY_BKSLASH | 0x80 :
          key == KEY_F ? KEY_COMILLA | 0x80 : key == KEY_A ? KEY_PTOCOMA | 0x80 : key == KEY_7 ? KEY_MENOS | 0x80 : key == KEY_0 ? KEY_SLASH :
          key == KEY_O ? KEY_COMA : key == KEY_P ? KEY_2 : key == KEY_V ? KEY_7 : key == KEY_C ? KEY_MENOS :
          key == KEY_X ? KEY_ACORCHE | 0x80 : key == KEY_Z ? KEY_PUNTO : key == KEY_H ? KEY_ACORCHE : key == KEY_J ? KEY_SLASH | 0x80 :
          key == KEY_K ? KEY_CCORCHE | 0x80 : key == KEY_L ? KEY_0 : key == KEY_B ? KEY_CCORCHE : key == KEY_N ? KEY_COMA | 0x80 :
          key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_8 || key == KEY_9) ? key : 0;
        break;

      case jup:
        key =
          key == KEY_2 ? KEY_BKSLASH | 0x80 : key == KEY_T ? KEY_LESS : key == KEY_R ? KEY_LESS | 0x80 : key == KEY_F ? KEY_COMILLA :
          key == KEY_D ? KEY_TLD | 0x80 : key == KEY_7 ? KEY_MENOS | 0x80 : key == KEY_0 ? KEY_SLASH : key == KEY_Y ? KEY_ACORCHE :
          key == KEY_O ? KEY_COMA : key == KEY_P ? KEY_2 : key == KEY_V ? KEY_7 : key == KEY_C ? KEY_MENOS : key == KEY_X ? KEY_COMILLA | 0x80 :
          key == KEY_Z ? KEY_PUNTO : key == KEY_H ? KEY_ACORCHE | 0x80 : key == KEY_J ? KEY_SLASH | 0x80 : key == KEY_K ? KEY_CCORCHE | 0x80 :
          key == KEY_L ? KEY_0 : key == KEY_B ? KEY_CCORCHE : key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 :
          (key == KEY_6 || key == KEY_8 || key == KEY_9) ? key : 0;
        break;
        
      case fus:
      
      case pc:
      
      case pcxt:
      
      case kbext:
        key =
          key == KEY_0 ? KEY_MENOS : key == KEY_9 ? KEY_0 : key == KEY_8 ? KEY_9 : key == KEY_7 ? KEY_COMILLA | 0x80 :
          key == KEY_6 ? KEY_7 : key == KEY_T ? KEY_PUNTO : key == KEY_R ? KEY_COMA : key == KEY_G ? KEY_CCORCHE :
          key == KEY_F ? KEY_ACORCHE : key == KEY_D ? KEY_LESS | 0x80 : key == KEY_S ? KEY_BKSLASH : key == KEY_A ? KEY_TLD :
          key == KEY_Y ? KEY_ACORCHE | 0x80 : key == KEY_U ? KEY_CCORCHE | 0x80 : key == KEY_O ? KEY_PTOCOMA | 0x80 : key == KEY_P ? KEY_COMILLA :
          key == KEY_V ? KEY_SLASH | 0x80 : key == KEY_C ? KEY_SLASH : key == KEY_Z ? KEY_PTOCOMA : key == KEY_H ? KEY_6 :
          key == KEY_J ? KEY_MENOS | 0x80 : key == KEY_K ? KEY_IGUAL : key == KEY_L ? KEY_IGUAL | 0x80 : key == KEY_B ? KEY_8 :
          key == KEY_N ? KEY_COMA | 0x80 : key == KEY_M ? KEY_PUNTO | 0x80 : key == KEY_2 ? key : 0;
        break;

      default:
        key = 0;
        break;
    }
  }

  return key;
}

void pulsateclaconsymbol(unsigned char row, unsigned char col, KBMODE modokb)
{
  unsigned char key = 0, shift = 0;
  typematicfirst = 0;
  typematic_codeaux = 0;
  key = traducekey(mapZX[row][col], modokb);
  if (key != 0)
  {
    if ((modokb == pc || modokb == pcxt || modokb == kbext) && codeset == 1) { key = mapXT1[row][col]; shift = modXT1[row][col]; }
    else { shift = !(key & 0x80); if (!shift) key ^= 0x80; }

      if (shift) { if (codeset == 2) { sendPS2(KEY_LSHIFT); sendUSB(KEY_LSHIFT); typematic_codeaux = KEY_LSHIFT; } else { sendPS2(KS1_LSHIFT); typematic_codeaux = KS1_LSHIFT; } }   
      sendPS2(key);
      sendUSB(key);
      typematic_code = key;
  } 
}

void sueltateclaconsymbol(unsigned char row, unsigned char col, KBMODE modokb)
{
  unsigned char key = 0, shift = 0;
  typematic_code = 0;
  key = traducekey(mapZX[row][col], modokb);
  if (key != 0)
  {
    if ((modokb == pc || modokb == pcxt || modokb == kbext) && codeset == 1) { key = mapXT1[row][col]; shift = modXT1[row][col]; }
    else { shift = !(key & 0x80); if (!shift) key ^= 0x80; }

    if (codeset == 2) { sendPS2(0xF0); sendPS2(key); }
    else sendPS2(key + KS1_RELEASE);

    if (shift) { if (codeset == 2) { sendPS2(0xF0); sendPS2(KEY_LSHIFT); } else sendPS2(KS1_LSHIFT + KS1_RELEASE); }

  } 
}

void pulsateclaconshift(unsigned char row, unsigned char col, unsigned char key)
{
    typematicfirst = 0;
    typematic_codeaux = 0;
    if (!key) //si no esta mapeada saca la mayuscula
    {
        if (codeset == 2) { sendPS2(KEY_LSHIFT); typematic_codeaux = KEY_LSHIFT; } else { sendPS2(KS1_LSHIFT); typematic_codeaux = KS1_LSHIFT; }
        if (codeset == 2) { sendPS2(mapZX[row][col]); typematic_code = mapZX[row][col]; } else { sendPS2(mapSET1[row][col]); typematic_code = mapSET1[row][col]; }
    }
    else
    {
        if (codeset == 2 && (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)) { sendPS2(0xE0); typematic_codeaux = 0xE0; } //Es una tecla del codeset2 que necesita E0
        if (codeset == 1 && (key == KS1_LEFT || key == KS1_RIGHT || key == KS1_UP || key == KS1_DOWN)) { sendPS2(0xE0); typematic_codeaux = 0xE0; }//Es una tecla del codeset1 que necesita E0
        sendPS2(key);
        typematic_code = key;
    }
}

void sueltateclaconshift(unsigned char row, unsigned char col, unsigned char key)
{
    typematic_code = 0;
    if (!key) //si no esta mapeada saca la mayuscula
    {
        if (codeset == 2) { sendPS2(0xF0); sendPS2(mapZX[row][col]); sendPS2(0xF0); sendPS2(KEY_LSHIFT); }
        else { sendPS2(mapSET1[row][col] + KS1_RELEASE); sendPS2(KS1_LSHIFT + KS1_RELEASE); }
    }
    else
    {
        if (codeset == 2 && (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)) sendPS2(0xE0); //Es una tecla del codeset2 que necesita E0
        if (codeset == 1 && (key == KS1_LEFT || key == KS1_RIGHT || key == KS1_UP || key == KS1_DOWN)) sendPS2(0xE0); //Es una tecla del codeset1 que necesita E0
        if (codeset == 2) { sendPS2(0xF0); sendPS2(key); }
        else sendPS2(key + KS1_RELEASE);
    }
}

void pulsateclaconfn(unsigned char row, unsigned char col, KBMODE modokb)
{
  if (mapZX[row][col] == KEY_B) pulsafn(row, col, KEY_BACKSP, 0, 0, 1, 1, 5);    //ZXUNO Hard Reset (Control+Alt+Backsp)
  if (mapZX[row][col] == KEY_N) pulsafn(row, col, KEY_DELETE, 1, 0, 1, 1, 5);    //ZXUNO Soft Reset (Control+Alt+Supr)
  if (mapZX[row][col] == KEY_G) pulsafn(row, col, KEY_SCRLCK, 0, 0, 0, 0, 5);   //ZXUNO RGB/VGA Swich (Bloq Despl)
  if (mapZX[row][col] == KEY_1) pulsafn(row, col, KEY_F1, 0, 0, 0, 0, 5);        //F1
  if (mapZX[row][col] == KEY_2) pulsafn(row, col, KEY_F2, 0, 0, 0, 0, 5);        //F2
  if (mapZX[row][col] == KEY_3) pulsafn(row, col, KEY_F3, 0, 0, 0, 0, 5);        //F3
  if (mapZX[row][col] == KEY_4) pulsafn(row, col, KEY_F4, 0, 0, 0, 0, 5);        //F4
  if (mapZX[row][col] == KEY_5) pulsafn(row, col, KEY_F5, 0, 0, 0, 0, 5);        //F5
  if (mapZX[row][col] == KEY_6) pulsafn(row, col, KEY_F6, 0, 0, 0, 0, 5);        //F6
  if (mapZX[row][col] == KEY_7) pulsafn(row, col, KEY_F7, 0, 0, 0, 0, 5);        //F7
  if (mapZX[row][col] == KEY_8) pulsafn(row, col, KEY_F8, 0, 0, 0, 0, 5);        //F8
  if (mapZX[row][col] == KEY_9) pulsafn(row, col, KEY_F9, 0, 0, 0, 0, 5);        //F9
  if (mapZX[row][col] == KEY_0) pulsafn(row, col, KEY_F10, 0, 0, 0, 0, 5);       //F10
  if (mapZX[row][col] == KEY_Q) pulsafn(row, col, KEY_F11, 0, 0, 0, 0, 5);       //F11
  if (mapZX[row][col] == KEY_W) pulsafn(row, col, KEY_F12, 0, 0, 0, 0, 5);       //F12
  if (mapZX[row][col] == KEY_BACKSP) pulsafn(row, col, KEY_DELETE, 1, 0, 0, 0, 5);//DELETE
  //if (mapZX[row][col] == KEY_F10) pulsafn(row, col, KEY_9, 0, 1, 0, 0, 5);       //GRAPH
  //if (mapZX[row][col] == KEY_F2) pulsafn(row, col, KEY_1, 0, 1, 0, 0, 5);        //EDIT    
  //if (mapZX[row][col] == KEY_ESCAPE) pulsafn(row, col, KEY_SPACE, 0, 1, 0, 0, 5);//Break      
  if (mapZX[row][col] == KEY_UP) pulsafn(row, col, KEY_PGUP, 1, 0, 0, 0, 5);         //UP
  if (mapZX[row][col] == KEY_DOWN) pulsafn(row, col, KEY_PGDW, 1, 0, 0, 0, 5);       //DOWN
  if (mapZX[row][col] == KEY_LEFT) pulsafn(row, col, KEY_HOME, 1, 0, 0, 0, 5);       //LEFT
  if (mapZX[row][col] == KEY_RIGHT) pulsafn(row, col, KEY_END, 1, 0, 0, 0, 5);       //RIGHT
            
  if (mapZX[row][col] == KEY_CAPS) pulsafn(row, col, KEY_NUMLCK, 0, 0, 0, 0, 5);     //CAPS LOCK
  //if (mapZX[row][col] == SYMBOL_SHIFT) pulsafn(row, col, KEY_RSHIFT, 0, 0, 0, 0, 5);     //SS
  if ((row == SYMBOL_SHIFT_ROW) && (col == SYMBOL_SHIFT_COL)) pulsafn(row, col, KEY_RSHIFT, 0, 0, 0, 0, 5);       //RIGHT CAPS SHIFT

  if (mapZX[row][col] == KEY_U) {kbescucha = 0; cambiomodo = 1; fnpulsada = 1; soltarteclas = 1; fnpulsando = 0; keyxfnpulsada = 0; sendStopUSB(KEY_U);matriz[Y_P_ROW][U_COL] = 0x04;_delay_ms(500); return;}
  
  if (mapZX[row][col] == KEY_V) imprimeversion();
  if (mapZX[row][col] == KEY_X) eepromsave();

  return;
}

void sueltateclaconfn(unsigned char row, unsigned char col, KBMODE modokb)
{
  //if (modo == fus) sendStopUSB(mapFUS[row][col]);
  //else 
  sendStopUSB(mapZX[row][col]);
  return;
}

void matrixScan()
{
  uint8_t r, c;
  uint8_t keyaux = 0;
  fnpulsada = 0; //Se pone a 0 la pulsacion de una tecla de funcion

  rescan:
  //Escaneo de la matriz del teclado
  if (!fnpulsada) for (r = 0; r<ROWS; r++)
  {
    //activar row/fila
    pinSet(pinsR[r], bcdR[r], _OUT);
    pinPut(pinsR[r], bcdR[r], LO);
    _delay_us(5);
    for (c = 0; c<COLS; c++)
    {
      if (pinStat(pinsC[c], bcdC[c]))
      {
        _delay_us(10); //debounce
        if (pinStat(pinsC[c], bcdC[c]))
        {
          if (matriz[r][c] & 0x01)
          {
            matriz[r][c] |= 0x02; //Marcado como mantenido "0x02"
          }
          else
          {
            matriz[r][c] |= 0x01; // Marcado como pulsado "0x01"
          }
        }
        else if (matriz[r][c] & 0x01)
        { 
          matriz[r][c] &= ~0x02; // Ya no esta mantenida "~0x02"
          matriz[r][c] &= ~0x01; // Ya no esta pulsada "~0x01"
          matriz[r][c] |= 0x04; // Marcado para soltar la tecla "0x04" (si entra por debounce)
        } 
      }
      else if (matriz[r][c] & 0x01)
      { 
        matriz[r][c] &= ~0x02; // Ya no esta mantenida "~0x02"
        matriz[r][c] &= ~0x01; // Ya no esta pulsada "~0x01"
        matriz[r][c] |= 0x04; // Marcado para soltar la tecla "0x04"
      }
    }//Fin de Escaneo de las Columnas para el Row/Fila indicado
    //desact. row/fila
    pinSet(pinsR[r], bcdR[r], _IN);
  } //fin escaneo de Rows/Filas de la matriz del teclado  
  if (soltarteclas)
  {
    soltarteclas = 0;
    for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++) if (matriz[r][c] & 0x01) soltarteclas = 1;
    goto rescan;
  }
  if (cambiomodo)
  { 
    if (matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x04) { matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0; if (!modo) { sendPS2(0xF0); sendPS2(CAPS_SHIFT); sendStopUSB(CAPS_SHIFT);} }
    if (matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x04) { matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0; if (!modo) { sendPS2(0xF0); sendPS2(SYMBOL_SHIFT); sendStopUSB(SYMBOL_SHIFT);} }
    //sendStopUSB(KEY_U);
    if (matriz[FN_ROW][FN_COL] & 0x04) { matriz[FN_ROW][FN_COL] = 0; /*if (!modo) { sendPS2(0xF0); sendPS2(CAPS_SHIFT);*/ sendStopUSB(KEY_X_FN);/*}*/ }
    //if (matriz[Y_P_ROW][U_COL] & 0x04) { matriz[Y_P_ROW][U_COL] = 0; /*if (!modo) {*/ sendPS2(0xF0); sendPS2(SYMBOL_SHIFT); sendStopUSB(KEY_U);/*}*/ }
    

    //for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++) if (matriz[r][c] & 0x01) modo = cambiarmodo( ((KBMODE)(mapMODO[r][c])) );
    for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++) {if(/*mapZX[r][c]==KEY_F || */mapZX[r][c]==KEY_X_FN) {continue;}; if (matriz[r][c] & 0x01) {modo = cambiarmodo( ((KBMODE)(mapMODO[r][c])) );}}
    fnpulsada = 1; //Si no se pulsa ninguna tecla sigue en bucle hasta que se pulse
  }
  //Comprobacion de Teclas especiales al tener pulsado Caps Shift y Symbol Shift
  if (!fnpulsada && (matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) && (matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01))
  {
    if (!fnpulsando)
    {//row, col, key, key_E0, shift, ctrl, alt, museg
      if (modo == pc || modo == pcxt || modo == kbext)
      {
        if (matriz[Z_V_ROW][C_COL] & 0x01) opqa_cursors = !opqa_cursors; // OPQA -> Cursores (Activacion / Desactivacion)
        if (matriz[H_L_ROW][L_COL] & 0x01) kbalt = 1; // Modo Alt activado para F1 - F12
        _delay_ms(200);
      }
      /*if (matriz[Y_P_ROW][U_COL] & 0x01)
      {
        kbescucha = 0;
        cambiomodo = 1; fnpulsada = 1; soltarteclas = 1;        
      }*/
      if (codeset == 2)
      {
        //Activa el cambio de modo lo que dejara en bucle hasta que se pulse una tecla. El led se enciende.
        if (matriz[N1_N5_ROW][N1_COL] & 0x01) pulsafn(N1_N5_ROW, N1_COL, KEY_F1, 0, 0, 0, 0, 5);  //F1
        if (matriz[N1_N5_ROW][N2_COL] & 0x01) pulsafn(N1_N5_ROW, N2_COL, KEY_F2, 0, 0, 0, 0, 5);  //F2
        if (matriz[N1_N5_ROW][N3_COL] & 0x01) pulsafn(N1_N5_ROW, N3_COL, KEY_F3, 0, 0, 0, 0, 5);  //F3
        if (matriz[N1_N5_ROW][N4_COL] & 0x01) pulsafn(N1_N5_ROW, N4_COL, KEY_F4, 0, 0, 0, 0, 5);  //F4
        if (matriz[N1_N5_ROW][N5_COL] & 0x01) pulsafn(N1_N5_ROW, N5_COL, KEY_F5, 0, 0, 0, 0, 5);  //F5
        if (matriz[N6_N0_ROW][N6_COL] & 0x01) pulsafn(N6_N0_ROW, N6_COL, KEY_F6, 0, 0, 0, 0, 5);  //F6 
        if (matriz[N6_N0_ROW][N7_COL] & 0x01) pulsafn(N6_N0_ROW, N7_COL, KEY_F7, 0, 0, 0, 0, 5);  //F7
        if (matriz[N6_N0_ROW][N8_COL] & 0x01) pulsafn(N6_N0_ROW, N8_COL, KEY_F8, 0, 0, 0, 0, 5);  //F8
        if (matriz[N6_N0_ROW][N9_COL] & 0x01) pulsafn(N6_N0_ROW, N9_COL, KEY_F9, 0, 0, 0, 0, 5);  //F9
        if (matriz[N6_N0_ROW][N0_COL] & 0x01) pulsafn(N6_N0_ROW, N0_COL, KEY_F10, 0, 0, 0, 0, 5); //F10
        if (matriz[Q_T_ROW][Q_COL] & 0x01) pulsafn(Q_T_ROW, Q_COL, KEY_F11, 0, 0, 0, 0, 50); //F11  
        if (matriz[Q_T_ROW][W_COL] & 0x01) pulsafn(Q_T_ROW, W_COL, KEY_F12, 0, 0, 0, 0, 50); //F12  

        if (matriz[Y_P_ROW][U_COL] & 0x01) {kbescucha = 0; cambiomodo = 1; fnpulsada = 1; soltarteclas = 1;sendStopUSB(KEY_U);matriz[Y_P_ROW][U_COL] = 0x04;_delay_ms(500);}  

        if (matriz[Z_V_ROW][X_COL] & 0x01) eepromsave();
        
        if ((matriz[A_G_ROW][S_COL] & 0x01) && modo)
        {
          if (modo == at8) //F8 + F10 para Atari
          {
            sendPS2(KEY_F8);
            sendUSB(KEY_F8);
            _delay_ms(50);
            sendPS2(KEY_F10);
            sendUSB(KEY_F10);
            _delay_ms(50);
            sendPS2(0xF0);
            sendPS2(KEY_F10);
            sendStopUSB(KEY_F10);
            _delay_ms(1000);
            sendPS2(0xF0);
            sendPS2(KEY_F8);
            sendStopUSB(KEY_F8);
            _delay_ms(50);
            fnpulsada = 1;
            fnpulsando = 1;
          }
          if (modo == c64) //Ctrl + F12 para C64
          {
            pulsafn(A_G_ROW, S_COL, KEY_F12, 0, 0, 1, 0, 5);
          }
        }
        //if (matriz[B_M_ROW][B_COL] & 0x01) pulsafn(B_M_ROW, B_COL, KEY_BACKSP, 0, 0, 1, 1, 5);    //ZXUNO Hard Reset (Control+Alt+Backsp)
        //if (matriz[B_M_ROW][N_COL] & 0x01) pulsafn(B_M_ROW, N_COL, KEY_DELETE, 1, 0, 1, 1, 5);    //ZXUNO Soft Reset (Control+Alt+Supr)
        //if (matriz[A_G_ROW][G_COL] & 0x01) pulsafn(A_G_ROW, G_COL, KEY_SCRLCK, 0, 0, 0, 0, 5);    //ZXUNO RGB/VGA Swich (Bloq Despl)
        //if (matriz[Z_V_ROW][V_COL] & 0x01) imprimeversion();
        //if (matriz[Z_V_ROW][X_COL] & 0x01) eepromsave();                      //Guarda en la EEPROM el modo actual de teclado
        if ((matriz[Q_T_ROW][E_COL] & 0x01) && modo) pulsafn(Q_T_ROW, E_COL, KEY_PGUP, 1, 0, 0, 0, 5); //Re Pag / Pg Up   (Acorn: VGA) (C64 Disco Anterior)
        if ((matriz[Q_T_ROW][R_COL] & 0x01) && modo) pulsafn(Q_T_ROW, R_COL, KEY_PGDW, 1, 0, 0, 0, 5); //Av Pag / Pg Down (Acorn: RGB) (C64 Disco Siguiente)
        if ((matriz[A_G_ROW][D_COL] & 0x01) && modo) pulsafn(A_G_ROW, D_COL, KEY_PGUP, 1, 1, 0, 0, 5); //Re Pag / Pg Up   + Shift (C64 10 Discos Anteriores)
        if ((matriz[A_G_ROW][F_COL] & 0x01) && modo) pulsafn(A_G_ROW, F_COL, KEY_PGDW, 1, 1, 0, 0, 5); //Av Pag / Pg Down + Shift (C64 10 Discos Siguientes)
        if ((matriz[H_L_ROW][H_COL] & 0x01) && modo) pulsafn(H_L_ROW, H_COL, KEYPAD_MENOS, 0, 0, 0, 0, 5); // Keypad - (C64, C16, Atari 800XL, BBC Micro y Apple II: Scanlines)
        if ((matriz[B_M_ROW][M_COL] & 0x01) && modo) pulsafn(B_M_ROW, M_COL, KEYPAD_ASTERISK, 0, 0, 0, 0, 5); //Keypad * (Atari 800XL: PAL/NTSC)
      }
      else if (codeset == 1)
      {
        /*if (matriz[N1_N5_ROW][N1_COL] & 0x01) pulsafn(N1_N5_ROW, N1_COL, KS1_F1, 0, 0, 0, kbalt, 5);  //F1
        if (matriz[N1_N5_ROW][N2_COL] & 0x01) pulsafn(N1_N5_ROW, N2_COL, KS1_F2, 0, 0, 0, kbalt, 5);  //F2
        if (matriz[N1_N5_ROW][N3_COL] & 0x01) pulsafn(N1_N5_ROW, N3_COL, KS1_F3, 0, 0, 0, kbalt, 5);  //F3
        if (matriz[N1_N5_ROW][N4_COL] & 0x01) pulsafn(N1_N5_ROW, N4_COL, KS1_F4, 0, 0, 0, kbalt, 5);  //F4
        if (matriz[N1_N5_ROW][N5_COL] & 0x01) pulsafn(N1_N5_ROW, N5_COL, KS1_F5, 0, 0, 0, kbalt, 5);  //F5
        if (matriz[N6_N0_ROW][N6_COL] & 0x01) pulsafn(N6_N0_ROW, N6_COL, KS1_F6, 0, 0, 0, kbalt, 5);  //F6 
        if (matriz[N6_N0_ROW][N7_COL] & 0x01) pulsafn(N6_N0_ROW, N7_COL, KS1_F7, 0, 0, 0, kbalt, 5);  //F7
        if (matriz[N6_N0_ROW][N8_COL] & 0x01) pulsafn(N6_N0_ROW, N8_COL, KS1_F8, 0, 0, 0, kbalt, 5);  //F8
        if (matriz[N6_N0_ROW][N9_COL] & 0x01) pulsafn(N6_N0_ROW, N9_COL, KS1_F9, 0, 0, 0, kbalt, 5);  //F9
        if (matriz[N6_N0_ROW][N0_COL] & 0x01) pulsafn(N6_N0_ROW, N0_COL, KS1_F10, 0, 0, 0, kbalt, 5); //F10
        if (matriz[Q_T_ROW][Q_COL] & 0x01) pulsafn(Q_T_ROW, Q_COL, KS1_F11, 0, 0, 0, kbalt, 5);     //F11  
        if (matriz[Q_T_ROW][W_COL] & 0x01) pulsafn(Q_T_ROW, W_COL, KS1_F12, 0, 0, 0, kbalt, 5);     //F12  
        if (matriz[B_M_ROW][B_COL] & 0x01) pulsafn(B_M_ROW, B_COL, KS1_BACKSP, 0, 0, 1, 1, 5);    //ZXUNO Hard Reset (Control+Alt+Backsp)
        if (matriz[B_M_ROW][N_COL] & 0x01) pulsafn(B_M_ROW, N_COL, KS1_DELETE, 1, 0, 1, 1, 5);    //ZXUNO Soft Reset (Control+Alt+Supr)*/
      }
    }
    else fnpulsando = 0; //Fin de escaneo de combos
  }
  //Control de teclado
  if (!fnpulsada) //Si no se ha pulsado ninguna tecla de funcion 
  {
    if (modo == zx) // Si el modo es 0 (ZX-Spectrum)
    {
      //Enviar la pulsacion de Caps Shift y/o Symbol Shift si estamos en modo ZX)
      if (!keyxfnpulsada == 1)
      {
        if ((matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) && !(matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x02)) { sendPS2(CAPS_SHIFT); sendUSB(CAPS_SHIFT);   matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] |= 0x02;     espera++; } // Probar a suprimir matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] |= 0x02; (Ya se deja mantenido durante el scan)
        if ((matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01) && !(matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x02)) { sendPS2(SYMBOL_SHIFT); sendUSB(SYMBOL_SHIFT); matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] |= 0x02; espera++; } // Probar a suprimir matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] |= 0x02; (Ya se deja mantenido durante el scan)
        if (matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x04) { sendPS2(0xF0); sendPS2(CAPS_SHIFT);    matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0;  sendStopUSB(CAPS_SHIFT);   espera++; }
        if (matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x04) { sendPS2(0xF0); sendPS2(SYMBOL_SHIFT);  matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0; sendStopUSB(SYMBOL_SHIFT);  espera++; }
      }
      //Enviar el resto de Teclas Pulsadas, nunca entrara por shift o symbol de matrix[r][c] ya que se cambia el valor en el bloque anterior a 3 o 0
      for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++)
      {
        if ((matriz[r][c] & 0x01) && !(matriz[r][c] & 0x02))
        {
          if (mapZX[r][c] == KEY_X_FN)
          {
            keyxfnpulsada = 1;
            matriz[r][c] |= 0x02; espera++; continue;
          }
          if (keyxfnpulsada == 1)
          {
            //if (mapZX[r][c] == KEY_B) { matriz[7][3] |= 0x01; matriz[r][c] = 0; espera++; continue; }

            //pulsafn(unsigned char row, unsigned char col, unsigned char key, unsigned char key_E0, unsigned char shift, unsigned char ctrl, unsigned char alt, unsigned char useg)
            if (mapZX[r][c] == KEY_B) pulsafn(r, c, KEY_BACKSP, 0, 0, 1, 1, 5);     //ZXUNO Hard Reset (Control+Alt+Backsp)
            if (mapZX[r][c] == KEY_N) pulsafn(r, c, KEY_DELETE, 1, 0, 1, 1, 5);     //ZXUNO Soft Reset (Control+Alt+Supr)
            if (mapZX[r][c] == KEY_G) pulsafn(r, c, KEY_SCRLCK, 0, 0, 0, 0, 5);     //ZXUNO RGB/VGA Swich (Bloq Despl)
            if (mapZX[r][c] == KEY_1) pulsafn(r, c, KEY_F1, 0, 0, 0, 0, 5);         //F1
            if (mapZX[r][c] == KEY_2) pulsafn(r, c, KEY_F2, 0, 0, 0, 0, 5);         //F2
            if (mapZX[r][c] == KEY_3) pulsafn(r, c, KEY_F3, 0, 0, 0, 0, 5);         //F3
            if (mapZX[r][c] == KEY_4) pulsafn(r, c, KEY_F4, 0, 0, 0, 0, 5);         //F4
            if (mapZX[r][c] == KEY_5) pulsafn(r, c, KEY_F5, 0, 0, 0, 0, 5);         //F5
            if (mapZX[r][c] == KEY_6) pulsafn(r, c, KEY_F6, 0, 0, 0, 0, 5);         //F6
            if (mapZX[r][c] == KEY_7) pulsafn(r, c, KEY_F7, 0, 0, 0, 0, 5);         //F7
            if (mapZX[r][c] == KEY_8) pulsafn(r, c, KEY_F8, 0, 0, 0, 0, 5);         //F8
            if (mapZX[r][c] == KEY_9) pulsafn(r, c, KEY_F9, 0, 0, 0, 0, 5);         //F9
            if (mapZX[r][c] == KEY_0) pulsafn(r, c, KEY_F10, 0, 0, 0, 0, 5);        //F10
            if (mapZX[r][c] == KEY_Q) pulsafn(r, c, KEY_F11, 0, 0, 0, 0, 5);        //F11
            if (mapZX[r][c] == KEY_W) pulsafn(r, c, KEY_F12, 0, 0, 0, 0, 5);        //F12
            if (mapZX[r][c] == KEY_F10) pulsafn(r, c, KEY_F10, 0, 0, 0, 0, 5);       //GRAPH
            if (mapZX[r][c] == KEY_F2) pulsafn(r, c, KEY_F2, 0, 0, 0, 0, 5);         //EDIT
            if (mapZX[r][c] == KEY_ESCAPE) pulsafn(r, c, KEY_ESCAPE, 0, 0, 0, 0, 5); //BREAK
            if (mapZX[r][c] == KEY_BACKSP) pulsafn(r, c, KEY_DELETE, 1, 0, 0, 0, 5); //DELETE
            if (mapZX[r][c] == KEY_UP) pulsafn(r, c, KEY_PGUP, 1, 0, 0, 0, 5);      //Re Pag - Pg Up
            if (mapZX[r][c] == KEY_DOWN) pulsafn(r, c, KEY_PGDW, 1, 0, 0, 0, 5);    //Av Pag - Pg Down
            if (mapZX[r][c] == KEY_LEFT) pulsafn(r, c, KEY_HOME, 1, 0, 0, 0, 5);    //Inicio - Home
            if (mapZX[r][c] == KEY_RIGHT) pulsafn(r, c, KEY_END, 1, 0, 0, 0, 5);    //Fin - End
            if (mapZX[r][c] == KEY_CAPS) pulsafn(r, c, KEY_NUMLCK, 0, 0, 0, 0, 5);  //Bloq Num - Num Lock
 
			      //if (mapZX[r][c] == SYMBOL_SHIFT) pulsafn(r, c, KEY_RSHIFT, 0, 0, 0, 0, 5);     //SS
            if ((r == SYMBOL_SHIFT_ROW) && (c == SYMBOL_SHIFT_COL)) pulsafn(r, c, KEY_RSHIFT, 0, 0, 0, 0, 5);    //Shift Derecho - Right Caps Shift
			      //if (mapZX[r][c] == KEY_U) {kbescucha = 0; cambiomodo = 1; fnpulsada = 1; soltarteclas = 1;sendStopUSB(KEY_U);matriz[Y_P_ROW][U_COL] = 0x04;_delay_ms(500);}  
            if (mapZX[r][c] == KEY_U) {kbescucha = 0; cambiomodo = 1; fnpulsada = 1; soltarteclas = 1; fnpulsando = 0; keyxfnpulsada = 0; sendStopUSB(KEY_U);matriz[Y_P_ROW][U_COL] = 0x04;_delay_ms(500); continue;}
            if (mapZX[r][c] == KEY_V) imprimeversion();
            if (mapZX[r][c] == KEY_X) eepromsave();

            fnpulsada = 0;
            fnpulsando = 0;
            kbalt = 0;
            soltarteclas = 0;

            matriz[r][c] = 0; espera++; continue;
          } else {
		        if (mapZX[r][c] == KEY_F10) { pulsafn(r, c, KEY_9, 0, 1, 0, 0, 5); soltarteclas = 0; matriz[r][c] = 0; espera++; continue; }        //GRAPH
            if (mapZX[r][c] == KEY_F2) { pulsafn(r, c, KEY_1, 0, 1, 0, 0, 5); soltarteclas = 0; matriz[r][c] = 0; espera++; continue; }         //EDIT
            if (mapZX[r][c] == KEY_ESCAPE) { pulsafn(r, c, KEY_SPACE, 0, 1, 0, 0, 5); soltarteclas = 0; matriz[r][c] = 0; espera++; continue; } //BREAK
          }

          sendUSB(mapZX[r][c]);
          if (mapZX[r][c] == KEY_UP || mapZX[r][c] == KEY_DOWN || mapZX[r][c] == KEY_LEFT || mapZX[r][c] == KEY_RIGHT || mapZX[r][c] == KEY_RCTRL || mapZX[r][c] == KEY_RALT || mapZX[r][c] == KEY_LWIN || mapZX[r][c] == KEY_RWIN || mapZX[r][c] == KEY_APPS || mapZX[r][c] == KEY_PGUP || mapZX[r][c] == KEY_PGDW) sendPS2(0xE0);
          sendPS2(mapZX[r][c]); continue;
        }
        if (matriz[r][c] & 0x04)
        {
          if (mapZX[r][c] == KEY_X_FN)
          {
            keyxfnpulsada = 0;
            matriz[r][c] = 0; espera++; continue;
          }
          sendStopUSB(mapZX[r][c]);
          if (mapZX[r][c] == KEY_UP || mapZX[r][c] == KEY_DOWN || mapZX[r][c] == KEY_LEFT || mapZX[r][c] == KEY_RIGHT || mapZX[r][c] == KEY_RCTRL || mapZX[r][c] == KEY_RALT || mapZX[r][c] == KEY_LWIN || mapZX[r][c] == KEY_RWIN || mapZX[r][c] == KEY_APPS || mapZX[r][c] == KEY_PGUP || mapZX[r][c] == KEY_PGDW) sendPS2(0xE0);
          sendPS2(0xF0); sendPS2(mapZX[r][c]); matriz[r][c] = 0;
        }
      }
      if (espera) { _delay_us(5); espera = 0; }
    }
    else if (modo == fus)
    {
      //Enviar la pulsacion de Caps Shift y/o Symbol Shift si estamos en modo ZX)
      if (!keyxfnpulsada == 1)
      {
        if ((matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) && !(matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x02)) { sendPS2(CAPS_SHIFT); sendUSB(CAPS_SHIFT);   matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] |= 0x02;     espera++; } // Probar a suprimir matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] |= 0x02; (Ya se deja mantenido durante el scan)
        if ((matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01) && !(matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x02)) { sendPS2(SYMBOL_SHIFT); sendUSB(SYMBOL_SHIFT); matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] |= 0x02; espera++; } // Probar a suprimir matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] |= 0x02; (Ya se deja mantenido durante el scan)
        if (matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x04) { sendPS2(0xF0); sendPS2(CAPS_SHIFT);    matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] = 0;  sendStopUSB(CAPS_SHIFT);   espera++; }
        if (matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x04) { sendPS2(0xF0); sendPS2(SYMBOL_SHIFT);  matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] = 0; sendStopUSB(SYMBOL_SHIFT);  espera++; }
      }
      //Enviar el resto de Teclas Pulsadas, nunca entrara por shift o symbol de matrix[r][c] ya que se cambia el valor en el bloque anterior a 3 o 0
      for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++)
      {
        if ((matriz[r][c] & 0x01) && !(matriz[r][c] & 0x02))
        {
          if (mapFUS[r][c] == KEY_X_FN)
          {
            keyxfnpulsada = 1;
            matriz[r][c] |= 0x02; espera++; continue;
          }
          if (keyxfnpulsada == 1)
          {
            //if (mapFUS[r][c] == KEY_B) { matriz[7][3] |= 0x01; matriz[r][c] = 0; espera++; continue; }

            //pulsafn(unsigned char row, unsigned char col, unsigned char key, unsigned char key_E0, unsigned char shift, unsigned char ctrl, unsigned char alt, unsigned char useg)
            if (mapFUS[r][c] == KEY_D) { pulsafn(r, c, KEY_BACKSP, 0, 0, 1, 1, 5); _delay_ms(1000); pulsafn(r, c, KEY_Z, 0, 0, 1, 0, 5); }
            if (mapFUS[r][c] == KEY_B) pulsafn(r, c, KEY_BACKSP, 0, 0, 1, 1, 5);     //ZXUNO Hard Reset (Control+Alt+Backsp)
            if (mapFUS[r][c] == KEY_N) pulsafn(r, c, KEY_DELETE, 1, 0, 1, 1, 5);     //ZXUNO Soft Reset (Control+Alt+Supr)
            if (mapFUS[r][c] == KEY_G) pulsafn(r, c, KEY_SCRLCK, 0, 0, 0, 0, 5);     //ZXUNO RGB/VGA Swich (Bloq Despl)
            if (mapFUS[r][c] == KEY_1) pulsafn(r, c, KEY_F1, 0, 0, 0, 0, 5);         //F1
            if (mapFUS[r][c] == KEY_2) pulsafn(r, c, KEY_F2, 0, 0, 0, 0, 5);         //F2
            if (mapFUS[r][c] == KEY_3) pulsafn(r, c, KEY_F3, 0, 0, 0, 0, 5);         //F3
            if (mapFUS[r][c] == KEY_4) pulsafn(r, c, KEY_F4, 0, 0, 0, 0, 5);         //F4
            if (mapFUS[r][c] == KEY_5) pulsafn(r, c, KEY_F5, 0, 0, 0, 0, 5);         //F5
            if (mapFUS[r][c] == KEY_6) pulsafn(r, c, KEY_F6, 0, 0, 0, 0, 5);         //F6
            if (mapFUS[r][c] == KEY_7) pulsafn(r, c, KEY_F7, 0, 0, 0, 0, 5);         //F7
            if (mapFUS[r][c] == KEY_8) pulsafn(r, c, KEY_F8, 0, 0, 0, 0, 5);         //F8
            if (mapFUS[r][c] == KEY_9) pulsafn(r, c, KEY_F9, 0, 0, 0, 0, 5);         //F9
            if (mapFUS[r][c] == KEY_0) pulsafn(r, c, KEY_F10, 0, 0, 0, 0, 5);        //F10
            if (mapFUS[r][c] == KEY_Q) pulsafn(r, c, KEY_F11, 0, 0, 0, 0, 5);        //F11
            if (mapFUS[r][c] == KEY_W) pulsafn(r, c, KEY_F12, 0, 0, 0, 0, 5);        //F12
            if (mapFUS[r][c] == KEY_BACKSP) pulsafn(r, c, KEY_DELETE, 1, 0, 0, 0, 5);//DELETE
            if (mapFUS[r][c] == KEY_F10) pulsafn(r, c, KEY_9, 0, 1, 0, 0, 5);        //GRAPH
            if (mapFUS[r][c] == KEY_F2) pulsafn(r, c, KEY_1, 0, 1, 0, 0, 5);         //EDIT
            if (mapFUS[r][c] == KEY_ESCAPE) pulsafn(r, c, KEY_SPACE, 0, 1, 0, 0, 5); //BREAK
            if (mapFUS[r][c] == KEY_UP) pulsafn(r, c, KEY_PGUP, 1, 0, 0, 0, 5);      //Re Pag - Pg Up
            if (mapFUS[r][c] == KEY_DOWN) pulsafn(r, c, KEY_PGDW, 1, 0, 0, 0, 5);    //Av Pag - Pg Down
            if (mapFUS[r][c] == KEY_LEFT) pulsafn(r, c, KEY_HOME, 1, 0, 0, 0, 5);    //Inicio - Home
            if (mapFUS[r][c] == KEY_RIGHT) pulsafn(r, c, KEY_END, 1, 0, 0, 0, 5);    //Fin - End
            if (mapFUS[r][c] == KEY_CAPS) pulsafn(r, c, KEY_NUMLCK, 0, 0, 0, 0, 5);  //Bloq Num - Num Lock
            if ((r == SYMBOL_SHIFT_ROW) && (c == SYMBOL_SHIFT_COL)) pulsafn(r, c, KEY_RSHIFT, 0, 0, 0, 0, 5);    //Shift Derecho - Right Caps Shift
			      if (mapFUS[r][c] == KEY_U) {kbescucha = 0; cambiomodo = 1; fnpulsada = 1; soltarteclas = 1; fnpulsando = 0; keyxfnpulsada = 0; sendStopUSB(KEY_U); matriz[Y_P_ROW][U_COL] = 0x04; _delay_ms(500); continue;}
            if (mapFUS[r][c] == KEY_V) imprimeversion();
            if (mapFUS[r][c] == KEY_X) eepromsave();

            fnpulsada = 0;
            fnpulsando = 0;
            kbalt = 0;
            soltarteclas = 0;

            matriz[r][c] = 0; espera++; continue;
          }
          sendUSB(mapFUS[r][c]);
          if (mapFUS[r][c] == KEY_UP || mapFUS[r][c] == KEY_DOWN || mapFUS[r][c] == KEY_LEFT || mapFUS[r][c] == KEY_RIGHT || mapFUS[r][c] == KEY_RCTRL || mapFUS[r][c] == KEY_RALT || mapFUS[r][c] == KEY_LWIN || mapFUS[r][c] == KEY_RWIN || mapFUS[r][c] == KEY_APPS || mapFUS[r][c] == KEY_PGUP || mapFUS[r][c] == KEY_PGDW) sendPS2(0xE0);
          sendPS2(mapFUS[r][c]); continue;
        }
        if (matriz[r][c] & 0x04)
        {
          if (mapFUS[r][c] == KEY_X_FN)
          {
            keyxfnpulsada = 0;
            matriz[r][c] = 0; espera++; continue;
          }
          sendStopUSB(mapFUS[r][c]);
          if (mapFUS[r][c] == KEY_UP || mapFUS[r][c] == KEY_DOWN || mapFUS[r][c] == KEY_LEFT || mapFUS[r][c] == KEY_RIGHT || mapFUS[r][c] == KEY_RCTRL || mapFUS[r][c] == KEY_RALT || mapFUS[r][c] == KEY_LWIN || mapFUS[r][c] == KEY_RWIN || mapFUS[r][c] == KEY_APPS || mapFUS[r][c] == KEY_PGUP || mapFUS[r][c] == KEY_PGDW) sendPS2(0xE0);
          sendPS2(0xF0); sendPS2(mapFUS[r][c]); matriz[r][c] = 0;
        }
      }
      if (espera) { _delay_us(5); espera = 0; }
    }
    else // Manejo de los otros modos de Keymap
    {
      if (!antighosting && cs_counter == 0 && ss_counter == 0)
      {
        if (((matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) && !(matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x02)) ||
          ((matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01) && !(matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x02)))
        {
          for (r = 0; r < ROWS_T; r++) for (c = 0; c < COLS_T; c++)
          {
            // Ignoramos teclas capssymbol y symbolshift en el recorrido
            if (r == CAPS_SHIFT_ROW && c == CAPS_SHIFT_COL) continue;
            if (r == SYMBOL_SHIFT_ROW && c == SYMBOL_SHIFT_COL) continue;
            if (matriz[r][c] & 0x01) { antighosting = 1; break; }
          }
        }
      }
      else
      {
        if (!(matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) && !(matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01))
            antighosting = 0;
      }
      for (r = 0; r<ROWS_T; r++) for (c = 0; c<COLS_T; c++)
      {
        // Ignoramos teclas capssymbol y symbolshift en el recorrido
        if (r == CAPS_SHIFT_ROW && c == CAPS_SHIFT_COL) continue;
        if (r == SYMBOL_SHIFT_ROW && c == SYMBOL_SHIFT_COL) continue;
        if (r == FN_ROW && c == FN_COL) continue;
        
        
        if ((matriz[r][c] & 0x01) && !(matriz[r][c] & 0x02)) // Gestion de pulsado
        {
            if (!antighosting)
            {
                if (matriz[CAPS_SHIFT_ROW][CAPS_SHIFT_COL] & 0x01) matriz[r][c] |= 0x08; // Se marca capsshift
                if (matriz[SYMBOL_SHIFT_ROW][SYMBOL_SHIFT_COL] & 0x01) matriz[r][c] |= 0x10; // Se marca symbolshift    
                if (matriz[FN_ROW][FN_COL] & 0x01) matriz[r][c] |= 0x20; // Se marca fn
            }
          if ((matriz[r][c] & 0x08) && !((matriz[r][c] & 0x10) || (matriz[r][c] & 0x20))) pulsateclaconshift(r, c, codeset == 2 ? mapEXT[r][c] : mapEXT1[r][c]); // Pulsar con capsshift
          else if ((matriz[r][c] & 0x10) && !((matriz[r][c] & 0x08) || (matriz[r][c] & 0x20))) pulsateclaconsymbol(r, c, modo); // Pulsar con symbolshift
          else if ((matriz[r][c] & 0x20) && !((matriz[r][c] & 0x08) || (matriz[r][c] & 0x10))) {pulsateclaconfn(r, c, modo);} // Pulsar con fn
          else if (!(matriz[r][c] & 0x08) && !(matriz[r][c] & 0x10) && !(matriz[r][c] & 0x20))
          {
            if (opqa_cursors)
            {
              typematicfirst = 0;
              typematic_codeaux = 0;
              if (codeset == 2)
              {
                keyaux = mapZX[r][c];
                keyaux = keyaux == KEY_O ? KEY_LEFT : keyaux;
                keyaux = keyaux == KEY_P ? KEY_RIGHT : keyaux;
                keyaux = keyaux == KEY_Q ? KEY_UP : keyaux;
                keyaux = keyaux == KEY_A ? KEY_DOWN : keyaux;
                if (keyaux == KEY_LEFT || keyaux == KEY_RIGHT || keyaux == KEY_UP || keyaux == KEY_DOWN) typematic_codeaux = 0xE0; //Es una tecla del codeset2 que necesita E0
              }
              if (codeset == 1)
              {
                keyaux = mapSET1[r][c];
                keyaux = keyaux == KS1_O ? KS1_LEFT : keyaux;
                keyaux = keyaux == KS1_P ? KS1_RIGHT : keyaux;
                keyaux = keyaux == KS1_Q ? KS1_UP : keyaux;
                keyaux = keyaux == KS1_A ? KS1_DOWN : keyaux;
                if (keyaux == KS1_LEFT || keyaux == KS1_RIGHT || keyaux == KS1_UP || keyaux == KS1_DOWN) typematic_codeaux = 0xE0; //Es una tecla del codeset1 que necesita E0
              }
              typematic_code = keyaux;
              if (typematic_codeaux > 0) sendPS2(0xE0);
              sendPS2(typematic_code);
              
              sendUSB(typematic_code);
            }
            else
            {
              typematicfirst = 0;
              if (codeset == 2) typematic_code = mapZX[r][c];
              else typematic_code = mapSET1[r][c]; // Pulsar sin modificadores

              typematic_codeaux = 0;
              sendPS2(typematic_code);
              
              sendUSB(typematic_code);
            }
          }
          // Si estan pulsados capsshift y symbolshift, no hacemos nada
          matriz[r][c] |= 0x02; // Se marca mantenida
        }
        if ((matriz[r][c] & 0x04)) // Gestion de liberado
        {
          //if ((matriz[r][c] & 0x08) && !(matriz[r][c] & 0x10)) sueltateclaconshift(r, c, codeset == 2 ? mapEXT[r][c] : mapEXT1[r][c]); // Liberar con capsshift         
          //if ((matriz[r][c] & 0x10) && !(matriz[r][c] & 0x08)) sueltateclaconsymbol(r, c, modo); // Liberar con symbolshift         
          if ((matriz[r][c] & 0x08) && !((matriz[r][c] & 0x10) || (matriz[r][c] & 0x20))) sueltateclaconshift(r, c, codeset == 2 ? mapEXT[r][c] : mapEXT1[r][c]); // Pulsar con capsshift
          if ((matriz[r][c] & 0x10) && !((matriz[r][c] & 0x08) || (matriz[r][c] & 0x20))) sueltateclaconsymbol(r, c, modo); // Pulsar con symbolshift
          if ((matriz[r][c] & 0x20) && !((matriz[r][c] & 0x08) || (matriz[r][c] & 0x10))) sueltateclaconfn(r, c, modo); // Pulsar con fn
          
          if (!(matriz[r][c] & 0x08) && !(matriz[r][c] & 0x10))
          {
            if (opqa_cursors)
            {
              typematic_code = 0;

              if (codeset == 2)
              {
                keyaux = mapZX[r][c];
                keyaux = keyaux == KEY_O ? KEY_LEFT : keyaux;
                keyaux = keyaux == KEY_P ? KEY_RIGHT : keyaux;
                keyaux = keyaux == KEY_Q ? KEY_UP : keyaux;
                keyaux = keyaux == KEY_A ? KEY_DOWN : keyaux;

                if (keyaux == KEY_LEFT || keyaux == KEY_RIGHT || keyaux == KEY_UP || keyaux == KEY_DOWN) sendPS2(0xE0); //Es una tecla del codeset2 que necesita E0
                sendPS2(0xF0); sendPS2(keyaux);
                sendStopUSB(keyaux);
              }
              if (codeset == 1)
              {
                keyaux = mapSET1[r][c];
                keyaux = keyaux == KS1_O ? KS1_LEFT : keyaux;
                keyaux = keyaux == KS1_P ? KS1_RIGHT : keyaux;
                keyaux = keyaux == KS1_Q ? KS1_UP : keyaux;
                keyaux = keyaux == KS1_A ? KS1_DOWN : keyaux;
                if (keyaux == KS1_LEFT || keyaux == KS1_RIGHT || keyaux == KS1_UP || keyaux == KS1_DOWN) sendPS2(0xE0);//Es una tecla del codeset1 que necesita E0
                sendPS2(keyaux + KS1_RELEASE);
                
                sendStopUSB(mapZX[r][c]);
              }
            }
            else
            {
              if (codeset == 2) { sendPS2(0xF0); sendPS2(mapZX[r][c]); typematic_code = typematic_code == mapZX[r][c] ? 0 : typematic_code; }
              else { sendPS2(mapSET1[r][c] + KS1_RELEASE); typematic_code = typematic_code == mapSET1[r][c] ? 0 : typematic_code; } // Liberar sin modificadores
              
              sendStopUSB(mapZX[r][c]);
            }
          }
          matriz[r][c] = 0; // Fin de gestion de la tecla
        }
      }
    }//Fin del If/else manejo de modo ZX u otros Keymaps*/
  }//Fin del If del control del teclado.
}//FIN de Matrixscan

void setup()
{
  DDRD=0; // Help to disable UART to use pins PD3 and PD2
  CPU_PRESCALE(CPU_16MHz);
  ps2Init();
  matrixInit();
  const uint8_t ZXUNO_SIGNATURE[] = { 'Z','X','U','N','O' };
  uint8_t checksignature[5];
  uint8_t issigned = 1;
  eeprom_read_block((void*)&checksignature, (const void*)0, 5);
  
  for (int n = 0; n < 5; n++) if (checksignature[n] != ZXUNO_SIGNATURE[n]) issigned = 0;
  if (issigned)
  {
    modo = cambiarmodo2(((KBMODE)eeprom_read_byte((uint8_t*)5)));
  }
  else
  {
    eeprom_write_block((const void*)&ZXUNO_SIGNATURE, (void*)0, 5); // Guardamos la firma
    eeprom_write_byte((uint8_t*)5, (uint8_t)0); // Guardamos modo ZX por defecto
    eeprom_write_byte((uint8_t*)6, (uint8_t)1); // Guardamos ZXFULLCOMBOS por defecto
  }
  //modo = pc;

  Keyboard.begin();
  setupKeymaps();
}

void loop()
{

  if ((ps2Stat() && modo == MAXKB && (kbescucha || timeout_escucha > 0))&&!(UDADDR & _BV(ADDEN))) // Lineas CLK y/o DATA a 0 y escucha activa
  {                                                                     // Solo hay escucha activa en modo PC, hasta su inicializacion.
                                                                        // Una vez completada la inicializacion de teclado, no es necesario mantener activa la escucha de comandos excepto si se hace eco
                                                                        //GAP: El ultimo termino de la condicion es para detectar si estamos conectados por USB o por PS/2
    while (checkState(1000)) // tramos de 5 us (5000 us)
    {
      hostdataAnt = hostdata;
      if (getPS2(&hostdata) == 0)
      {
        timeout_escucha = 100000;   // Dejamos tiempo para que se complete la inicializacion
        if (hostdata == 0xEE)
        {
            sendPS2(0xEE); // Echo
            kbescucha = 1; // Si se hace eco, mantenemos la escucha de comandos (necesario en determinados adaptadores PS/2 -> USB)
        }
        else
        {
            sendPS2(0xFA); // Ack
        }
        switch (hostdata)
        {
          case 0x00: // second bit of 0xED or 0xF3 (or get scancode set)  
            if (hostdataAnt == 0xF0)
            {
              sendPS2(codeset);
            }
            break;
          case 0x01: // set scancode 1
            if (hostdataAnt == 0xF0)
            {
              codeset = 1;
            }
            break;
          case 0x02: // set scancode 2
            if (hostdataAnt == 0xF0)
            {
              codeset = 2;
            }
            break;
          case 0xF2: // ID          
            sendPS2(0xAB);
            sendPS2(0x83);
            break;
          case 0xFF:
            // tell host we are ready to connect
            sendPS2(0xAA); 
            kbescucha = 0; // En el inicio iniciamos la cuenta atras de timeout
            break;
          case 0xED: // set/reset LEDs
          case 0xF0: // get/set scancode set
          case 0xF3: // set/reset typematic delay
          case 0xF4: // keyboard is enabled, break loop
          case 0xF5: // keyboard is disabled, break loop            
          default:
            break;
        } //Fin del Swich
      } //Fin del IF de si detecta dato
    } //Fin del while que chequea el estado
  }
  else
  {
    if (timeout_escucha > 0) timeout_escucha--;
    if (modo == MAXKB && typematic_code != 0 && (typematicfirst++ > 1000 || codeset == 2) && typematic++ > 150) // Funcion tipematica simulada para PC
    {
      if (typematic_codeaux != 0) sendPS2(typematic_codeaux);
      sendPS2(typematic_code); typematic = 0;
    }
    matrixScan(); //No llegan datos del Host, enviamos teclado.
  }
}
