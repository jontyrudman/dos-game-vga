/* Game Playground                *
 * Compile using Large mem model. */

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <alloc.h>
#include "MAIN.H"
#include "TIMER.H"
#include "INPUT.H"
#include "TYPES.H"
#include "ENTITY.H"
#include "GRID.H"

#define NUM_COLORS 256
#define PALETTE_INDEX_ADDR 0x3C8
#define PALETTE_DATA_ADDR 0x3C9
#define VGA_256 0x13, 320, 200

#define VID_INT 0x10
#define INPUT_STATUS_1 0x3DA

byte old_vid_mode = 0x03;
byte far *VGA_SEG = MK_FP(0xa000, 0); // Make far pointer
byte far *screen, *offscreen;
int screen_width, screen_height;
unsigned int screen_size;

event_t input_event = {0, 0, 0, 0, 0x00, 0x00};

unsigned long cycle_start;
unsigned long cycle_delta;

struct entity *player;

void enter_mode(byte mode) {
	/* A union is like a struct.
	   union REGS is a register union.
	   h is for BYTEREGS and x is for WORDREGS. */
	union REGS in_reg, out_reg;

	in_reg.h.ah = 0x0f; // high byte for getting vid state
	int86(0x10, &in_reg, &out_reg); // send interrupt
	old_vid_mode = out_reg.h.al;

	in_reg.h.ah = 0x00; // high byte for set mode
	in_reg.h.al = mode; // low byte for mode
	int86(0x10, &in_reg, &out_reg);
}

void exit_mode() {
	union REGS in_reg;

	in_reg.h.ah = 0x00;
	in_reg.h.al = old_vid_mode;
	int86(0x10, &in_reg, &in_reg);
}

/* Uses double buffering. Page flipping impossible
   on old VGA cards (unless using "Mode X"...)*/
int init_video_mode(byte mode, int width, int height) {
	screen_width = width;
	screen_height = height;
	screen_size = screen_width * screen_height;

	// Allocate 64KB for the screen buffer
	offscreen = farmalloc(screen_size); // u means const unsigned int

	if (offscreen) {
		screen = VGA_SEG;
		enter_mode(mode);
		_fmemset(offscreen, 0, screen_size); // far memset
		return 0;
	}

	// No memory! Return error.
	exit_mode();
	printf("Out of memory!\n");
	return 1;
}

void deinit_video_mode() {
	exit_mode();
	farfree(offscreen);
}

void update_buffer() {
	/* Wait for the screen to be displaying info
	   so we get a full vert retrace to copy in. */
	while (inportb(INPUT_STATUS_1) & 8) // Wait during vert retrace
		;
	// Wait for screen to start vert retrace
	while (!(inportb(INPUT_STATUS_1) & 8)) // Wait during info disp
		;

	// Copy our screen over to vid mem
	_fmemcpy(screen, offscreen, screen_size);
}

void draw_pix(int x, int y, byte c) {
	*(offscreen + x + y * screen_width) = c;
}

byte get_pix(int x, int y) {
	return *(offscreen + x + y * screen_width);
}

/* Get a patch of offscreen and save it as a
   sprite. spr must be allocated width*height + 4. */
void get_sprite(byte far *spr, int x, int y, int width, int height) {
	byte far *p;

	_fmemcpy(spr, &width, 2);
	spr += 2;
	_fmemcpy(spr, &height, 2);
	spr += 2;

	p = offscreen + y*(screen_width) + x;
	while (height--) {
		_fmemcpy(spr, p, width);
		spr += width;
		p += screen_width;
	}
}

void paint_screen(byte colour) {
	_fmemset(offscreen, colour, screen_size);
}

void horz_line(int x, int y, int len, byte colour) {
	byte far *p;

	p = offscreen + x + y * screen_width;
	_fmemset(p, colour, len);
}

void vert_line(int x, int y, int len, byte colour) {
	byte far *p;

	p = offscreen + x + y * screen_width;
	while (len--) {
		*p = colour;
		p += screen_width;
	}
}

