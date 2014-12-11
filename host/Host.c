/* This file is a modified copy of the MIDI class driver
 * example of the LUFA library, found on lufa-lib.org.
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "Host.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_BAUD 115200
#include "usart.h"
#define DEBUG_SEND(x) usart_putchar(x)
#define DEBUG_INIT() usart_init(CALC_UBRR(DEBUG_BAUD))
#else
#define DEBUG_SEND(x)
#define DEBUG_INIT()
#endif

uint8_t control_number = 0; ///< Number of controls registered so far
#define MIDI_BUFFER_SIZE 30 ///< Size of the MIDI buffer, in bytes
uint8_t midi_buffer[MIDI_BUFFER_SIZE];  ///< Buffer of MIDI messages to be written to the USB
volatile uint8_t midi_buffer_end_position = 0; ///< Points after the last index written to by the I2C ISR
volatile uint8_t midi_buffer_start_position = 0; ///< Index of the next byte that should be written to the USB

uint8_t command_buffer[4]; ///< Buffer for data for the command that is currently being read
uint8_t command_buffer_position = 0; ///< Position in #command_buffer
uint8_t state = WAITING_COMMAND; ///< Command that is currently being read, or WAITING_COMMAND
uint8_t bytes_written = 0; ///< Number of bytes written in answer to a command

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint          =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	GlobalInterruptEnable();

	for (;;)
	{
		if(midi_buffer_end_position != midi_buffer_start_position) {
                        send_packets();
                }

		MIDI_EventPacket_t ReceivedMIDIEvent;
		while (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
        DEBUG_INIT();

	/* TWI stuff */
	// Address 0x01, listen to general calls
	TWAR = ((0x3C << 1) | 1);
	// Clear status
	TWSR = 0;
	// Enable ACK, enable TWI, clear interrupt, enable interrupt
	TWCR = (1<<TWEA) | (1<<TWEN) | (1<<TWINT) | (1<<TWIE);
	// Port as output and high
	//PORTD |= 3;
	//DDRD |= 3;

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();
}

/** Checks for fresh buffered data, sending MIDI events to the host upon each change. */
void send_packets(void)
{
        do {
                DEBUG_SEND(0xFE);
                uint8_t command, data2, data3;
                command = midi_buffer[midi_buffer_start_position++];
                if(midi_buffer_start_position == 30) midi_buffer_start_position = 0;
                data2   = midi_buffer[midi_buffer_start_position++];
                if(midi_buffer_start_position == 30) midi_buffer_start_position = 0;
                data3   = midi_buffer[midi_buffer_start_position++];
                if(midi_buffer_start_position == 30) midi_buffer_start_position = 0;
                
                MIDI_EventPacket_t event = (MIDI_EventPacket_t) {
                        .Event = MIDI_EVENT(0, command),
                        .Data1 = command,
                        .Data2 = data2,
                        .Data3 = data3
                };

                MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &event);
        } while(midi_buffer_start_position != midi_buffer_end_position);

        MIDI_Device_Flush(&Keyboard_MIDI_Interface);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

