#ifndef __ROBOT_IF_H_
#define __ROBOT_IF_H_

#include <stdint.h>
#include <stdbool.h>

// OpenCV header
#ifndef __ROBOT_IF_CPP_H
	// If we're using the C++ wrapper, we want to include these there instead
	#include <opencv/cv.h>
	#include <opencv/highgui.h>
#endif

// Sockets
#include <sys/socket.h>
#include <arpa/inet.h>

/**************************************
 * Movement Definitions
 **************************************/
#define RI_STOP			0
#define RI_MOVE_FORWARD		1
#define RI_MOVE_BACKWARD	2
#define RI_MOVE_LEFT		3
#define RI_MOVE_RIGHT		4
#define RI_TURN_LEFT		5
#define RI_TURN_RIGHT		6

#define RI_MOVE_FWD_LEFT	7
#define RI_MOVE_FWD_RIGHT	8
#define RI_MOVE_BACK_LEFT	9
#define RI_MOVE_BACK_RIGHT	10

#define RI_TURN_LEFT_20DEG	17
#define RI_TURN_RIGHT_20DEG	18

#define RI_HEAD_UP		11
#define RI_HEAD_MIDDLE		13
#define RI_HEAD_DOWN		12

#define RI_FASTEST		1
#define RI_SLOWEST		10

/***************************************
 * Response Code Definitions 
 ***************************************/
#define RI_RESP_SUCCESS		0
#define RI_RESP_FAILURE		1
#define RI_RESP_BUSY		2
#define RI_RESP_NOT_IMPLEMENTED	3
#define RI_RESP_UNK_CGI		4
#define RI_RESP_NO_NS_SIGNAL	5

#define RI_RESP_PARAM_RANGE_ERR	22
#define RI_RESP_NO_PARAM	23

/***************************************
 * Sensor data description
 ***************************************/
typedef struct {
	unsigned int		length;
	unsigned int		unused;
	unsigned int 		left_wheel_dir;
	unsigned int 		left_wheel_enc_ticks;
	unsigned int		right_wheel_dir;
	unsigned int		right_wheel_enc_ticks;
	unsigned int		rear_wheel_dir;
	unsigned int		rear_wheel_enc_ticks;
	unsigned int		unused2;
	unsigned int		head_position;
	unsigned int		battery;
	unsigned int		status;
} ri_data_t;

// Wheel Direction
#define RI_WHEEL_MASK(x)	((x >> 1) & 1)
#define RI_WHEEL_FORWARD	0
#define RI_WHEEL_BACKWARD	1

// Wheel IDs
#define RI_WHEEL_LEFT	0
#define RI_WHEEL_RIGHT	1
#define RI_WHEEL_REAR	2

/* Status field in the sensor data */
#define RI_STATUS_LED		(1 << 0)
#define RI_STATUS_IR_POWER	(1 << 1)
#define RI_STATUS_IR_DETECTOR	(1 << 2)

/***************************************
 * Sensor report data description
 ***************************************/
typedef struct {
	/* Rovio location in relation to the room beacon */
	int			x, y; /* -32767 to 32768 */
	float			theta; /* -PI to PI */
	unsigned int		room_id; /* 0 = Home base, 1 - 9 other rooms */
	unsigned int		strength; /* Navigation signal strength: 0 - 65535: Strong > 47000, No Signal < 5000 */
	// Robot operational state
	unsigned int		state; /* RI_ROBOT_STATE below, current robot state */
	// Camera controls
	unsigned int		brightness; /* Brightness of the camera */
	unsigned int		cam_res; /* Camera Resolution */
	unsigned int		frame_rate; /* Camera frame rate */
	unsigned int		speaker_volume; /* Speaker volume */
	unsigned int		mic_volume; /* Microphone volume */
	// Wifi info
	unsigned int		wifi; /* Wifi signal strength */
	// Battery charge info
	unsigned int		battery; /* Battery level */
	unsigned int		charging; /* Battery charging */
	// Head position
	unsigned int		head_position; /* Head position */
} ri_report_t;

// Robot state values, describes the current action of the robot
#define RI_ROBOT_STATE_IDLE	0
#define RI_ROBOT_STATE_DRIVING	1
#define RI_ROBOT_STATE_DOCKING	2
#define RI_ROBOT_STATE_PATH	3
#define RI_ROBOT_STATE_REC_PATH	4

// Robot head positions
#define RI_ROBOT_HEAD_LOW	204
// MID is actually between 135 and 140
#define RI_ROBOT_HEAD_MID	137
#define RI_ROBOT_HEAD_HIGH	65

// Light Status
#define RI_LIGHT_OFF		0
#define RI_LIGHT_ON		1

// Nav Signal Strength
#define RI_ROBOT_NAV_SIGNAL_STRONG	3
#define RI_ROBOT_NAV_SIGNAL_MID		2
#define RI_ROBOT_NAV_SIGNAL_WEAK	1
#define RI_ROBOT_NAV_SIGNAL_NO_SIGNAL	0
// Wifi Signal Strength 
#define RI_ROBOT_WIFI_LOW		0
#define RI_ROBOT_WIFI_HIGH		254

// Battery level
// Robot turns itself off below this level
#define RI_ROBOT_BATTERY_OFF		100
// Robot tries to go home below this level
#define RI_ROBOT_BATTERY_HOME		106
// Robot is operating under normal conditions below this level
#define RI_ROBOT_BATTERY_MAX		127

/* Camera definitions */
#define RI_CAMERA_DEFAULT_BRIGHTNESS	0x28
#define RI_CAMERA_DEFAULT_CONTRAST	0x48

// 176x144
#define RI_CAMERA_RES_176		0
// 320x240
#define RI_CAMERA_RES_320		1
// 352x240
#define RI_CAMERA_RES_352		2
// 640x480
#define RI_CAMERA_RES_640		3