void rect_fill(int x, int y, int width, int height, byte colour) {
	int end = y + height;

	for ( ; y < end; y++) {
		horz_line(x, y, width, colour);
	}
}

void draw_entity(struct entity *e) {
	rect_fill(e->x, e->y, e->width, e->height, e->colour);
}

void draw_entities() {
	struct entity *head = get_first_entity();

	while (head != NULL) {
		draw_entity(head);
		head = head->next;
	}
}

void save_bg() {
	struct entity *head = get_first_entity();

	while (head != NULL) {
		get_sprite(head->behind, head->x, head->y, head->width, head->height);
		head = head->next;
	}
}

void restore_bg() {
	byte far *p;
	byte far *behind;
	int width, height;
	struct entity *head = get_first_entity();

	while (head != NULL) {
		p = offscreen + head->x + head->y * screen_width;
		behind = head->behind;

		_fmemcpy(&width, behind, 2);
		behind += 2;
		_fmemcpy(&height, behind, 2);
		behind += 2;
		while (height--) {
			_fmemcpy(p, behind, width);
			behind += width;
			p += screen_width;
		}

		head = head->next;
	}
}

void draw_grid() {
	int x, y;

	for (y = 0; y < grid.height; y++) {
		for (x = 0; x < grid.width; x++) {
			rect_fill(x*grid.cell_size, y*grid.cell_size,
					  grid.cell_size, grid.cell_size,
					  grid.cells[y][x].solid);
		}
	}
}

void cleanup() {
	deinit_video_mode();
	destroy_grid();
	destroy_entities();
	deinit_timer();
	deinit_keyboard();
}

void reset_event(event_t *event) {
	event->type = 0;
	event->sub_type = 0;
	event->x = 0;
	event->y = 0;
	event->data1 = 0x00;
	event->data2 = 0x00;
}

int cycle() {
	static int kc;
	static int kc_down;
	static float vx_add, vy_add;
	cycle_start = fast_tick;

	if (vx_add == NULL)
		vx_add = 0;
	if (vy_add == NULL)
		vy_add = 0;

	kc = 0;
	kc_down = 0;
	if (check_input(&input_event)) {
		if (input_event.type == KEY) {
			kc = input_event.data1;
			if (input_event.sub_type == KEY_DOWN)
				kc_down = 1;
		}
	}

	switch (kc) {
		case UP:
			if (player->on_ground) {
				player->vel_y = -6 * kc_down;
				player->on_ground = 0;
			}
			break;
		case LEFT:
			vx_add = -0.7f * kc_down;
			break;
		case RIGHT:
			vx_add = 0.7f * kc_down;
			break;
		case ESC:
			return 0;
		default:
			break;
	}

	if (
		(player->vel_x > 0 && vx_add < 0)
		|| (player->vel_x < 0 && vx_add > 0)
	) {
		player->vel_x = 0;
	}
	set_vel_x(player, player->vel_x + vx_add);
	animate_entities();

	save_bg();
	draw_entities();
	update_buffer();
	restore_bg();

	cycle_delta = fast_tick - cycle_start;

	return 1;
}

int init_player() {
	player = create_entity(0);
	if (player == NULL)
		return 1;
	player->x = 300;
	player->y = 0;
	player->width = 10;
	player->height = 10;
	if (realloc_behind(player))
		return 1;
	get_sprite(player->behind, player->x, player->y,
			   player->width, player->height);

	return 0;
}

int init() {
	init_timer();
	init_keyboard();
	if (init_video_mode(VGA_256))
		return 1;
	if (init_grid(32, 20, 10)) {
		deinit_video_mode();
		return 1;
	}
	draw_grid();
	if (init_player()) {
		deinit_video_mode();
		destroy_grid();
		destroy_entities();
		return 1;
	}

	return 0;
}

int main() {
	if (init()) {
		// Show any message for a while and quit.
		sleep(3);
		return 1;
	}

	// Loop until ESC
	while (cycle())
		;

	cleanup();
	return 0;
}