ISR(TWI_vect) {
        //DEBUG_SEND(TWSR);
	switch(TWSR) {
	case 0x60: // Received own address
	case 0x70: // Received general address
		// Got a write request. Acknowledge.
		TWCR |= (1<<TWINT) | (1<<TWEA);
		break;
	case 0x80: // Read byte sent to own address
	case 0x90: // Read byte sent to general address
		switch(state) {
		case WAITING_COMMAND:
                        // This byte starts a new command. Check which one it is.
			switch(TWDR) {
			case GET_ADDRESS:
				state = COMMAND_GET_ADDRESS;
                                command_buffer_position = 0;
                                break;
			default:
				state = COMMAND_MIDI;
                                command_buffer[0] = TWDR;
                                command_buffer_position = 1;
                                break;
			}
			break;
		case COMMAND_GET_ADDRESS:
			command_buffer[command_buffer_position++] = TWDR;
                        // Prevent a buffer overflow
			if(command_buffer_position == 4) command_buffer_position = 3;
			break;
		case COMMAND_MIDI:
			command_buffer[command_buffer_position++] = TWDR;
			if(command_buffer_position == 3) {
                                // Completed a MIDI command, write it to the MIDI buffer
				midi_buffer[midi_buffer_end_position++] = command_buffer[0];
				if(midi_buffer_end_position == 30) midi_buffer_end_position = 0;
				midi_buffer[midi_buffer_end_position++] = command_buffer[1];
				if(midi_buffer_end_position == 30) midi_buffer_end_position = 0;
				midi_buffer[midi_buffer_end_position++] = command_buffer[2];
				if(midi_buffer_end_position == 30) midi_buffer_end_position = 0;
                                if(midi_buffer_start_position == midi_buffer_end_position) {
                                        midi_buffer_start_position += 3;
                                        if(midi_buffer_start_position == 30) {
                                                midi_buffer_start_position = 0;
                                        }
                                }
                                DEBUG_SEND(midi_buffer_start_position);
                                DEBUG_SEND(midi_buffer_end_position);
			} else if(command_buffer_position == 4) {
                                // Only handle one MIDI message between one START/STOP,
                                // so just write to the end of the buffer and ignore it
				command_buffer_position = 3;
			}
			break;
		default:
			break;
		}
                // Acknowledge the byte
		TWCR |= (1<<TWINT) | (1<<TWEA);
		break;
	case 0xA0:
		// Master has sent STOP or repeated START. Continue as usual.
		switch(state) {
		case COMMAND_MIDI:
			// Wait for a new command
			state = WAITING_COMMAND;
			break;
		case COMMAND_GET_ADDRESS:
			// Next we'll want to write out the answer, so don't change the state.
			break;
		default:
			break;
		}
                // Clear the interrupt and set TWEA to keep listening
		TWCR |= (1<<TWINT) | (1<<TWEA);
		break;
	case 0xA8:
		// Received own address and a read request
		switch(state) {
		case COMMAND_GET_ADDRESS:
                        // Previously finished a GET_ADDRESS command, acknowledge it
			TWDR = GET_ADDRESS | 0x80;
			TWCR |= (1<<TWINT) | (1<<TWEA);
			break;
		default:
                        // No previous command, or command doesn't require a response.
                        // Send 0xFF.
			TWDR = 0xFF;
			TWCR = (TWCR | (1<<TWINT)) & ~(1<<TWEA);
			break;
		}
		bytes_written = 1;
		break;
	case 0xB8:
		// Sent a byte, got ACK
		switch(state) {
		case COMMAND_GET_ADDRESS:
			switch(bytes_written) {
			case 1:
                                // Only send an address if we've received a full request
                                if(command_buffer_position >= 1) {
        				TWDR = control_number > 127 ? 255 : control_number;
                                } else {
                                        TWDR = 0xFF;
                                }
				// last byte, so no TWEA
				TWCR = (TWCR | (1<<TWINT)) & ~(1<<TWEA);
				break;
			case 2:
				// Shouldn't have received an ACK, do not increase the control counter yet.
				bytes_written = 1; // will be 2 after we've increased it below
				// Just write high bytes for now
				TWDR = 0xFF;
				TWCR = (TWCR | (1<<TWINT)) & ~(1<<TWEA);
				break;
			}
			bytes_written++;
			break;
		default:
			TWDR = 0xFF;
			TWCR = (TWCR | (1<<TWINT)) & ~(1<<TWEA);
		}
		break;
	case 0xC0:
		// Sent a byte (not the last one), got NAK
		// Acknowledge and stay responsive
		TWCR |= (1<<TWINT) | (1<<TWEA);
		break;
	case 0xC8:
		// Sent last byte, got NAK
		switch(state) {
		case COMMAND_GET_ADDRESS:
			// Okay, increase the control counter.
                        // But only if the message was completed
			if(bytes_written == 2 && control_number < 127 && command_buffer_position >= 1) {
				control_number += command_buffer[0];
			}
			break;
		default:
			break;
		}
		TWCR |= (1<<TWINT) | (1<<TWEA);
		break;
	case 0x00:
		TWCR |= (1<<TWSTO);
		// fall through
	default:
		// shouldn't happen
		TWCR |= (1<<TWINT);
		break;
	}
}
