/* Name: main.c
 * Project: hid-mouse, a very simple HID example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: main.c 692 2008-11-07 15:07:40Z cs $
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.

We use VID/PID 0x046D/0xC00E which is taken from a Logitech mouse. Don't
publish any hardware using these IDs! This is for demonstration only!
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <string.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include "main.h"
#include "buttons.h"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

/* USB report descriptor (length is defined in usbconfig.h)
   This has been changed to conform to the USB keyboard boot protocol */
char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] PROGMEM = {
	0x05, 0x01,            // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,            // USAGE (Keyboard)
	0xa1, 0x01,            // COLLECTION (Application)
	0x05, 0x07,            //   USAGE_PAGE (Keyboard)
	
	0x95, 0x08,            //   REPORT_COUNT (6)
	0x75, 0x08,            //   REPORT_SIZE (8)
	0x15, 0x00,            //   LOGICAL_MINIMUM (0)
	0x25, 0x65,            //   LOGICAL_MAXIMUM (101)
	0x05, 0x07,            //   USAGE_PAGE (Keyboard)
	
	0x19, 0x00,            //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,            //   USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,            //   INPUT (Data,Ary,Abs)
	0xc0                   // END_COLLECTION
};

/* The ReportBuffer contains the USB report sent to the PC */

static uint8_t reportBuffer[REPORT_BUF_LEN];    /* buffer for HID reports */
static int8_t bufLen;
static uint8_t idleRate;           /* in 4 ms units */
static uint8_t protocolVer = 1;    /* 0 is boot protocol, 1 is report protocol */

uint8_t expectReport = 0;

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

uint8_t convertToKey(uint8_t nibble) {
	if (nibble == 0x0) {
		return KEY_0;
	} else if (nibble < 0xA) {
		return KEY_1 + nibble - 1;
	} else if (nibble <= 0xF) {
		return KEY_A + nibble - 0xA;
	} else {
		return KEY_X;
	}
}

void transmitKey(uint8_t key) {
	
	uint8_t sendBuffer[8];
	memset(sendBuffer, 0, sizeof(sendBuffer));
	
	sendBuffer[0] = key;
	for (;;) {
		wdt_reset();
		usbPoll();
		if (TCNT0 > 150) { //approx 10ms
			
			TCNT0 = 0;
			if(usbInterruptIsReady()) {
				usbSetInterrupt(sendBuffer, sizeof(sendBuffer));
				return;
			}
		}
	}
}

void pressKey(uint8_t key) {
	transmitKey(key);
	transmitKey(0);
}

void dumpMemory(void* ptr, int bytes) {
	
	uint8_t* u8ptr = (uint8_t*)ptr;
	uint8_t high, low;
	
	while (bytes > 0) {
		
		high = low = *u8ptr;
		high = high >> 4;
		low = low & 0x0F;
	
		pressKey(convertToKey(high));
		pressKey(convertToKey(low));
		
		u8ptr++;
		bytes--;
	}
}

/* ------------------------------------------------------------------------- */

extern button_t buttons[0];

