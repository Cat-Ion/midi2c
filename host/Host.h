/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
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

/** \file
 *
 *  Header file for MIDI.c.
 */

#ifndef _HOST_H_
#define _HOST_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>

#include "Descriptors.h"

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

void SetupHardware(void);
void send_packets(void);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

/** List of codes for non-MIDI commands
 * The first byte of the answer to each command will be the command, but with the high bit set to 1.
 */
enum {
        /** Clients request a continuous block of control addresses using this command
         * There is one parameter: The number of commands.
         * The output is the address of the first control,
         * or 0xFF if none are available.
         */
	GET_ADDRESS = 0x7F
};

/** Possible command states. Can be either waiting for
 * a command, executing some non-MIDI command, or
 * reading MIDI data
 */
enum {
	WAITING_COMMAND = 0x00,
	COMMAND_GET_ADDRESS = 0x01,
	COMMAND_MIDI = 0x02
};
#endif

