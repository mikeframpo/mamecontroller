
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

/* The USB keycodes are enumerated here - the first part is simply
   an enumeration of the allowed scan-codes used for USB HID devices */
typedef enum keycodes {
  KEY__=0,
  KEY_errorRollOver,
  KEY_POSTfail,
  KEY_errorUndefined,
  KEY_A,        // 4
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G, 
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,        // 0x10
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q, 
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_1,
  KEY_2,
  KEY_3,        // 0x20
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,        // 0x27
  KEY_enter,
  KEY_esc,
  KEY_bckspc,   // backspace
  KEY_tab,
  KEY_spc,      // space
  KEY_minus,    // - (and _)
  KEY_equal,    // = (and +)
  KEY_lbr,      // [
  KEY_rbr,      // ]  -- 0x30
  KEY_bckslsh,  // \ (and |)
  KEY_hash,     // Non-US # and ~
  KEY_smcol,    // ; (and :)
  KEY_ping,     // ' and "
  KEY_grave,    // Grave accent and tilde
  KEY_comma,    // , (and <)
  KEY_dot,      // . (and >)
  KEY_slash,    // / (and ?)
  KEY_cpslck,   // capslock
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6, 
  KEY_F7,       // 0x40
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_PrtScr,
  KEY_scrlck,
  KEY_break,
  KEY_ins,
  KEY_home,
  KEY_pgup,
  KEY_del,
  KEY_end,
  KEY_pgdn,
  KEY_rarr, 
  KEY_larr,     // 0x50
  KEY_darr,
  KEY_uarr,
  KEY_numlock,
  KEY_KPslash,
  KEY_KPast,
  KEY_KPminus,
  KEY_KPplus,
  KEY_KPenter,
  KEY_KP1,
  KEY_KP2,
  KEY_KP3,
  KEY_KP4,
  KEY_KP5,
  KEY_KP6,
  KEY_KP7,
  KEY_KP8,      // 0x60
  KEY_KP9,
  KEY_KP0,
  KEY_KPcomma,
  KEY_Euro2,

  /* These are NOT standard USB HID - handled specially in decoding,
     so they will be mapped to the modifier byte in the USB report */
  KEY_Modifiers,
  MOD_LCTRL,    // 0x01
  MOD_LSHIFT,   // 0x02
  MOD_LALT,     // 0x04
  MOD_LGUI,     // 0x08
  MOD_RCTRL,    // 0x10
  MOD_RSHIFT,   // 0x20
  MOD_RALT,     // 0x40
  MOD_RGUI,     // 0x80
} keycode_t;

typedef struct {
    port_t port;
    uint8_t pin;
    bool_t debouncedState; //non-zero indicates that the button is being pressed.
    int8_t cyclesRemaining;
    keycode_t key;
    uint8_t mod_mask;
} button_t;

#endif
