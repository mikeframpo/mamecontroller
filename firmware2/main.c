
#include "main.h"

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

const PROGMEM char usbHidReportDescriptor[35] = {   /* USB report descriptor */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

#define CREATE_BUTTON(port, pin, key) \
    { \
        port, \
        pin, \
        FALSE, \
        RELEASED_CYCLES, \
        key \
    }

#define REPORT_BUF_LEN 1
static uint8_t reportBuffer[REPORT_BUF_LEN + 1];    /* buffer for HID reports, extra 1 is for the modifier byte. */
static uint8_t idleRate = 1;

uint8_t usbFunctionSetup(uint8_t data[8]) {
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0;

	switch (rq->bRequest) {
		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = (usbMsgPtr_t)&idleRate;
			return 1;
		case USBRQ_HID_SET_IDLE:
			idleRate = rq->wValue.bytes[1];
			return 0;
		case USBRQ_HID_GET_REPORT:
	        usbMsgPtr = (usbMsgPtr_t)reportBuffer;
			return sizeof(reportBuffer);
		default:
			return 0;
	}
}

button_t buttons[] = {

    CREATE_BUTTON(PORT_D, P1_UP, KEY_Q),
    CREATE_BUTTON(PORT_D, P1_DOWN, KEY_W),
    CREATE_BUTTON(PORT_C, P1_LEFT, KEY_E),
    CREATE_BUTTON(PORT_D, P1_RIGHT, KEY_R),
    
    CREATE_BUTTON(PORT_D, P1_START, KEY_F),

    CREATE_BUTTON(PORT_A, P1_A, KEY_A),
    CREATE_BUTTON(PORT_A, P1_B, KEY_S),
    CREATE_BUTTON(PORT_A, P1_C, KEY_D),
    CREATE_BUTTON(PORT_A, P1_D, KEY_Z),
    CREATE_BUTTON(PORT_A, P1_E, KEY_X),
    CREATE_BUTTON(PORT_A, P1_F, KEY_C),
    
    CREATE_BUTTON(PORT_B, P2_UP, KEY_T),
    CREATE_BUTTON(PORT_B, P2_DOWN, KEY_Y),
    CREATE_BUTTON(PORT_B, P2_LEFT, KEY_U),
    CREATE_BUTTON(PORT_B, P2_RIGHT, KEY_I),
    
    CREATE_BUTTON(PORT_A, P2_START, KEY_K),
    
    CREATE_BUTTON(PORT_C, P2_A, KEY_G),
    CREATE_BUTTON(PORT_C, P2_B, KEY_H),
    CREATE_BUTTON(PORT_C, P2_C, KEY_J),
    CREATE_BUTTON(PORT_C, P2_D, KEY_B),
    CREATE_BUTTON(PORT_C, P2_E, KEY_N),
    CREATE_BUTTON(PORT_C, P2_F, KEY_M)
};

static inline bool_t getButtonState(button_t* button) {
    switch(button->port) {
        case PORT_A:
            return (PINA & button->pin) == 0;
        case PORT_B:
            return (PINB & button->pin) == 0;
        case PORT_C:
            return (PINC & button->pin) == 0;
        case PORT_D:
            return (PIND & button->pin) == 0;
        default:
            return FALSE;
    }
}

static inline void resetCycles(button_t* button) {
    if (button->debouncedState) {
        button->cyclesRemaining = RELEASED_CYCLES;
    } else {
        button->cyclesRemaining = DEPRESSED_CYCLES;
    }
}

void debounceButtons(uint8_t* reportBuffer, int8_t* numPressed, int8_t* numChanged) {
    int iButton;
    *numPressed = 0;
    *numChanged = 0;
    memset(reportBuffer, 0, REPORT_BUF_LEN);

    // we don't need to send anything in the modifier byte.
    reportBuffer[0] = 0;

    for (iButton = 0; iButton < sizeof(buttons); iButton++) {

        button_t* button = &buttons[iButton];
        bool_t rawState = getButtonState(button);

        if (rawState == button->debouncedState) {
            resetCycles(button);
        } else {
            button->cyclesRemaining--;
            if (button->cyclesRemaining == 0) {
                button->debouncedState = rawState;
                resetCycles(button);
                (*numChanged)++;
            }
        }

        if (button->debouncedState && *numPressed < REPORT_BUF_LEN) {
            reportBuffer[++(*numPressed)] = button->key;
        }
    }
}

void initButtons() {

    int8_t iButton;
    // pullup on all the inputs.
    for (iButton = 0; iButton < sizeof(buttons); iButton++) {
        switch(buttons[iButton].port) {
            case PORT_A:
                PORTA |= buttons[iButton].pin;
                break;
            case PORT_B:
                PORTB |= buttons[iButton].pin;
                break;
            case PORT_C:
                PORTC |= buttons[iButton].pin;
                break;
            case PORT_D:
                PORTD |= buttons[iButton].pin;
                break;
            default:
                break;
        }
    }
}

//#define FLASH_LED

#ifdef FLASH_LED
void flash_led(void) {
    while (1) {
        PORTD |= RED_LED;
        _delay_ms (1000);
        PORTD &= ~RED_LED;
        _delay_ms (1000);
    }
}
#endif


//TODO
//
//* Probably just need to implement get_idle, set_idle, get_report (prob not used)
//* modify descriptor to use a joystick or non-boot-keyboard to increase button presses.
//* check that timer is correctly initialized, scope?
//* test poll rate is as expected, how is poll rate set?
//* test the device functionality from startup.

int main(void) {

    DDRD |= RED_LED;
#ifdef FLASH_LED
    flash_led();
#endif

    usbDeviceDisconnect();
    _delay_ms(500);
    usbDeviceConnect();

    wdt_enable(WDTO_1S);
    usbInit();

    GICR |= IVCE;
    GICR |= IVSEL;

    sei();

    TCCR0 = 0x5;
    TCNT0 = 0;
    memset(reportBuffer, 0, sizeof(reportBuffer));

    initButtons();

    PORTD |= RED_LED;

    while(1) {
        wdt_reset();
        usbPoll();

        if (TCNT0 > 47) { //47 == 4ms approx
            TCNT0 = 0;

            int8_t numPressed;
            int8_t numChanged;
            debounceButtons(reportBuffer, &numPressed, &numChanged);

            if(usbInterruptIsReady()) {
                usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            }
        }
    }
}
