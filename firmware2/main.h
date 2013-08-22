
#ifndef DEF_MAIN_H
#define DEF_MAIN_H

#include <inttypes.h>

#define TRUE (0 == 0)
#define FALSE (1 == 0)

#define RED_LED (1 << 6) //portD

//PORTC

//PORTD
#define P1_UP       (1 << 3) //red
#define P1_DOWN     (1 << 1) //black
#define P1_RIGHT    (1 << 4) //orange
#define P1_START    (1 << 0) //white
//PORTC
#define P1_LEFT     (1 << 7) //brown

//PORTA
#define P1_A (1 << 1) //red
#define P1_B (1 << 3) //yellow
#define P1_C (1 << 5) //blue
#define P1_D (1 << 2) //orange
#define P1_E (1 << 4) //green
#define P1_F (1 << 6) //purple

//PORTB
#define P2_UP       (1 << 2) //orange     
#define P2_DOWN     (1 << 0) //brown
#define P2_LEFT     (1 << 1) //red
#define P2_RIGHT    (1 << 3) //black

// PORTA
#define P2_START    (1 << 7) //white

//PORTC
#define P2_A (1 << 2) //blue
#define P2_B (1 << 4) //yellow
#define P2_C (1 << 6) //red
#define P2_D (1 << 1) //purple
#define P2_E (1 << 3) //green
#define P2_F (1 << 5) //orange

#define DEPRESSED_CYCLES 7
#define RELEASED_CYCLES 4

typedef enum {
    PORT_A,
    PORT_B,
    PORT_C,
    PORT_D,
} port_t;

typedef uint8_t bool_t;

typedef enum {
    // lower 8 values correspond to bits in the
    // button-state byte.
    GP_BUT_A = 0,
    GP_BUT_B,
    GP_BUT_C,
    GP_BUT_D,
    GP_BUT_E,
    GP_BUT_F,
    GP_BUT_START,
    GP_BUT_EXTRA,
    // directional buttons are handled separately
    GP_UP,
    GP_DOWN,
    GP_LEFT,
    GP_RIGHT,
} gamepad_button_t;

typedef struct {
    port_t port;
    uint8_t pin;
    bool_t debouncedState;  //non-zero indicates that the button is being pressed.
    int8_t cyclesRemaining;
    gamepad_button_t btype;
} button_t;

typedef struct {
    uint8_t id;
    button_t* buttons;
    uint8_t num_buttons;
} gamepad_t;

#endif
