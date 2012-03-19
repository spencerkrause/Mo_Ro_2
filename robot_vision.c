/* Filename:  robot_vision.c
 * Author:	Based on robot_camera_example.c from API
 * 
 * 05-16: I added the cvOr() function to compute the new threshold, it definitely has improved our thresholded image.
 * 	 But the thresholded image still needs some work. I tried various combinations of HSV values but it didn't help that much.
 * 	 Also, I added a function (get_square_diffence()) to calculate the difference in distance between the biggest two squares 
 * 	and the center vertical line. 
 * 
 * A side note: I found that the resolution/quality of the cameras varies with different robots. Please use "Optimus"
 * 
 * 05-17: I tried several different robots and found that Optimus had the best threshold image for the current HSV values.
 *	Also, I added a routine to move the robot through the maze.  It actually works pretty well 50% of the times, until 
 *      the end where the robot has to make a right turn.  Because I didn't implement anything to keep track the distance 
 *      the robot has gone, so it doesn't know when to make that 90 degrees right turn yet.  
 * 
 * (Lines 267- 305 are the movement stuff)
 * 
 */

#include "robot_if.h"
#include "robot_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sort_squares(squares_t *squares) {
	squares_t *sq_idx, 
	          *counter;
	int temp;
	
	if (squares == NULL) {
		printf("List does not exist\n\n");
		return;
	}
	
	sq_idx = squares;
	for(; sq_idx->next != NULL; sq_idx = sq_idx->next)
	{
		for(counter = sq_idx->next; counter != NULL; counter = counter->next)
		{
			if(sq_idx->area < counter->area)
			{
				/* swap areas */
				temp = sq_idx->area;
				sq_idx->area = counter->area;
				counter->area = temp;
				
				/* swap center.x */
				temp = sq_idx->center.x;
				sq_idx->center.x = counter->center.x;
				counter->center.x = temp;
				
				/* swap center.y */
				temp = sq_idx->center.y;
				sq_idx->center.y = counter->center.y;
				counter->center.y = temp;
			}
		}
	}  
}

//compute the difference in distance between the biggest two squares and the center vertical line
int get_square_diffence(squares_t *square1, squares_t *square2, IplImage *img){
	CvPoint pt1, pt2;
	int dist_to_center1, dist_to_center2, diff;
	
	pt1 = square1->center;
	pt2 = square2->center;
	
	dist_to_center1 = pt1.x - img->width/2;
	dist_to_center2 = pt2.x - img->width/2;
	
	//negative diff means robot pointing to the right of the origin, positive diff means robot pointing to the left of the origin
	diff = dist_to_center1 + dist_to_center2;
	
	/*printf("square 1 distance = %d\t square 2 distance = %d\t difference in distance = %d\n", 
	       dist_to_center1, dist_to_center2, diff);
	*/
	return diff;
}

//checks if the same square gets marked twice
bool is_same_square(squares_t *square1, squares_t *square2){
	CvPoint pt1, pt2;
	int x_diff, y_diff;
	pt1 = square1->center;
	pt2 = square2->center;
	
	x_diff = abs(pt1.x - pt2.x);
	y_diff = abs(pt1.y - pt2.y);
	
	if ((0 <= x_diff && x_diff <= 3) && (0 <= y_diff && y_diff <= 3))return true;
	   
	return false;
}

/* Check the differnce in y between two squares */
int get_diff_in_y(squares_t *square1, squares_t *square2){
	int y_1, y_2, diff;
	
	y_1 = square1->center.y;
	y_2 = square2->center.y;
	
	diff = abs(y_1 - y_2);
	//printf("square_1 y = %d\t square_2 y = %d\tdifference in y = %d\n", y_1, y_2, diff);
	return diff;
}

float getRatio(int x, int y) {  // x>y
	float r;
	
	if( x < y ) 	r = (float) x / (float) y;
	else if(x > y)	r = (float) y / (float) x;
	else 		r = 1.0;
		
	//printf("Area ratio = %f\n", r);
	return r;
}