// Microphone volume
#define RI_CAMERA_MIC_LOW		0
#define RI_CAMERA_MIC_HIGH		31

// Speaker volume
#define RI_CAMERA_SPEAKER_LOW		0
#define RI_CAMERA_SPEAKER_HIGH		31

// Camera quality
#define RI_CAMERA_QUALITY_LOW		0		
#define RI_CAMERA_QUALITY_MID		1
#define RI_CAMERA_QUALITY_HIGH		2
#define RI_CAMERA_MAX_IMG_SIZE		(1024*1024)

// Camera I2C Addresses
#define RI_CAMERA_AGC_ADDR		0x14
#define RI_CAMERA_BRIGHTNESS_ADDR	0x55
#define RI_CAMERA_CONTRAST_ADDR		0x56
#define RI_CAMERA_NIGHTMODE_ADDR	0x3b

// Centroid data
typedef struct st squares_t;
struct st {
	CvPoint center;
	int area;
	squares_t *next;
};

// Default square size
#define RI_DEFAULT_SQUARE_SIZE			250

// Battery charging?
#define RI_BATTERY_CHARGING_THRESH	80

// Robot Games Server
#define DEFAULT_SERVER_NAME		"kujo.cs.pitt.edu"

// Map Coordinate Limits
#define MAP_MAX_X                       7
#define MAP_MAX_Y                       5

// Map Types
#define MAP_OBJ_EMPTY                   0x00    
#define MAP_OBJ_ROBOT_1                 0x01
#define MAP_OBJ_ROBOT_2                 0x02
#define MAP_OBJ_PELLET                  0x03
#define MAP_OBJ_POST                    0x04
#define MAP_OBJ_RESERVE_1               0x05
#define MAP_OBJ_RESERVE_2               0x06

// Map object structure
typedef struct map_obj map_obj_t;
struct map_obj {
	int x;
	int y;
	int type;
	int points;
	map_obj_t *next;
};

// API Decoder
#define MAP_OBJ_TYPE(t)                 ((t & 0xF0) >> 4)
#define MAP_OBJ_SCORE(s)                (s & 0xF)
#define MAP_OBJ_ENC(t,s)                (((t & 0x0F) << 4) | (s & 0xF))

// Error codes
#define SERR_NONE                       (0)
#define SERR_INVALID_MOVE               (-1)
#define SERR_INVALID_COORD              (-2)
#define SERR_INVALID_ROBOT              (-3)

// ******************************************************************
// * Robot user interface
// ******************************************************************

#define MAX_ADDR_LEN			128
typedef struct robot_if {
	char address[MAX_ADDR_LEN];
	char gs_address[MAX_ADDR_LEN];
	int sock;
	struct sockaddr_in server_addr;
	struct sockaddr_in game_server_addr;

	// Robot number
	int id;
	
	// Current robot encoder counts
	unsigned long long right_wheel_enc;
	unsigned long long left_wheel_enc;
	unsigned long long rear_wheel_enc;

	// Report caches, remember to update before calling any of the getters
	ri_report_t report;
	ri_data_t sensor;
} robot_if_t;

// Return the API version
void ri_api_version(int* major, int* minor);

// Setup the Robot Interface struct
int ri_setup(robot_if_t *ri, const char *address, int robot_id);

/* Set motor speeds / neck interface */
int ri_move(robot_if_t *ri, int movement, int speed);

// Get robot sensor data
// Now these are handled by the update function
// int ri_get_sensor_data(robot_if_t *ri, ri_data_t *sensor);
/* Robot status - Get information about the robot status (NS, wifi level, battery level, etc) */
// int ri_get_status(robot_if_t *ri, ri_report_t *status);

// Update the stored status
int ri_update(robot_if_t *ri);

// Resets the counters
void ri_reset_state(robot_if_t *ri);

/* Robot headlight */
int ri_headlight(robot_if_t *ri, int status);
int ri_IR(robot_if_t *ri, int status);

/* Force the robot to go home and dock */
int ri_go_home(robot_if_t *ri);

/* Setup the camera */
int ri_cfg_camera(robot_if_t *ri, int brightness, int contrast, int framerate, int resolution, int quality);
int ri_cfg_volume(robot_if_t *ri, int mic_volume, int speaker_volume); 

/* Get a jpeg image from the robot */
int ri_get_image(robot_if_t *ri, IplImage *image);

// Square detector
squares_t* ri_find_squares(IplImage* input, int threshold);
double ri_angle(CvPoint* pt1, CvPoint* pt2, CvPoint* pt0);

// Accessor functions
bool ri_IR_Detected(robot_if_t *ri);

// North Star / Reports
int ri_getX(robot_if_t *ri);
int ri_getY(robot_if_t *ri);
float ri_getTheta(robot_if_t *ri);
int ri_getNavStrength(robot_if_t *ri);
int ri_getNavStrengthRaw(robot_if_t *ri);
int ri_getWifiStrengthRaw(robot_if_t *ri);
int ri_getBattery(robot_if_t *ri);
int ri_getRoomID(robot_if_t *ri);
		
// Movement
int ri_getWheelDirection(robot_if_t *ri, int wheel);
int ri_getWheelEncoder(robot_if_t *ri, int wheel);
int ri_getWheelEncoderTotals(robot_if_t *ri, int wheel);
int ri_getHeadPosition(robot_if_t *ri);

/* Game API */
int ri_update_map(robot_if_t *ri, int x, int y);
int ri_reserve_map(robot_if_t *ri, int x, int y);
map_obj_t* ri_get_map(robot_if_t *ri, int *score1, int* score2);

#endif /* __ROBOT_IF_H_ */
