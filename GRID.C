#include <stdlib.h>

#include "GRID.H"

struct grid grid;

int init_grid(int width, int height, int cell_size) {
	int x, y;
	struct grid_cell current;

	grid.cells =
		(struct grid_cell**)malloc(height*sizeof(struct grid_cell*));

	if (grid.cells == NULL) {
		printf("%s", "Out of memory!");
		return 1;
	}

	grid.width = width;
	grid.height = height;
	grid.cell_size = cell_size;
	grid.friction = 0.4f;
	grid.gravity = 0.5f;

	for (y = 0; y < height; y++) {
		grid.cells[y] = (struct grid_cell*)malloc(width*sizeof(struct grid_cell));
		if (grid.cells[y] == NULL) {
			printf("%s", "Out of memory!");
			return 1;
		}

		if (y == height - 1) {
			for (x = 0; x < width; x++) {
				current.solid = 1;
				grid.cells[y][x] = current;
			}
		} else {
			for (x = 0; x < width; x++) {
				if (y == height - 2 &&
					x == 10) {
					current.solid = 1;
				} else {
					current.solid = 0;
				}
				grid.cells[y][x] = current;
			}
		}
	}

	return 0;
}

void destroy_grid() {
	int y;

	for (y = 0; y < grid.height; y++) {
		free(grid.cells[y]);
		grid.cells[y] = NULL;
	}

	free(grid.cells);
	grid.cells = NULL;
}


struct grid_cell *get_cell(int x, int y) {
	int cell_x, cell_y;

	cell_x = x / grid.cell_size;
	cell_y = y / grid.cell_size;
	return &grid.cells[cell_y][cell_x];
}
