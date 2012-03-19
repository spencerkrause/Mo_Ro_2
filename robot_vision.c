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
int get_diff_in_y(squares_t *square1, squares_t *square2){
	int y_1, y_2, diff;
	
	y_1 = square1->center.y;
	y_2 = square2->center.y;
	
	diff = abs(y_1 - y_2);
	printf("square_1 y = %d\t square_2 y = %d\tdifference in y = %d\n", y_1, y_2, diff);
	return diff;
}

// Draw an X marker on the image

float getRatio(int x, int y) {  // x>y
  float ratio = (float)x / (float)y;
  printf("Area ratio = %f\n", ratio);
  return ratio;
}

int isPair(squares_t *square1, squares_t *square2, float area_ratio_threshold){//set thresh around .5
  //compare areas
  float ratio;
  int diff;
  
  if((square1->area)<(square2->area))
    ratio = getRatio((int)square1->area, (int)square2->area);
  else if((square1->area)>(square2->area))
    ratio = getRatio((int)square2->area, (int)square1->area);
  else 
    ratio = 1;
  
  diff = get_diff_in_y(square1, square2);
  
  if(ratio > area_ratio_threshold && diff < 25)
      if(abs((square1->center.x) - (square2->center.x))>=50)//if they're not in the same place ie: the same square
	return 1;
      else
	return 0;
  else
    return 0;
}
void draw_green_X(squares_t *s, IplImage *img) {
	CvPoint pt1, pt2;
	int sq_amt = (int) (sqrt(s->area) / 2);	

	// Upper Left to Lower Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y - sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y + sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(0, 255, 0), 3, CV_AA, 0);

	// Lower Left to Upper Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y + sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y - sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(0, 255, 0), 3, CV_AA, 0);
}

void draw_red_X(squares_t *s, IplImage *img) {
	CvPoint pt1, pt2;
	int sq_amt = (int) (sqrt(s->area) / 2);	

	// Upper Left to Lower Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y - sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y + sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(255, 0, 0), 3, CV_AA, 0);

	// Lower Left to Upper Right
	pt1.x = s->center.x - sq_amt;
	pt1.y = s->center.y + sq_amt;
	pt2.x = s->center.x + sq_amt;
	pt2.y = s->center.y - sq_amt;
	cvLine(img, pt1, pt2, CV_RGB(255, 0, 0), 3, CV_AA, 0);
}

void draw_vertical_line(IplImage *img){
	CvPoint pt1, pt2;
	pt1.x = img->width/2;
	pt1.y = 0;
	pt2.x = img->width/2;
	pt2.y = img->height;
	
	cvLine(img, pt1, pt2, CV_RGB(0, 60, 255), 3, CV_AA, 0);
}
void printAreas(squares_t *squares) {
       printf("Areas of squares: \n");
       while(squares != NULL) {
               printf("  %d\n", squares->area);
               squares = squares->next;
       }
}

