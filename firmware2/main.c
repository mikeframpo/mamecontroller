
#include "main.h"

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

#define CREATE_BUTTON(port, pin, btype) \
    { \
        port, \
        pin, \
        FALSE, \
        RELEASED_CYCLES \
        btype \
    }

button_t buttons[] = {

    //TODO: put these into a different array for each set.
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

const PROGMEM char usbHidReportDescriptor[43] = {
    //0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    //0x09, 0x05,                    // USAGE (Game Pad)
    //0xa1, 0x01,                    // COLLECTION (Application)
    //0x05, 0x09,                    //     USAGE_PAGE (Button)
    //0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    //0x29, 0x18,                    //     USAGE_MAXIMUM (Button 24)
    //0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    //0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    //0x95, NUM_BUTTONS,             //     REPORT_COUNT (24)
    //0x75, 0x01,                    //     REPORT_SIZE (1)
    //0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    //0xc0                           // END_COLLECTION

    /* Controller and report_id 1 */
    0x05, 0x01,         // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,         // USAGE (Joystick)
    0xa1, 0x01,         //  COLLECTION (Application)
    0x09, 0x01,         //      USAGE (Pointer)
    0xa1, 0x00,         //      COLLECTION (Physical)
    0x85, 0x01,         //          REPORT_ID (1)
    0x07, 0x30,         //          USAGE (X)
    0x09, 0x31,         //          USAGE (Y)
    //TODO: change this to a signed format.
    0x15, 0x00,         //          LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,   //          LOGICAL_MAXIMUM (255)
    0x75, 0x08,         //          REPORT_SIZE (8)
    0x95, 0x02,         //          REPORT_COUNT (2)
    0x81, 0x02,         //          INPUT (Data,Var,Abs)

    0x05, 0x09,			//			USAGE_PAGE (Button)
    0x19, 1,			//   		USAGE_MINIMUM (Button 1)
    0x29, 8,			//   		USAGE_MAXIMUM (Button 8)
    0x15, 0x00,			//   		LOGICAL_MINIMUM (0)
    0x25, 0x01,			//   		LOGICAL_MAXIMUM (1)
    0x75, 1,			// 			REPORT_SIZE (1)
    0x95, 8,			//			REPORT_COUNT (8)
    0x81, 0x02,			//			INPUT (Data,Var,Abs)
	0xc0,				//		END_COLLECTION
    0xc0,				// END_COLLECTION
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

static inline void buildReport (button_t* button, uint8_t* reportBuffer) {
    switch (button->btype) {
        case GP_UP:
            ((int8_t)reportBuffer[1]) = 127;
            break;
        case GP_DOWN:
            ((int8_t)reportBuffer[1]) = -127;
            break;
        case GP_LEFT:
            ((int8_t)reportBuffer[2]) = -127;
            break;
        case GP_RIGHT:
            ((int8_t)reportBuffer[2]) = 127;
            break;
        default:
            reportBuffer[3] &= (1 << button->btype);
    }
}

void debounceButtons(uint8_t* reportBuffer) {
    int iButton;
    //TODO: replace this with initReport to get x,y into nice initial values.
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

        // Generate the report.
        if (button->debouncedState) {
            // [0]  report id
            // [1]  x pos 0-255
            // [2]  y pos 0-255
            // [3]  [0..5] buttons, [6] start, [7] coin/quit.
            buildReport(button, reportBuffer);
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
//* test if getReport in usb function is hit
//* test poll rate is as expected, how is poll rate set?
//* check that timer is correctly initialized, scope?
//* test the device functionality from startup.
//* see how much we can reduce the depressed/release cycles to.
//
// 
// idle implementation:
// if idle is 0, only send when report has changed.
// if non-zero, send every idle * 4ms
// if polled and not time to send yet, send NAK
// if anything changed, report anyway
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