int isPair(squares_t *square1, squares_t *square2, float area_ratio_threshold){//set thresh around .75
	//compare areas
	float ratio;
	int diff;
	bool same_square;
	
	ratio = getRatio(square1->area, square2->area);
	
	diff = get_diff_in_y(square1, square2);
	
	same_square = is_same_square(square1, square2);
	
	/* if ratio greater than threshold, the difference in y is small, and they are NOT the same square */
	if( (ratio > area_ratio_threshold ) && ( diff < 25 ) && !same_square )	return 1;
	else									return 0;
}

void draw_X(squares_t *s, IplImage *img, int R, int G, int B) {
	CvPoint pt1, pt2;
	int sq_amt = (int) (sqrt(s->area) / 2);	

	// Upper Left to Lower Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y - sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y + sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(R, G, B), 3, CV_AA, 0);

	// Lower Left to Upper Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y + sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y - sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(R, G, B), 3, CV_AA, 0);
}

void draw_vertical_line(IplImage *img){
	CvPoint pt1, pt2;
	pt1.x = img->width/2;
	pt1.y = 0;
	pt2.x = img->width/2;
	pt2.y = img->height;
	
	cvLine(img, pt1, pt2, CV_RGB(0, 60, 255), 3, CV_AA, 0);
}

/* print out list of square areas */
void printAreas(squares_t *squares) {
	squares_t *sq_idx;
	printf("Areas of squares: \n");
	
	sq_idx = squares;
	while(sq_idx != NULL) {
               printf("  %d\n", sq_idx->area);
               sq_idx = sq_idx->next;
       }
}

