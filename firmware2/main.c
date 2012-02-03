
#include "main.h"

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
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
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

#define REPORT_BUF_LEN 14
static uint8_t reportBuffer[REPORT_BUF_LEN];    /* buffer for HID reports */
static uint8_t idleRate = 1;           /* in 4 ms units */
static uint8_t protocolVer = 1;    /* 0 is boot protocol, 1 is report protocol */
static uint8_t expectReport = 0;

uint8_t usbFunctionSetup(uint8_t data[8]) {
	usbRequest_t *rq = (void *)data;
	usbMsgPtr = reportBuffer;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0;

	switch (rq->bRequest) {
		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = &idleRate;
			return 1;
		case USBRQ_HID_SET_IDLE:
			idleRate = rq->wValue.bytes[1];
			return 0;
		case USBRQ_HID_GET_REPORT:
			return sizeof(reportBuffer);
		case USBRQ_HID_SET_REPORT:
			if (rq->wLength.word == 1)
				expectReport = 1;
			return expectReport == 1 ? 0xFF : 0;
		case USBRQ_HID_GET_PROTOCOL:
			if (rq->wValue.bytes[1] < 1)
				protocolVer = rq->wValue.bytes[1];
			return 0;
		case USBRQ_HID_SET_PROTOCOL:
			usbMsgPtr = &protocolVer;
			return 1;
		default:
			return 0;
	}
}

#define NUM_BUTTONS 12
button_t buttons[NUM_BUTTONS];

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
                (*numChanged)++;
            }
        }
        
        if (button->debouncedState) {
            // TODO: add buffer overflow protection.
            reportBuffer[++(*numPressed)] = button->key;
        }
    }
}

void addButton(port_t port, uint8_t pin, keycode_t key, int* index) {
    
    buttons[*index].port = port;
    buttons[*index].pin = pin;
    buttons[*index].debouncedState = FALSE;
    buttons[*index].cyclesRemaining = RELEASED_CYCLES;
    buttons[*index].key = key;
    
    //pullup
    switch(port) {
        case PORT_A:
            PORTA |= pin;
            break;
        case PORT_B:
            PORTB |= pin;
            break;
        case PORT_C:
            PORTC |= pin;
            break;
        case PORT_D:
            PORTD |= pin;
            break;
        default:
            break;
    }
    
    (*index)++;
}

void initButtons() {
    
    int index = 0;

    addButton(PORT_A, P1_A, KEY_A, &index);
    addButton(PORT_A, P1_B, KEY_S, &index);
    addButton(PORT_A, P1_C, KEY_D, &index);
    addButton(PORT_A, P1_D, KEY_Z, &index);
    addButton(PORT_A, P1_E, KEY_X, &index);
    addButton(PORT_A, P1_F, KEY_C, &index);
    
    addButton(PORT_C, P2_A, KEY_G, &index);
    addButton(PORT_C, P2_B, KEY_H, &index);
    addButton(PORT_C, P2_C, KEY_J, &index);
    addButton(PORT_C, P2_D, KEY_B, &index);
    addButton(PORT_C, P2_E, KEY_N, &index);
    addButton(PORT_C, P2_F, KEY_M, &index);
}

int main(void) {

    DDRD |= RED_LED;
    
    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
	 
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    uint8_t i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();

    GICR |= IVCE;
    GICR |= IVSEL;
    
    sei();
    
    TCCR0 = 0x5;
    TCNT0 = 0;
    memset(reportBuffer, 0, sizeof(reportBuffer));
    
    initButtons();
    
    uint8_t idleCounter = 0;
    bool_t updateNeeded = FALSE;
    
    PORTD |= RED_LED;
    
    while(1) {
        wdt_reset();
        usbPoll();
        
        if (TCNT0 > 47) { //47 == 4ms approx
            TCNT0 = 0;
            
            int8_t numPressed;
            int8_t numChanged;
            debounceButtons(reportBuffer, &numPressed, &numChanged);
	    
	    if (idleRate != 0) {
		if (--idleCounter == 0) {
			idleCounter = idleRate;
			updateNeeded = TRUE;
		}
	    }
	    
	    if(updateNeeded && usbInterruptIsReady()) {
		updateNeeded = FALSE;
		usbSetInterrupt(reportBuffer, 8);
		usbPoll();
		while(!usbInterruptIsReady()) {
		    wdt_reset();
		}
		usbSetInterrupt(reportBuffer + 8, 6);
	    }
        }
    }
}
