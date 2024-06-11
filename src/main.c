#include <Arduino.h>
#include <util.h>

#define MAX_SNAKE_LEN 30

#define MATRIX_X 24
#define MATRIX_Y 8

typedef struct point
{
  int x;
  int y;
} point;

void refresh_matrix(uint8_t* buff);

int main(void)
{

  uint8_t disp_buff[MATRIX_X];

  point apple;
  point snake_head;
  uint8_t snake_len;
  point snake_body[MAX_SNAKE_LEN];

  //creo la primer manzana
  apple.x = rand() % MATRIX_X;
  apple.y = rand() % MATRIX_Y;

  //ubico la vivora en el centro (aprox) de la matriz

  snake_head.x = MATRIX_Y / 2;
  snake_head.y = MATRIX_Y / 2;

  //inicializo cuerpo de la vivora
  snake_len = 0;
  for(uint8_t i = 0 ; i < MAX_SNAKE_LEN ; i++){
    snake_body[i].x=0;
    snake_body[i].y=0;
  }


  //cada loop del while es un frame, es es decir un avance de la vivora.
  while (1)
  {






    // limpio buffer de salida

    memset( disp_buff, 0, sizeof(disp_buff) );
    //for(uint8_t i = 0 ; i < MATRIX_X ; i++){
    //  disp_buff[i] = 0;
    //}


    // convierto coordenadas a matriz ( render(? )


    //prendo led de manzana
    disp_buff[apple.x] |= (1<<apple.y);
    //prendo cabeza de la vivora
    disp_buff[snake_head.x] |= (1<<snake_head.y);
    //prendo cuerpo de la vivora
    for(uint8_t i = 0 ; i < snake_len ; i++){
      disp_buff[snake_body[i].x] |= (1<<snake_body[i].y);
    }


    //refresco matriz de leds
    refresh_matrix( disp_buff );
  }
}




void refresh_matrix(uint8_t* buff){


  //envio buffer a registros de desplazamiento.

  return;
}