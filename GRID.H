#ifndef GRID_H
#define GRID_H

struct grid_cell {
	int solid;
};

/* Contains the screen grid of
   width*height cells containing
   cell_size*cell_size pixels */
struct grid {
	int width;
	int height;
	int cell_size;
	float gravity;
	float friction;
	struct grid_cell **cells; // 2D array
};

extern struct grid grid;

int init_grid(int width, int height, int cell_size);
void destroy_grid();
struct grid_cell *get_cell(int x, int y);

#endif
