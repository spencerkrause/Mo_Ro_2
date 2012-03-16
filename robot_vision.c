/* Filename:  robot_vision.c
 * Author:	Based on robot_camera_example.c from API
 * 
 * Note: I added the cvOr() function to compute the new threshold, it definitely has improved our thresholded image.
 * 	 But the thresholded image still needs some work. I tried various combinations of HSV values but it didn't help that much.
 * 	 Also, I added a function (get_square_diffence()) to calculate the difference in distance between the biggest two squares 
 * 	and the center vertical line. 
 * 
 * A side note: I found that the resolution/quality of the cameras varies with different robots. I have been using Rosie.
 */

#include "robot_if.h"
#include "robot_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//compute the difference in distance between the biggest two squares and the center vertical line
void get_square_diffence(squares_t *square1, squares_t *square2, IplImage *img){
	CvPoint pt1, pt2;
	int dist_to_center1, dist_to_center2, diff;
	
	pt1 = square1->center;
	pt2 = square2->center;
	
	dist_to_center1 = pt1.x - img->width/2;
	dist_to_center2 = pt2.x - img->width/2;
	
	//negative diff means robot pointing to the right of the origin, positive diff means robot pointing to the left of the origin
	diff = dist_to_center1 + dist_to_center2;
	
	printf("square 1 distance = %d, square 2 distance = %d, difference in distance = %d\n", 
	       dist_to_center1, dist_to_center2, diff);
}

// Draw an X marker on the image
int isPair(squares_t *square1, squares_t *square2, float area_ratio_threshold){//set thresh around .5
  //code me -> compare areas
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

float getRatio(int x, int y) {  // x>y
  float ratio = (float)x / (float)y;
  //printf("Ratio of biggest to next biggest = %f\n", ratio);
  return ratio;
}

int main(int argv, char **argc) {
	robot_if_t ri;
	int major, minor;
	IplImage *image = NULL, *hsv = NULL, *threshold_1 = NULL, *threshold_2 = NULL, *final_threshold = NULL;
	squares_t *squares, *biggest_1, *biggest_2, *sq_idx;
	
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

		// Loop over the squares and find the biggest one
		sq_idx = squares;
		
		if(sq_idx != NULL) {
			printAreas(sq_idx);
			sq_idx = squares;
			biggest_1 = squares;
			
			if(sq_idx->next != NULL) {
				//printf("Finding the two largest\n");
			
				//iterate through and find the largest square by area. 
				while(sq_idx != NULL) {
					if(sq_idx->area > biggest_1->area)
						biggest_1 = sq_idx;
					sq_idx = sq_idx->next;
				}
				
				// Iterate through a second time to find second largest
				sq_idx = squares;
				
				if(biggest_1 != squares)
				  biggest_2 = squares;
				else if(squares->next!=NULL)
				  biggest_2 = squares->next;
				else{
				  printf("Only one square in list.  Getting out of here.\n");
				  break;
				}
				while(sq_idx != NULL) {
					
					if((sq_idx->area > biggest_2->area) && sq_idx != biggest_1 && 
						//make sure the same square doesn't drawn twice
						sq_idx->center.x != biggest_1->center.x && sq_idx->center.y != biggest_1->center.y)
						biggest_2 = sq_idx;
					sq_idx = sq_idx->next;
				}
			}
		}
		
		//printf("Drawing the two largest\n");
		// Only draw if we have 2 biggest squares
		if(biggest_1 != NULL){
			draw_green_X(biggest_1, image);
			printf("Area 1 = %d", biggest_1->area);
		}
		if(biggest_1 != NULL && biggest_2 != NULL ) {
			draw_red_X(biggest_2, image);
			printf("\tArea 2 = %d\n", biggest_2->area);
			get_square_diffence(biggest_1, biggest_2, image);
		}
		else if (biggest_1 != NULL){
			printf("\n");
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
		getc(stdin);
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
