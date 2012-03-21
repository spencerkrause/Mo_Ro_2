#include "robot_if.h"
#include "robot_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sort_squares(squares_t *squares);
int get_square_diffence(squares_t *square1, squares_t *square2, IplImage *img);
bool is_same_square(squares_t *square1, squares_t *square2);
int get_diff_in_y(squares_t *square1, squares_t *square2);
float getRatio(int x, int y);
int isPair(squares_t *square1, squares_t *square2, float area_ratio_threshold);
void draw_green_X(squares_t *s, IplImage *img);
void draw_red_X(squares_t *s, IplImage *img);
void draw_vertical_line(IplImage *img);
void printAreas(squares_t *squares);

