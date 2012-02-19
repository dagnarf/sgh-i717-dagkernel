
#ifndef _SEC_CAM_DEV_H
#define _SEC_CAM_DEV_H



/* Saturation*/
enum cam_saturation_mode {
	SATURATION_MINUS_2 = 0,
	SATURATION_MINUS_1,
	SATURATION_DEFAULT,
	SATURATION_PLUS_1,
	SATURATION_PLUS_2,
};

/* Sharpness */
enum cam_sharpness_mode {
	SHARPNESS_MINUS_2 = 0,
	SHARPNESS_MINUS_1,
	SHARPNESS_DEFAULT,
	SHARPNESS_PLUS_1,
	SHARPNESS_PLUS_2,
};

/* Contrast */
enum cam_contrast_mode {
	CONTRAST_MINUS_2 = 0,
	CONTRAST_MINUS_1,
	CONTRAST_DEFAULT,
	CONTRAST_PLUS_1,
	CONTRAST_PLUS_2,
};

/* EV */
enum cam_ev{
	EV_MINUS_4 = 0,
	EV_MINUS_3,
	EV_MINUS_2,
	EV_MINUS_1,
	EV_DEFAULT,
	EV_PLUS_1,
	EV_PLUS_2,
	EV_PLUS_3,
	EV_PLUS_4,
};


/* BLUR  */
enum cam_blur
{
	BLUR_LEVEL_0 = 0,
	BLUR_LEVEL_1,
	BLUR_LEVEL_2,
	BLUR_LEVEL_3,
};



/* ISO */
// from camera.h
enum cam_iso_mode {
	ISO_AUTO = 0,
	ISO_50,
	ISO_100,
	ISO_200,
	ISO_400,
	ISO_800,
	ISO_1600,
};


/* White Balance */
// from camera.h
enum cam_wb_mode {
	WHITE_BALANCE_BASE = 0,
	WHITE_BALANCE_AUTO,		
	WHITE_BALANCE_CUSTOM,
	WHITE_BALANCE_INCANDESCENT,
	WHITE_BALANCE_FLUORESCENT,
	WHITE_BALANCE_SUNNY, // 5, DAYLIGHT
	WHITE_BALANCE_CLOUDY,
};

/* Exposure mode */
// from camera.h
enum cam_metering_mode {
	METERING_MATRIX = 0, //FRAME_AVERAGE
	METERING_CENTER,
	METERING_SPOT,
	METERING_MAX,
};

/* Scene Mode */ 
enum cam_scene_mode {
	SCENE_OFF = 1,
	SCENE_PORTRAIT,
	SCENE_NIGHTSHOT,
	SCENE_BACKLIGHT,
	SCENE_LANDSCAPE,
	SCENE_SPORTS,
	SCENE_PARTY,
	SCENE_BEACH,
	SCENE_SUNSET,
	SCENE_DAWN,
	SCENE_FALL,
	SCENE_FIREWORK,
	SCENE_TEXT,
	SCENE_CANDLE,
};

/* Effect  */ 
enum cam_effect_mode {
	EFFECT_OFF = 0,
	EFFECT_MONO,
	EFFECT_NEGATIVE,
	EFFECT_SEPIA = 4,
};

/* Focus  */ 
enum cam_focus_mode {
	FOCUS_AUTO = 0,
	FOCUS_MACRO,
};

// from msm_camera.h

#endif

