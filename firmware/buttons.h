
#ifndef DEF_BUTTONS_H
#define DEF_BUTTONS_H

#define NUM_BUTTONS 22

#define PRESSED_CYCLES 1
#define RELEASED_CYCLES 2

#include "main.h"

enum port_enum {
	PORT_ENUM_PORTA = 	(1 << 0),
	PORT_ENUM_PORTB	=	(1 << 1),
	PORT_ENUM_PORTC = 	(1 << 2),
	PORT_ENUM_PORTD = 	(1 << 3)
};

typedef struct {
	//enum representing the port that the pin is part of.
	enum port_enum pinPort;

	//mask of this pin.
	uint8_t pinMask;

	//boolean indicating the real button state, true is pressed
	int8_t realState;

	//number of times INSERT_F_NAME_HERE must be called with the same
	//state to register as a legit pin change.
	uint8_t debounceCycles;

	enum keycodes key;

} button_t;

void init_buttons(void);

uint8_t debounce_all_buttons(uint8_t* reportBuffer, int8_t* bufLen);

void clear_queue(void);

#endif
