#ifndef INPUT_H
#define INPUT_H

#include "TYPES.H"

enum keys {
	UP = 0x48,
	DOWN = 0x50,
	LEFT = 0x4B,
	RIGHT = 0x4D,
	ESC = 0x01
};

enum in_type { KEY };
enum in_sub_type { KEY_UP, KEY_DOWN };
typedef struct {
	enum in_type type;
	enum in_sub_type sub_type;
	int x;
	int y;
	byte data1;
	byte data2;
} event_t;

void add_input(event_t *event);
int check_input(event_t *event);

void init_keyboard();
void deinit_keyboard();

#endif