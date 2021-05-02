#include <dos.h>

#include "INPUT.H"
#include "TYPES.H"

#define SCAN_QUEUE_SIZE 256
#define MAX_INPUT 256

#define KEY_UP_MASK 0x80
#define KEY_ALL_MASK 0x7F

static void interrupt (far *oldkb)(void); // BIOS keyboard handler

byte gb_scan;
byte gb_scan_q[SCAN_QUEUE_SIZE];
byte gb_scan_head;
byte gb_scan_tail;

event_t input_queue[SCAN_QUEUE_SIZE];
byte i_head;
byte i_tail;

// Add event to input queue
void add_input(event_t *event) {
	input_queue[i_tail].type = event->type;
	input_queue[i_tail].sub_type = event->sub_type;
	input_queue[i_tail].x = event->x;
	input_queue[i_tail].y = event->y;
	input_queue[i_tail].data1 = event->data1;
	input_queue[i_tail].data2 = event->data2;

	i_tail++;
	if (i_tail == MAX_INPUT) {
		i_tail = 0;
	}

	if (i_tail == i_head) {
		i_head++;
		if (i_head == MAX_INPUT) {
			i_head = 0;
		}
	}
}

int check_input(event_t *event) {
	int is_event = 0;
	event_t new_event;

	// Add any pending key presses to event queue
	while (gb_scan_head != gb_scan_tail) {
		new_event.type = KEY;
		new_event.data1 = gb_scan_q[gb_scan_head];

		// Ignore key from second keypad
		if (new_event.data1 == 0xE0) {
			gb_scan_head++;
			continue;
		}

		gb_scan_head++;

		if (new_event.data1 & KEY_UP_MASK) {
			new_event.sub_type = KEY_UP;
		} else {
			new_event.sub_type = KEY_DOWN;
		}

		new_event.data1 &= KEY_ALL_MASK; // Remove high bit
		add_input(&new_event);
	}

	// Set `event` to oldest pending event
	if (i_head != i_tail) {
		is_event = 1;

		event->type = input_queue[i_head].type;
		event->sub_type = input_queue[i_head].sub_type;
		event->x = input_queue[i_head].x;
		event->y = input_queue[i_head].y;
		event->data1 = input_queue[i_head].data1;
		event->data2 = input_queue[i_head].data2;

		i_head++;
		if (i_head == MAX_INPUT) {
			i_head = 0;
		}
	}

	return is_event;
}

void interrupt get_scan() {
	asm cli

	asm {
		in al, 060h // read scan code
		mov gb_scan, al
		in al, 061h // read kb status
		mov bl, al
		or al, 080h
		out 061h, al // set bit 7 and write
		mov al, bl
		out 061h, al // write again, bit 7 clear

		mov al, 020h // reset PIC
		out 020h, al

		sti
	}

	*(gb_scan_q + gb_scan_tail) = gb_scan;
	++gb_scan_tail;
}

/* Save old int9 ISR vector and insert ours */
void init_keyboard() {
	byte far *bios_key_state;

	oldkb = getvect(9);

	// turn off num-lock and caps via BIOS
	bios_key_state = MK_FP(0x040, 0x017);
	*bios_key_state &= (~(32 | 64));

	oldkb(); // Call the old BIOS kb handler to update kb lights

	gb_scan_head = 0;
	gb_scan_tail = 0;
	gb_scan = 0;

	// Install our handler
	setvect(9, get_scan);
}

void deinit_keyboard() {
	setvect(9, oldkb);
}
