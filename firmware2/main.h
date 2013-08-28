
#ifndef DEF_MAIN_H
#define DEF_MAIN_H

#include <inttypes.h>

#define TRUE (0 == 0)
#define FALSE (1 == 0)

#define RED_LED (1 << 6) //portD

#define DEPRESSED_CYCLES 20
#define RELEASED_CYCLES 4

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

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