int main(int argv, char **argc) {
	robot_if_t ri;
	int major, minor, x_dist_diff, square_count, prev_square_area_1 = 0, prev_square_area_2 = 0;
	IplImage *image = NULL, *hsv = NULL, *threshold_1 = NULL, *threshold_2 = NULL, *final_threshold = NULL;
	squares_t *squares, *biggest_1, *biggest_2, , *pair_square_1, *pair_square_2, *sq_idx;
	bool same_square;
	bool hasPair = 0;
	
	square_count = 0;
	
	// Make sure we have a valid command line argument
	if(argv <= 1) {
		printf("Usage: robot_test <address of robot>\n");	
		exit(-1);
	}

	ri_api_version(&major, &minor);
	printf("Robot API Test: API Version v%i.%i\n", major, minor);

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
	cvNamedWindow("Biggest Square", CV_WINDOW_AUTOSIZE);
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

		// Get the current camera image and display it
		if(ri_get_image(&ri, image) != RI_RESP_SUCCESS) {
			printf("Unable to capture an image!\n");
			continue;
		}
		//cvShowImage("Rovio Camera", image);

		// Convert the image from RGB to HSV
		cvCvtColor(image, hsv, CV_BGR2HSV);

		// Pick out the first range of pink color from the image
		cvInRangeS(hsv, RC_PINK_LOW_1, RC_PINK_HIGH_1, threshold_1);
		
		// Pick out the second range of pink color from the image
		cvInRangeS(hsv, RC_PINK_LOW_2, RC_PINK_HIGH_2, threshold_2);
		
		// compute the final threshold image by using cvOr
		cvOr(threshold_1, threshold_2, final_threshold, NULL);
		

		cvShowImage("Thresholded", final_threshold);
		
		// Find the squares in the image
		squares = ri_find_squares(final_threshold, RI_DEFAULT_SQUARE_SIZE);
		
		if( squares != NULL ) {
			printf("Sorting squares!\n");
			sort_squares(squares);
			printf("Sort Complete!\n");
			printAreas(squares);
			printf("Done printing");
		
			//find biggest pair (if it exists)
			sq_idx = squares;
			
			while(sq_idx != NULL){
				if(sq_idx->next == NULL) break;
				else if(isPair(sq_idx, sq_idx->next, 0.75)){
					hasPair = 1;
					break;
				}
				sq_idx = sq_idx->next;
			}
		
			printf("Pair ID complete!\n");
			
			if(hasPair){
				printf("Pair found.\n");
				//draw_green_X(sq_idx, image);
				//draw_green_X(sq_idx->next, image);
				biggest_1 = sq_idx;
				biggest_2 = sq_idx->next;
				
				
				
			}
			else {
				printf("Pair not found.  Marking largest.\n");
				draw_red_X(squares, image);
				
				//temporary:
				biggest_1 = squares;
				biggest_2 = squares;
			}
			hasPair = 0;
		}
		else {
			printf("No squares found.\n");
		}
		
		hasPair = 0;
		
		if(biggest_1 != NULL){
			draw_green_X(biggest_1, image);
			printf("Area 1 = %d", biggest_1->area);
		}
		
		//we only see the last pair of squares, go straight ahead and make a 90 degree right turn
		if (square_count == 3){	
			ri_move(&ri, RI_MOVE_FORWARD, 1);
			if (ri_IR_Detected(&ri)) {
				square_count++;
				printf("Object detected, square_count = %d\n", square_count);
			}		
	
		}
		else if(square_count == 4){
			printf("Rotating\n");
			
			if (biggest_1 != NULL && biggest_2 != NULL && (biggest_1->area - biggest_2->area) < 500){
				square_count++;
				printf("New Path Found\n");
			}
			ri_move(&ri, RI_TURN_RIGHT, 7); 
			
		}
		else{
			/*
			 * 	If we only find a single usable largest square:
			 * 		if square is on left of screen, turn right, strafe right
			 * 		if square is on right of screen, turn left, strafe left
			 */	
			
			
			
			if(biggest_1 != NULL && biggest_2 != NULL ) {
				draw_red_X(biggest_2, image);
				printf("\tArea 2 = %d\n", biggest_2->area);
				
				//get the difference in distance between the two biggest squares and the center vertical line
				x_dist_diff = get_square_diffence(biggest_1, biggest_2, image);
				get_diff_in_y(biggest_1, biggest_2);
				
				//when the camera can't detect the other biggest square, which means now the second biggest square
				//is much smaller than the first biggest square
				if ((biggest_1->area - biggest_2->area) > 500){
					//if both squares are at the left side of the center line
					if (biggest_1->center.x < image->width/2 && biggest_2->center.x < image->width/2){
						printf("rotate right at speed = 6\n");
						ri_move(&ri, RI_TURN_RIGHT, 6); 
					}
					//if both squares are at the right side of the center line
					else if (biggest_1->center.x > image->width/2 && biggest_2->center.x > image->width/2){
						printf("rotate left at speed = 6\n");
						ri_move(&ri, RI_TURN_LEFT, 6); 
					}
					//if the center line is in the middle of the two biggest squares
					else if (biggest_1->center.x < image->width/2 && biggest_2->center.x > image->width/2 ){
						printf("rotate right at speed = 2\n");
						ri_move(&ri, RI_TURN_RIGHT, 2); 
						
					}
					else{
						printf("rotate left at speed = 2\n");
						ri_move(&ri, RI_TURN_LEFT, 2); 
					}
					
				}
				else{
					if (prev_square_area_1 != 0 && prev_square_area_2 != 0 && 
						biggest_1->area < prev_square_area_1  && biggest_2->area < prev_square_area_2 ){
						square_count++;
						printf("square count = %d\n", square_count);
					}
					//rotate to the left
					if (x_dist_diff < -40){
						printf("rotate left at speed = 6\n");
						ri_move(&ri, RI_TURN_LEFT, 6); 
					}
					//rotate to the right
					else if (x_dist_diff > 40){
						printf("rotate right at speed = 6\n");
						ri_move(&ri, RI_TURN_RIGHT, 6);
					}
					prev_square_area_1 = biggest_1->area;
					prev_square_area_2 = biggest_2->area;
					
				}
				ri_move(&ri, RI_MOVE_FORWARD, 5);
			}
			//once the camera can't detect any squares, make the robot go backwards
			else if (biggest_1 == NULL && biggest_2 == NULL){
				printf("Move Backwards\n");
				ri_move(&ri, RI_MOVE_BACKWARD , 1); 
			}
		}

		// display a straight vertical line
		draw_vertical_line(image);
		
		// Display the image with the drawing oon ito
		cvShowImage("Biggest Square", image);
		
		// Update the UI (10ms wait)
		cvWaitKey(10);
	
		// Release the square data
		while(squares != NULL) {
			
			sq_idx = squares->next;
			free(squares);
			squares = sq_idx;	
		}
		biggest_1 = NULL;
		biggest_2 = NULL;

		// Move forward unless there's something in front of the robot
		/*if(!ri_IR_Detected(&ri))
			ri_move(&ri, RI_MOVE_FORWARD, RI_SLOWEST);*/
		//printf("Loop Complete\n");
		//getc(stdin);
	} while(1);

	// Clean up (although we'll never get here...)
	//cvDestroyWindow("Rovio Camera");
	cvDestroyWindow("Biggest Square");
	cvDestroyWindow("Thresholded");
	
	// Free the images
	cvReleaseImage(&threshold_1);
	cvReleaseImage(&threshold_2);
	cvReleaseImage(&final_threshold);
	cvReleaseImage(&hsv);
	cvReleaseImage(&image);

	return 0;
}
