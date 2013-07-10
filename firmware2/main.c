
#include "main.h"

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

#define CREATE_BUTTON(port, pin, key) \
    { \
        port, \
        pin, \
        FALSE, \
        RELEASED_CYCLES, \
        key \
    }

button_t buttons[] = {

    CREATE_BUTTON(PORT_D, P1_UP, KEY_W),
    CREATE_BUTTON(PORT_D, P1_DOWN, KEY_S),
    CREATE_BUTTON(PORT_C, P1_LEFT, KEY_A),
    CREATE_BUTTON(PORT_D, P1_RIGHT, KEY_D),
    
    CREATE_BUTTON(PORT_D, P1_START, KEY_Q),

    CREATE_BUTTON(PORT_A, P1_A, MOD_LCTRL),
    CREATE_BUTTON(PORT_A, P1_B, MOD_LSHIFT),
    CREATE_BUTTON(PORT_A, P1_C, MOD_LALT),
    CREATE_BUTTON(PORT_A, P1_D, KEY_R),
    CREATE_BUTTON(PORT_A, P1_E, KEY_R),
    CREATE_BUTTON(PORT_A, P1_F, KEY_R),
    
    CREATE_BUTTON(PORT_B, P2_UP, KEY_U),
    CREATE_BUTTON(PORT_B, P2_DOWN, KEY_J),
    CREATE_BUTTON(PORT_B, P2_LEFT, KEY_H),
    CREATE_BUTTON(PORT_B, P2_RIGHT, KEY_K),
    
    CREATE_BUTTON(PORT_A, P2_START, KEY_R),
    
    CREATE_BUTTON(PORT_C, P2_A, KEY_R),
    CREATE_BUTTON(PORT_C, P2_B, KEY_R),
    CREATE_BUTTON(PORT_C, P2_C, KEY_R),
    CREATE_BUTTON(PORT_C, P2_D, KEY_R),
    CREATE_BUTTON(PORT_C, P2_E, KEY_R),
    CREATE_BUTTON(PORT_C, P2_F, KEY_R),
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))
#define SIMUL_BUTTONS 9
#define REPORT_COUNT (SIMUL_BUTTONS + 1)

const PROGMEM char usbHidReportDescriptor[37] = {
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
    
    0x95, 0x09,                    //   REPORT_COUNT
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                           // END_COLLECTION
};

static uint8_t reportBuffer[REPORT_COUNT];
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

void debounceButtons(uint8_t* reportBuffer) {
    int iButton;
    int iReport = 0;
    memset(reportBuffer, 0, REPORT_COUNT);

    for (iButton = 0; iButton < NUM_BUTTONS; iButton++) {

        button_t* button = &buttons[iButton];
        bool_t rawState = getButtonState(button);

        if (rawState == button->debouncedState) {
            resetCycles(button);
        } else {
            button->cyclesRemaining--;
            if (button->cyclesRemaining == 0) {
                button->debouncedState = rawState;
                resetCycles(button);
            }
        }

        if (button->debouncedState) {
            //TODO: write the report
        }
    }
}

void initButtons() {

    int8_t iButton;
    // pullup on all the inputs.
    for (iButton = 0; iButton < NUM_BUTTONS; iButton++) {
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

void toggle_led(void) {
    if (PIND & RED_LED) {
        PORTD &= ~RED_LED;
    } else {
        PORTD |= RED_LED;
    }
}

//TODO
//
//* Probably just need to implement get_idle, set_idle, get_report (prob not used)
//* test poll rate is as expected, how is poll rate set?
//* check that timer is correctly initialized, scope?
//* test the device functionality from startup.
//* see how much we can reduce the depressed/release cycles to.
// 
// idle implementation:
// if idle is 0, only send when report has changed.
// if non-zero, send every idle * 4ms
// if polled and not time to send yet, send NAK
// if anything changed, report anyway
//
// keyboard
// non-modifiers must be input,array,absolute
//

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

    TCCR0 = 0x5; // F_CPU / 1024
    TCCR1B = 0x1; // F_CPU / 1
    TCNT0 = 0;
    TCNT1 = 0;
    memset(reportBuffer, 0, sizeof(reportBuffer));

    initButtons();

    PORTD |= RED_LED;

    while(1) {
        wdt_reset();
        usbPoll();

        if (TCNT1 > 1200) { //1200 = 100us
            TCNT1 = 0;
            debounceButtons(reportBuffer);
        }

        if (TCNT0 > 47) { //47 == 4ms approx
            TCNT0 = 0;
            if(usbInterruptIsReady()) {
                usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            }
        }
    }
}
