
#include "main.h"

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

#define CREATE_BUTTON(port, pin) \
    { \
        port, \
        pin, \
        FALSE, \
        RELEASED_CYCLES \
    }

button_t buttons[] = {

    CREATE_BUTTON(PORT_D, P1_UP),
    CREATE_BUTTON(PORT_D, P1_DOWN),
    CREATE_BUTTON(PORT_C, P1_LEFT),
    CREATE_BUTTON(PORT_D, P1_RIGHT),
    
    CREATE_BUTTON(PORT_D, P1_START),

    CREATE_BUTTON(PORT_A, P1_A),
    CREATE_BUTTON(PORT_A, P1_B),
    CREATE_BUTTON(PORT_A, P1_C),
    CREATE_BUTTON(PORT_A, P1_D),
    CREATE_BUTTON(PORT_A, P1_E),
    CREATE_BUTTON(PORT_A, P1_F),
    
    CREATE_BUTTON(PORT_B, P2_UP),
    CREATE_BUTTON(PORT_B, P2_DOWN),
    CREATE_BUTTON(PORT_B, P2_LEFT),
    CREATE_BUTTON(PORT_B, P2_RIGHT),
    
    CREATE_BUTTON(PORT_A, P2_START),
    
    CREATE_BUTTON(PORT_C, P2_A),
    CREATE_BUTTON(PORT_C, P2_B),
    CREATE_BUTTON(PORT_C, P2_C),
    CREATE_BUTTON(PORT_C, P2_D),
    CREATE_BUTTON(PORT_C, P2_E),
    CREATE_BUTTON(PORT_C, P2_F),
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))
#define REPORT_SIZE 3 /* 1 bit per button, rounded up to the nearest byte. */

const PROGMEM char usbHidReportDescriptor[23] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x18,                    //     USAGE_MAXIMUM (Button 24)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, NUM_BUTTONS,             //     REPORT_COUNT (24)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};

static uint8_t reportBuffer[REPORT_SIZE];    /* buffer for HID reports, extra 1 is for the modifier byte. */
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
    memset(reportBuffer, 0, REPORT_SIZE);

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
            int8_t bytepos = iButton >> 3; // equivalent to divide by 8
            int8_t bitpos = iButton % 8;
            reportBuffer[bytepos] |= (1 << bitpos);
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

//#define TEST_REPORT


//TODO
//
//* Probably just need to implement get_idle, set_idle, get_report (prob not used)
//* check that timer is correctly initialized, scope?
//* test poll rate is as expected, how is poll rate set?
//* test the device functionality from startup.

int main(void) {

#ifdef TEST_REPORT
    int8_t iButton = 0;
    uint8_t iPoll = 0;
#endif

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
#ifdef TEST_REPORT
            if (iPoll == UINT8_MAX) {
                toggle_led ();
                iButton = (iButton + 1) % NUM_BUTTONS;
                int8_t iByte = iButton >> 3;
                int8_t iBit = iButton % 8;
                memset(reportBuffer, 0, sizeof(reportBuffer));
                reportBuffer[iByte] |= (1 << iBit);
            }
            iPoll++;
#else
            debounceButtons(reportBuffer);
#endif

            if(usbInterruptIsReady()) {
                usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            }
        }
    }
}