int main(int argv, char **argc) {
	robot_if_t ri;
	int 	x_dist_diff, 
		square_count = 0, 
		prev_square_area_1 = 0, 
		prev_square_area_2 = 0;
	IplImage *image = NULL, 
		*hsv = NULL, 
		*threshold_1 = NULL, 
		*threshold_2 = NULL, 
		*final_threshold = NULL;
	squares_t *squares, 
		*largest,
		*next_largest,
		*pair_square_1, 
		*pair_square_2, 
		*sq_idx;
	bool 	hasPair = 0,
		onlyLargest = 0,
		twoLargest = 0;
	
	// Make sure we have a valid command line argument
	if(argv <= 1) {
		printf("Usage: robot_test <address of robot>\n");	
		exit(-1);
	}

	// Setup the robot with the address passed in
	if(ri_setup(&ri, argc[1], 0)) {
		printf("Failed to setup the robot!\n");
		exit(-1);
	}

	// Setup the camera
	if(ri_cfg_camera(&ri, RI_CAMERA_DEFAULT_BRIGHTNESS, RI_CAMERA_DEFAULT_CONTRAST, 5, RI_CAMERA_RES_640, RI_CAMERA_QUALITY_LOW)) {
		printf("Failed to configure the camera!\n");
		exit(-1);
	}

	// Create a window to display the output
	//cvNamedWindow("Rovio Camera", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Square Display", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Thresholded", CV_WINDOW_AUTOSIZE);

	// Create an image to store the image from the camera
	image = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

	// Create an image to store the HSV version in
	// We configured the camera for 640x480 above, so use that size here
	hsv = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

	// And an image for each thresholded version
	threshold_1 = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);
	threshold_2 = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);
	final_threshold = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);

	// Move the head up to the middle position
	ri_move(&ri, RI_HEAD_MIDDLE, RI_FASTEST);
	
	// Action loop
	do {
		// Update the robot's sensor information
		if(ri_update(&ri) != RI_RESP_SUCCESS) {
			printf("Failed to update sensor information!\n");
			continue;
		}

		// Get the current camera image
		if(ri_get_image(&ri, image) != RI_RESP_SUCCESS) {
			printf("Unable to capture an image!\n");
			continue;
		}
		/* for testing */
		//cvShowImage("Rovio Camera", image);

		// Convert the image from RGB to HSV
		cvCvtColor(image, hsv, CV_BGR2HSV);

		// Pick out the first range of pink color from the image
		cvInRangeS(hsv, RC_PINK_LOW_1, RC_PINK_HIGH_1, threshold_1);
		
		// Pick out the second range of pink color from the image
		cvInRangeS(hsv, RC_PINK_LOW_2, RC_PINK_HIGH_2, threshold_2);
		
		// compute the final threshold image by using cvOr
		cvOr(threshold_1, threshold_2, final_threshold, NULL);
		
		/* show final thresholded image for testing */
		cvShowImage("Thresholded", final_threshold);
		
		// Find the squares in the image
		squares = ri_find_squares(final_threshold, RI_DEFAULT_SQUARE_SIZE);
		
		/* If any squares are found */
		if( squares != NULL ) {
			/* sort squares from largest to smallest */
			sort_squares(squares);
			
			//printAreas(squares);
			
			//find largest useful pair (if they exist)
			sq_idx = squares;
			
			while(sq_idx != NULL){
				if(sq_idx->next == NULL) break;
				else if(isPair(sq_idx, sq_idx->next, 0.75)){
					hasPair = 1;
					break;
				}
				sq_idx = sq_idx->next;
			}
		
			/* if pair is found, mark them for later use */	
			if(hasPair){
				//printf("Pair found.\n");
				
				pair_square_1 = sq_idx;
				pair_square_2 = sq_idx->next;
				
				draw_X(pair_square_1, image, 0, 255, 0);
				draw_X(pair_square_2, image, 100, 255, 100);
			}
			else /* otherwise, mark the largest squares found */
			{
				largest = squares;
				onlyLargest = 1;
				draw_X(largest, image, 255, 0, 0);
				
				sq_idx = squares;
				
				while(sq_idx != NULL){
					if(sq_idx->next == NULL) break;
					else if(!is_same_square(sq_idx, sq_idx->next) ){
						break;
					}
					sq_idx = sq_idx->next;
				}
				
				if(sq_idx->next != NULL) {
					next_largest = sq_idx->next;
					draw_X(next_largest, image, 255, 100, 100);
					
					onlyLargest = 0;
					twoLargest = 1;
					//printf ("Two Largest Found.\n");
				}
				//else printf ("Only Largest Found.\n");
			}
		}
		
		//we only see the last pair of squares, go straight ahead and make a 90 degree right turn
		
		if(square_count == 4){
			printf("Rotating\n");
			
			if (hasPair){
				square_count++;
				printf("New Path Found\n");
			}
			ri_move(&ri, RI_TURN_RIGHT, 7); 
			
		}
		else{
			if(hasPair) {
				printf("Has pair.  ");			
				//get the difference in distance between each square and the center vertical line
				x_dist_diff = get_square_diffence(pair_square_1, pair_square_2, image);
				
				if (prev_square_area_1 != 0 && prev_square_area_2 != 0 && 
					pair_square_1->area < prev_square_area_1  && pair_square_2->area < prev_square_area_2 ){
					square_count++;
					printf("square count changed.  Now equals %d\n", square_count);
				}
				//rotate to the left
				if (x_dist_diff < -40){
					printf("Diff < - 40.  rotate left at speed = 6\n");
					ri_move(&ri, RI_TURN_LEFT, 6);					
				}
				//rotate to the right
				else if (x_dist_diff > 40){
					printf("Diff > 40.  rotate right at speed = 6\n");
					ri_move(&ri, RI_TURN_RIGHT, 6);					
				}
				prev_square_area_1 = pair_square_1->area;
				prev_square_area_2 = pair_square_2->area;
				
				printf("Move forward speed = 5\n");				
				ri_move(&ri, RI_MOVE_FORWARD, 5);
			}
			else if(twoLargest) /* when pair not detected, second biggest square is smaller than the first biggest square */
			{
				if ((largest->area - next_largest->area) > 500){
					//if both squares are at the left side of the center line
					if (largest->center.x < image->width/2 && next_largest->center.x < image->width/2){
						printf("Both squares left of center line.  rotate right at speed = 6\n");
						ri_move(&ri, RI_TURN_RIGHT, 6);
						ri_move(&ri, RI_STOP, 1);
					}
					//if both squares are at the right side of the center line
					else if (largest->center.x > image->width/2 && next_largest->center.x > image->width/2){
						printf("Both squares right of center line.  rotate left at speed = 6\n");
						ri_move(&ri, RI_TURN_LEFT, 6); 
						ri_move(&ri, RI_STOP, 1);
					}
					//if the center line is in the middle of the two biggest squares
					else if (largest->center.x < image->width/2 && next_largest->center.x > image->width/2 ){
						printf("Center in middle of twoLargest on left.  rotate right at speed = 6\n");
						ri_move(&ri, RI_TURN_RIGHT, 4);
						ri_move(&ri, RI_STOP, 1);
					}
					else{
						printf("Center in middle of twoLargest on right.  rotate left at speed = 6\n");
						ri_move(&ri, RI_TURN_LEFT, 4);
						ri_move(&ri, RI_STOP, 1);
					}
					
				}
			}
			else if(onlyLargest) /* If we only find a single usable largest square */
			{
			 	//if both squares are at the left side of the center line
				if (largest->center.x < image->width/2){
					printf("Only Largest Found on left.  rotate right at speed = 6\n");
					ri_move(&ri, RI_TURN_RIGHT, 6);
					ri_move(&ri, RI_MOVE_FWD_RIGHT, 6);
					ri_move(&ri, RI_STOP, 1);
				}
				//if both squares are at the right side of the center line
				else if (largest->center.x > image->width/2){
					printf("Only Largest Found on right.  rotate left at speed = 6\n");
					ri_move(&ri, RI_TURN_LEFT, 6);
					ri_move(&ri, RI_MOVE_FWD_LEFT, 6);
					ri_move(&ri, RI_STOP, 1);
				}			 
			}
			else	/* once the camera can't detect any squares, make the robot go backwards */
			{
				if (square_count == 3){
					printf("Moving forward, count equals 3\n");
					ri_move(&ri, RI_MOVE_FORWARD, 5);
					if (ri_IR_Detected(&ri)) {
						square_count++;
						printf("Object detected, square_count = %d\n", square_count);
					}		
	
				}
				else {
					printf("No squares found.  Move Backwards\n");
					ri_move(&ri, RI_MOVE_BACKWARD , 1); 
				}
			}
		}

		// display a straight vertical line
		draw_vertical_line(image);
		
		// Display the image with the drawing oon ito
		cvShowImage("Square Display", image);
		
		// Update the UI (10ms wait)
		cvWaitKey(10);
	
		// Release the square data
		while(squares != NULL) 
		{
			sq_idx = squares->next;
			free(squares);
			squares = sq_idx;	
		}
		
		/* reset loop control variables */
		pair_square_1 = NULL;
		pair_square_2 = NULL;
		largest = NULL;
		next_largest = NULL;
		hasPair = 0;
		onlyLargest = 0;
		twoLargest = 0;

		printf("square count = %d\n\n", square_count);
		getc(stdin);
	} while(1);

	// Clean up (although we'll never get here...)
	//cvDestroyWindow("Rovio Camera");
	cvDestroyWindow("Square Display");
	cvDestroyWindow("Thresholded");
	
	// Free the images
	cvReleaseImage(&threshold_1);
	cvReleaseImage(&threshold_2);
	cvReleaseImage(&final_threshold);
	cvReleaseImage(&hsv);
	cvReleaseImage(&image);

	return 0;
}