int main(void)
{
uint8_t   i;

	DDRD |= RED_LED;
	
	//DDRC = 0;
	//PORTC = 0xFF;
	//while(1) {
	//;;
	//}
	//PORTD |= RED_LED;
	
	//PORTC |= (1 << 2);
	
	//while(1) {
	//	if (PINC & (1 << 2)) {
	//		PORTD &= ~RED_LED;
	//	} else {
	//		PORTD |= RED_LED;
	//	}
	//}

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
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();

    GICR |= IVCE;
    GICR |= IVSEL;
	
	//init 8bit timer to fclk/1024
	TCCR0 = 0x5;
	TCNT0 = 0;
	
	bufLen = 0;
	memset(reportBuffer, 0, sizeof(reportBuffer));

    sei();
	
	init_buttons();
	
	int8_t numButtonsChanged = 0;
	
#ifdef DEBUG_DUMP_REPORTBUFFER
	uint8_t prevReportBuffer[8];
	memset(prevReportBuffer, 0, sizeof(prevReportBuffer));
#endif
	
    for(;;){                
		/* main event loop */
        wdt_reset();
        usbPoll();
		
		if (TCNT0 > 60) { //approx 5ms
			
			TCNT0 = 0;
			numButtonsChanged = debounce_all_buttons(reportBuffer, &bufLen);
			
#ifdef DEBUG_DEBOUNCE_COUNT
			memset(reportBuffer, 0, sizeof(reportBuffer));
			reportBuffer[0] = KEY_A + numButtonsPressed;
#endif

#ifdef DEBUG_BUF_LEN
			memset(reportBuffer, 0, sizeof(reportBuffer));
			reportBuffer[0] = KEY_A + bufLen;
#endif

#ifdef DEBUG_REPORT_BUF
			int8_t numNonZeroEntries = 0;
			int8_t iBuf;
			for (iBuf = 0; iBuf < sizeof(reportBuffer); iBuf++) {
				if (reportBuffer[iBuf] != 0) {
					numNonZeroEntries++;
				}
			}
			memset(reportBuffer, 0, sizeof(reportBuffer));
			reportBuffer[0] = KEY_A + numNonZeroEntries;
#endif

#ifdef DEBUG_CAN_SEND_EIGHT
			if (reportBuffer[0]) {
				int8_t iBuf;
				for (iBuf = 0; iBuf < sizeof(reportBuffer); iBuf++) {
					reportBuffer[iBuf] = KEY_A + iBuf;
				}
			}
#endif

#ifdef DEBUG_CAN_SEND_PLAYERS_BUTTONS
			if (reportBuffer[0] == KEY_Z) {
				reportBuffer[0] = KEY_Z;
				reportBuffer[1] = KEY_B;
				reportBuffer[2] = KEY_V;
				reportBuffer[3] = KEY_M;
				reportBuffer[4] = KEY_N;
				reportBuffer[5] = KEY_C;
			} else if (reportBuffer[0] == KEY_KP5) {
				reportBuffer[0] = KEY_KP5;
				reportBuffer[1] = KEY_KPplus;
				reportBuffer[2] = KEY_0;
				reportBuffer[3] = KEY_6;
				reportBuffer[4] = KEY_4;
				reportBuffer[5] = KEY_3;
			}
#endif

#ifdef DEBUG_CAN_SEND_PLAYERS_BUTTONS_2
			if (reportBuffer[0] == KEY_Z) {
				reportBuffer[0] = buttons[0].key;
				reportBuffer[3] = buttons[4].key;
				reportBuffer[2] = buttons[3].key;
				reportBuffer[5] = buttons[6].key;
				reportBuffer[4] = buttons[5].key;
				reportBuffer[1] = buttons[2].key;
			} else if (reportBuffer[0] == KEY_KP5) {
				reportBuffer[0] = buttons[8].key;
				reportBuffer[1] = buttons[12].key;
				reportBuffer[2] = buttons[7].key;
				reportBuffer[3] = buttons[13].key;
				reportBuffer[4] = buttons[11].key;
				reportBuffer[5] = buttons[10].key;
			}
#endif

#ifdef DEBUG_TESTDUMP
			if (numButtonsChanged > 0 && reportBuffer[0] != 0) {	
				//uint8_t testArr[] = { 0xAB, 0xCD, 0xEF, 0x12, 0x34 };
				//dumpMemory(testArr, 5);
				
				//uint8_t testArr[] = { 0xAD, 0xCC };
				//dumpMemory(buttons, sizeof(button_t));
				dumpMemory(&buttons[0].pinPort, sizeof(enum port_enum));
				dumpMemory(&buttons[0].pinMask, sizeof(uint8_t));
				dumpMemory(&buttons[0].realState, sizeof(int8_t));
				dumpMemory(&buttons[0].debounceCycles, sizeof(uint8_t));
				dumpMemory(&buttons[0].key, sizeof(uint8_t));
			}
#endif

#ifdef DEBUG_DUMP_REPORTBUFFER
			if (memcmp(reportBuffer, prevReportBuffer, sizeof(reportBuffer)) != 0) {
				
				dumpMemory(reportBuffer, sizeof(reportBuffer));
				memcpy(prevReportBuffer, reportBuffer, sizeof(reportBuffer));
				memset(reportBuffer, 0, sizeof(reportBuffer));
				
				bufLen = 0;
				pressKey(KEY_enter);		
			}
#endif

		}

#ifndef DEBUG_DUMP_REPORTBUFFER
		if(usbInterruptIsReady()) {
		
			usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
			memset(reportBuffer, 0, sizeof(reportBuffer));
			bufLen = 0;
			numButtonsChanged = 0;
		}
#endif
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
