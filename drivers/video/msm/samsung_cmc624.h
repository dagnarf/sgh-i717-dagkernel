/*
 * =====================================================================================
 *
 *       Filename:  p8lte_cmc624.h
 *
 *    Description:  LCDC p8lte define header file.
 *                  LVDS define
 *                  CMC624 Imaging converter driver define
 *
 *        Version:  1.0
 *        Created:  2011년 04월 08일 16시 41분 42초
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Park Gyu Tae (), 
 *        Company:  Samsung Electronics
 *
 * =====================================================================================
 */
#include "cmc624.h"
#include "samsung_cmc624_tune.h"

#define CMC624_INITSEQ cmc624_init
//#define CMC624_INITSEQ cmc624_bypass
#define CMC624_MAX_SETTINGS	 100
#define TUNING_FILE_PATH "/sdcard/tuning/"

#define PMIC_GPIO_LVDS_nSHDN 18 /*  LVDS_nSHDN GPIO on PM8058 is 19 */
#define PMIC_GPIO_BACKLIGHT_RST 36 /*  LVDS_nSHDN GPIO on PM8058 is 37 */
#define PMIC_GPIO_LCD_LEVEL_HIGH	1
#define PMIC_GPIO_LCD_LEVEL_LOW		0


#define IMA_PWR_EN			13/*70*//*1.1V*/
#define IMA_nRST			14 /*72*/
#define IMA_SLEEP			15 /*103*/
#define IMA_CMC_EN			11/*104*//*failsafeV*/
#define MLCD_ON				12/*128*//*1.8V*/ /*joann_test*/

#define IMA_I2C_SDA 18/*156*/
#define IMA_I2C_SCL 17/*157*/

#define CMC624_FAILSAFE		106
#define P8LTE_LCDON			1
#define P8LTE_LCDOFF		0

/*  BYPASS Mode OFF */
#define BYPASS_MODE_ON 		0


struct cmc624_state_type{
	enum eCabc_Mode cabc_mode;
	unsigned int brightness;
	unsigned int suspended;
    enum eLcd_mDNIe_UI scenario;
    enum eBackground_Mode background;
//This value must reset to 0 (standard value) when change scenario
    enum eCurrent_Temp temperature;
    enum eOutdoor_Mode ove;
    const struct str_sub_unit *sub_tune;
    const struct str_main_unit *main_tune;
};


/*  CMC624 function */
int cmc624_sysfs_init(void);
void bypass_onoff_ctrl(int value);
void cabc_onoff_ctrl(int value);
void set_backlight_pwm(int level);
int load_tuning_data(char *filename);
int apply_main_tune_value(enum eLcd_mDNIe_UI ui, enum eBackground_Mode bg, enum eCabc_Mode cabc, int force);
int apply_sub_tune_value(enum eCurrent_Temp temp, enum eOutdoor_Mode ove, enum eCabc_Mode cabc, int force);
void dump_cmc624_register(void);

int samsung_cmc624_init(void);
//int p8lte_cmc624_setup(void);
int p8lte_cmc624_on(int enable);
int p8lte_cmc624_bypass_mode(void);
int p8lte_cmc624_normal_mode(void);
int p8lte_lvds_on(int enable);
int p8lte_backlight_en(int enable);
void cmc624_manual_brightness(int bl_lvl);
int Is_There_cmc624(void);
void p8lte_get_id(unsigned char *buf);
void Check_Prog(void);
void change_mon_clk(void);

#if 1 //defined(CONFIG_TARGET_LOCALE_KOR_SKT) || defined(CONFIG_TARGET_LOCALE_KOR_LGU)
void cmc624_Set_Region_Ext(int enable, int hStart, int hEnd, int vStart, int vEnd);
#endif
