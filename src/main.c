#include <Arduino.h>
#include <util.h>

//#define espera      0
//#define incremento  1
//#define decremento  2

enum ESTADOS_POSIBLES{espera,incremento,decremento};

int foo;

int main(void)
{
  config_timer0();

  enum ESTADOS_POSIBLES estado=espera;

  int cnt;

  while (1)
  {

  switch(estado){
    case espera://espera
      if(bit_is_clear(PIND,PD7)){
        estado = incremento;
      }

    break;

    case incremento://incrementar
      cnt++;
      //show_num(cnt);
      estado=espera;

    break;
    case decremento:
    break;



  }
    suma(2,4);
    // do nothing!
    asm("nop");
  }
}