#include "DOS.H"
#include "TIMER.H"

volatile unsigned long fast_tick, slow_tick;

static void interrupt new_timer() {
	asm cli
	fast_tick++;

	if (!(fast_tick & 3)) { // Call old timer every 4th tick
		oldtimer();
		slow_tick++;
	} else {
		// reset PIC
		asm {
			mov al, 20h
			out 20h, al
		}
	}
	asm sti
}

void init_timer() {
	slow_tick = fast_tick = 0l;
	oldtimer = getvect(8); // save old timer

	asm cli

	// Speed up clock
	asm {
		mov bx, 19886 // clock speed 60Hz (1193180/60)
		mov al, 00110110b
		out 43h, al
		mov al, bl
		out 40h, al
		mov al, bh
		out 40h, al
	}

	setvect(8, new_timer);

	asm sti
}

void deinit_timer() {
	asm cli

	// slow down clock 1193180 / 65536 = 18.2, but we use zero
	asm {
		xor bx, bx
		mov al, 00110110b
		out 43h, al
		mov al, bl
		out 40h, al
		mov al, bh
		out 40h, al
	}

	setvect(8, oldtimer); // restore oldtimer

	asm sti
}