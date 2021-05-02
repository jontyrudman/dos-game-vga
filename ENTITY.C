#include <ALLOC.H>
#include "MAIN.H"
#include "ENTITY.H"
#include "TIMER.H"
#include "GRID.H"

#define MAX_ENTITIES 10
#define ABS(x) (x<0?-x:x)

struct entity *head = NULL;
struct entity **indirect = NULL;
int length = 0;
double stop_tolerance = 0.1;

unsigned long last_animated = -1;

struct entity *create_entity(int id) {
	struct entity *e;

	if (length == MAX_ENTITIES)
		return NULL;

	e = (struct entity*)malloc(sizeof(struct entity));

	if (e == NULL) {
		printf("%s", "Out of memory!");
		return NULL;
	}

	e->id = id;
	e->x = 0;
	e->y = 0;
	e->vel_x = 0;
	e->vel_y = 0;
	e->width = 5;
	e->height = 5;
	e->colour = 3;
	e->on_ground = 0;
	// For the sprite patch behind the entity
	e->behind = (byte far*)farmalloc((e->width) * (e->height) + 4);
	if (e->behind == NULL) {
		printf("%s", "Out of memory!");
		return NULL;
	}
	e->next = head;
	head = e;
	length++;

	return e;
}

/* Reallocates the size of the sprite patch behind,
   e.g. if the width has changed */
int realloc_behind(struct entity *e) {
	farfree(e->behind);
	e->behind = (byte far*)farmalloc((e->width) * (e->height) + 4);
	if (e->behind == NULL) {
		printf("%s", "Out of memory!");
		sleep(3);
		return 1;
	}
	return 0;
}

struct entity *get_entity(int id) {
	while ((*indirect) != NULL) {
		if ((*indirect)->id == id)
			return (*indirect);
	}

	return NULL;
}

struct entity *get_first_entity() {
	return head;
}

void del_entity(struct entity *e) {
	indirect = &head;
	while ((*indirect) != e)
		indirect = &(*indirect)->next;

	*indirect = e->next;
	free(e);
}

void destroy_entities() {
	struct entity *temp;

	while (head != NULL) {
		temp = head;
		head = head->next;
		free(temp);
	}
}

void set_vel_x(struct entity *e, float v) {
	if (v == 0 || ABS(v) < stop_tolerance) {
		e->vel_x = 0;
	} else if (v < 0) {
		e->vel_x = MAX(v, -3);
	} else if (v > 0) {
		e->vel_x = MIN(v, 3);
	}
}

void set_vel_y(struct entity *e, float v) {
	if (v == 0 || ABS(v) < stop_tolerance) {
		e->vel_y = 0;
	} else if (v < 0) {
		e->vel_y = MAX(v, -5);
	} else if (v > 0) {
		e->vel_y = MIN(v, 5);
	}
}

void apply_gravity(struct entity *e) {
	if (e->on_ground)
		return;
	if (e->vel_y > 0) {
		set_vel_y(e, e->vel_y + grid.gravity);
	} else {
		e->vel_y += grid.gravity;
	}
}

void apply_friction(struct entity *e) {
	if (!e->on_ground)
		return;
	if (e->vel_x > 0) {
		if (e->vel_x - grid.friction < 0) {
			set_vel_x(e, 0);
		} else {
			set_vel_x(e, e->vel_x - grid.friction);
		}
	} else if (e->vel_x < 0) {
		if (e->vel_x + grid.friction > 0) {
			set_vel_x(e, 0);
		} else {
			set_vel_x(e, e->vel_x + grid.friction);
		}
	}
}

int round_to(int n, int m) {
	int a = (n / m) * m;
	int b = a + m;
	return (n - a > b - n) ? b : a;
}

void collide(struct entity *e) {
	vector_2d_int tl, tr, br, bl;
	tl.x = e->x;
	tl.y = e->y;

	tr.x = e->x + e->width - 1;
	tr.y = e->y;

	br.x = e->x + e->width - 1;
	br.y = e->y + e->height - 1;

	bl.x = e->x;
	bl.y = e->y + e->height - 1;

	// RIGHT
	if (e->vel_x > 0 &&
		(get_cell(tr.x+1, tr.y)->solid ||
		 get_cell(br.x+1, br.y)->solid ||
		 br.x >= screen_width))
	{
		e->vel_x = 0;
		e->x = round_to(e->x, grid.cell_size);
		br.x = e->x + e->width - 1;
		tr.x = e->x + e->width - 1;
		bl.x = e->x;
		tl.x = e->x;
	}

	// LEFT
	if (e->vel_x < 0 &&
		(get_cell(tl.x-1, tl.y)->solid ||
		 get_cell(bl.x-1, bl.y)->solid ||
		 e->x < 0))
	{
		e->vel_x = 0;
		e->x = round_to(e->x, grid.cell_size);
		br.x = e->x + e->width - 1;
		tr.x = e->x + e->width - 1;
		bl.x = e->x;
		tl.x = e->x;
	}

	// DOWN
	if (e->vel_y > 0 &&
		(get_cell(bl.x, bl.y+1)->solid ||
		 get_cell(br.x, br.y+1)->solid ||
		 e->y >= screen_height))
	{
		e->vel_y = 0;
		e->y = round_to(e->y, grid.cell_size);
		bl.y = e->y + e->height - 1;
		br.y = e->y + e->height - 1;
		tl.y = e->y;
		tr.y = e->y;
		e->on_ground = 1;
	}

	// UP
	if (e->vel_y < 0 &&
		(get_cell(tl.x, tl.y-1)->solid ||
		 get_cell(tr.x, tr.y-1)->solid ||
		 e->y < 0))
	{
		e->vel_y = 0;
		e->y = round_to(e->y, grid.cell_size);
		bl.y = e->y + e->height - 1;
		br.y = e->y + e->height - 1;
		tl.y = e->y;
		tr.y = e->y;
	}

	if (!(get_cell(bl.x, bl.y+1)->solid ||
		  get_cell(br.x, br.y+1)->solid)) {
		e->on_ground = 0;
	}
}

void move(struct entity *e, unsigned long delta) {
	//if (ABS(e->vel_x) < stop_tolerance)
	//	e->vel_x = 0;
	//if (ABS(e->vel_y) < stop_tolerance)
	//	e->vel_y = 0;
	e->x += e->vel_x * delta;
	e->y += e->vel_y * delta;
}

void animate_entities() {
	unsigned long delta;
	struct entity *e = head;

	if (last_animated == -1)
		last_animated = fast_tick;

	delta = fast_tick - last_animated;

	while (e != NULL) {
		apply_gravity(e);
		apply_friction(e);
		move(e, delta);
		collide(e);
		e = e->next;
	}

	last_animated = fast_tick;
}
