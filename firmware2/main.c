
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
        RELEASED_CYCLES, \
        btype \
    }

button_t gp1_buttons[] = {

    CREATE_BUTTON(PORT_D, (1 << 1), GP_UP),
    CREATE_BUTTON(PORT_C, (1 << 7), GP_DOWN),
    CREATE_BUTTON(PORT_D, (1 << 3), GP_LEFT),
    CREATE_BUTTON(PORT_D, (1 << 4), GP_RIGHT),
    
    CREATE_BUTTON(PORT_A, (1 << 1), GP_BUT_A),
    CREATE_BUTTON(PORT_A, (1 << 3), GP_BUT_B),
    CREATE_BUTTON(PORT_A, (1 << 5), GP_BUT_C),
    CREATE_BUTTON(PORT_A, (1 << 2), GP_BUT_D),
    CREATE_BUTTON(PORT_A, (1 << 4), GP_BUT_E),
    CREATE_BUTTON(PORT_A, (1 << 6), GP_BUT_F),

    CREATE_BUTTON(PORT_A, (1 << 0), GP_BUT_START),
    CREATE_BUTTON(PORT_D, (1 << 0), GP_BUT_EXTRA) //quit
};

button_t gp2_buttons[] = {

    CREATE_BUTTON(PORT_B, (1 << 3), GP_UP),
    CREATE_BUTTON(PORT_B, (1 << 0), GP_DOWN),
    CREATE_BUTTON(PORT_B, (1 << 1), GP_LEFT),
    CREATE_BUTTON(PORT_B, (1 << 2), GP_RIGHT),

    CREATE_BUTTON(PORT_C, (1 << 1), GP_BUT_A),
    CREATE_BUTTON(PORT_C, (1 << 3), GP_BUT_B),
    CREATE_BUTTON(PORT_C, (1 << 5), GP_BUT_C),
    CREATE_BUTTON(PORT_C, (1 << 2), GP_BUT_D),
    CREATE_BUTTON(PORT_C, (1 << 4), GP_BUT_E),
    CREATE_BUTTON(PORT_C, (1 << 6), GP_BUT_F),

    CREATE_BUTTON(PORT_A, (1 << 7), GP_BUT_START),
    CREATE_BUTTON(PORT_D, (1 << 7), GP_BUT_EXTRA) //coin
};

#define NUM_GAMEPADS 2

gamepad_t gamepads[] = {
    {
        .id = 1,
        .buttons = gp1_buttons,
        .num_buttons = sizeof(gp1_buttons) / sizeof(gp1_buttons[0]),
    },
    {
        .id = 2,
        .buttons = gp2_buttons,
        .num_buttons = sizeof(gp2_buttons) / sizeof(gp2_buttons[0]),
    }
};

const PROGMEM char usbHidReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x01,                    //     REPORT_ID(1)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x08,                    //     REPORT_COUNT (8)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0,                           // END_COLLECTION

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x02,                    //     REPORT_ID(2)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x08,                    //     REPORT_COUNT (8)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0                           // END_COLLECTION
};

#define REPORT_SIZE 4 /* 1 bit per button, rounded up to the nearest byte. */
static uint8_t reportBuffer[REPORT_SIZE];    /* buffer for HID reports, extra 1 is for the modifier byte. */

uint8_t usbFunctionSetup(uint8_t data[8]) {
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0;

	switch (rq->bRequest) {
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

static int8_t buildReport(gamepad_t* gp, uint8_t* reportBuffer) {

    int iBut;
    button_t* button;

    memset(reportBuffer, 0, REPORT_SIZE);
    reportBuffer[0] = gp->id;

    for (iBut = 0; iBut < gp->num_buttons; iBut++) {
        button = &gp->buttons[iBut];
        if (button->debouncedState) {
            switch (button->btype) {
                case GP_LEFT:
                    reportBuffer[1] = -127;
                    break;
                case GP_RIGHT:
                    reportBuffer[1] = 127;
                    break;
                case GP_UP:
                    reportBuffer[2] = 127;
                    break;
                case GP_DOWN:
                    reportBuffer[2] = -127;
                    break;
                default:
                    reportBuffer[3] |= (1 << button->btype);
            }
        }
    }

    return REPORT_SIZE;
}

bool_t debounceButtons(gamepad_t* gamepad) {
    uint8_t iButton;
    bool_t any_changed = FALSE;
    for (iButton = 0; iButton < gamepad->num_buttons; iButton++) {

        button_t* button = &gamepad->buttons[iButton];
        bool_t rawState = getButtonState(button);

        if (rawState == button->debouncedState) {
            resetCycles(button);
        } else {
            button->cyclesRemaining--;
            if (button->cyclesRemaining == 0) {
                button->debouncedState = rawState;
                resetCycles(button);
                any_changed = TRUE;
            }
        }
    }
    return any_changed;
}

uint8_t debounceGamepads(void) {
    int8_t gp;
    uint8_t must_report = 0;
    for (gp = 0; gp < NUM_GAMEPADS; gp++) {
        if (debounceButtons(&gamepads[gp])) {
            must_report |= (1 << gp);
        }
    }
    return must_report;
}

void initButtons(gamepad_t* gamepad) {

    int8_t iButton;
    button_t* buttons;

    // pullup on all the inputs.
    buttons = gamepad->buttons;
    for (iButton = 0; iButton < gamepad->num_buttons; iButton++) {
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

inline void pollButtons(uint8_t* must_report) {
    if (TCNT1 > 1200) { //1200 = 100us
        TCNT1 = 0;
        *must_report |= debounceGamepads();
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
    wdt_disable();

    usbInit();

    GICR |= IVCE;
    GICR |= IVSEL;

    sei();

    TCCR0 = 0x5; // F_CPU / 1024
    TCCR1B = 0x1; // F_CPU / 1
    TCNT0 = 0;
    TCNT1 = 0;
    memset(reportBuffer, 0, sizeof(reportBuffer));

    int8_t gp;
    for (gp = 0; gp < NUM_GAMEPADS; gp++) {
        initButtons(&gamepads[gp]);
    }

    // put buttons in the initial state.
    int8_t db;
    for (db = 0; db < 100; db++)
        debounceGamepads();

    uint8_t must_report = 0;
    while(1) {

        wdt_reset();
        usbPoll(); //this must be called approx every 50ms

        pollButtons(&must_report);

        if (must_report) {
            for (gp = 0; gp < NUM_GAMEPADS; gp++) {
                if (must_report & (1 << gp) == 0)
                    continue;

                if (usbInterruptIsReady()) {
                    buildReport(&gamepads[gp], reportBuffer);
                    usbSetInterrupt(reportBuffer, REPORT_SIZE);
                    must_report &= ~(1 << gp);

                    while (!usbInterruptIsReady()) {
                        usbPoll();
                        wdt_reset();
                        pollButtons(&must_report);
                    }
                }
            }
        }
    }
}


