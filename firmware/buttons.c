
#include <avr/io.h>
#include <string.h>

#include "buttons.h"
#include "main.h"

button_t buttons[NUM_BUTTONS];
uint8_t numButtons;

void add_button(enum port_enum port, uint8_t mask, enum keycodes key) {

	buttons[numButtons].pinPort = port;
	buttons[numButtons].pinMask = mask;
	buttons[numButtons].key = key;
	
	//set pullup
	switch(port) {
		case (PORT_ENUM_PORTA): {
			PORTA |= mask;
			break;
		}
		case (PORT_ENUM_PORTB): {
			PORTB |= mask;
			break;
		}
		case (PORT_ENUM_PORTC): {
			PORTC |= mask;
			break;
		}
		case (PORT_ENUM_PORTD): {
			PORTD |= mask;
			break;
		}
		default: {
			break;
		}
	}
	numButtons++;
}

void init_buttons(void) {

	numButtons = 0;
	memset(buttons, 0, NUM_BUTTONS * sizeof(button_t));

	//left buttons
	add_button(PORT_ENUM_PORTA, (1 << 0), KEY_Z);
	add_button(PORT_ENUM_PORTA, (1 << 1), KEY_X); //green start
	add_button(PORT_ENUM_PORTA, (1 << 2), KEY_C);
	add_button(PORT_ENUM_PORTA, (1 << 3), KEY_V);
	add_button(PORT_ENUM_PORTA, (1 << 4), KEY_B);
	add_button(PORT_ENUM_PORTA, (1 << 5), KEY_N);
	add_button(PORT_ENUM_PORTA, (1 << 6), KEY_M);
	
	//right buttons
	add_button(PORT_ENUM_PORTC, (1 << 1), KEY_0);
	add_button(PORT_ENUM_PORTC, (1 << 2), KEY_KP5);
	add_button(PORT_ENUM_PORTC, (1 << 3), KEY_2); //yellow start
	add_button(PORT_ENUM_PORTC, (1 << 4), KEY_3);
	add_button(PORT_ENUM_PORTC, (1 << 5), KEY_4);
	add_button(PORT_ENUM_PORTC, (1 << 6), KEY_KPplus);
	add_button(PORT_ENUM_PORTC, (1 << 7), KEY_6);
	
	//left joy
	add_button(PORT_ENUM_PORTD, (1 << 0), KEY_W); //up
	add_button(PORT_ENUM_PORTD, (1 << 1), KEY_D); //right
	add_button(PORT_ENUM_PORTD, (1 << 3), KEY_S); //down
	add_button(PORT_ENUM_PORTD, (1 << 4), KEY_A); //left
	
	//right joy
	add_button(PORT_ENUM_PORTB, (1 << 0), KEY_KP6); //right
	add_button(PORT_ENUM_PORTB, (1 << 1), KEY_KP2); //down
	add_button(PORT_ENUM_PORTB, (1 << 2), KEY_KP4); //left
	add_button(PORT_ENUM_PORTB, (1 << 3), KEY_KP8); //up
	
}

//Returns true if the input pin is currently low, meaning the button is pressed.
uint8_t get_pin_state(button_t* button) {
	
	switch(button->pinPort) {
		case (PORT_ENUM_PORTA):
			return (PINA & button->pinMask) == 0;
		case (PORT_ENUM_PORTB):
			return (PINB & button->pinMask) == 0;
		case (PORT_ENUM_PORTC):
			return (PINC & button->pinMask) == 0;
		case (PORT_ENUM_PORTD):
			return (PIND & button->pinMask) == 0;
		default:
			//fail case
			return 0;
	}
}

void add_to_buffer(uint8_t* reportBuffer, int8_t* bufLen, enum keycodes key) {

	if (*bufLen < REPORT_BUF_LEN) {
		reportBuffer[(*bufLen)] = key;
		(*bufLen)++;
	}
}

uint8_t debounce_button(button_t* button) {

	uint8_t rawState = get_pin_state(button);

	if (rawState == button->realState) {
		//hasn't changed, or still bouncing, reset the timer
		if (button->realState) {
			button->debounceCycles = RELEASED_CYCLES;
		} else {
			button->debounceCycles = PRESSED_CYCLES;
		}
	} else {
		button->debounceCycles--;
		if (button->debounceCycles <= 0) {
			button->realState = rawState;
			if (button->realState) {
				button->debounceCycles = RELEASED_CYCLES;
			} else {
				button->debounceCycles = PRESSED_CYCLES;
			}
			return TRUE; //changed
		}
	}
	return FALSE;
}

int8_t debounce_all_buttons(uint8_t* reportBuffer, int8_t* bufLen) {

	int8_t numButtonsPressed = 0;
	
	*bufLen = 0;
	memset(reportBuffer, 0, REPORT_BUF_LEN);
	
	int8_t iButton;
	for (iButton = 0; iButton < numButtons; iButton++) {
		uint8_t changed = debounce_button(&buttons[iButton]);
		if (buttons[iButton].realState) {
			add_to_buffer(reportBuffer, bufLen, buttons[iButton].key);
			numButtonsPressed++;
		}
	}
	
	return numButtonsPressed;
}
