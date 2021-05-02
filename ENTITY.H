#ifndef ENTITY_H
#define ENTITY_H
#include "TYPES.H"

enum entity_types {
	MOVER, STAT
};

struct entity {
	int id;
	int x;
	int y;
	float vel_x;
	float vel_y;
	int width;
	int height;
	int colour;
	int on_ground;
	byte far *behind;
	struct entity *next;
};

struct entity *create_entity(int id);
// Use when entity has changed width and height
int realloc_behind(struct entity *e);
struct entity *get_entity(int id);
struct entity *get_first_entity();
void del_entity(struct entity *e);
void destroy_entities();

void set_vel_x(struct entity *e, float v);
void set_vel_y(struct entity *e, float v);
void animate_entities();

#endif
