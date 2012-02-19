/*
  SEC ISX012
 */
/***************************************************************
CAMERA DRIVER FOR 5M CAM (SONY)
****************************************************************/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>


#include "sec_isx012.h"

#include "sec_cam_pmic.h"
#include "sec_cam_dev.h"

#include "sec_isx012_reg.h"	

#include <linux/clk.h>
#include <linux/io.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>


#include <asm/mach-types.h>
#include <mach/vreg.h>
#include <linux/io.h>
#include "msm.h"


//#define CONFIG_LOAD_FILE

#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

static char *isx012_regs_table = NULL;
static int isx012_regs_table_size;
static int isx012_write_regs_from_sd(char *name);

#endif

#define ISX012_WRITE_LIST(A)			isx012_i2c_write_list(A,(sizeof(A) / sizeof(A[0])),#A);

/*native cmd code*/
#define CAM_AF		1
#define CAM_FLASH	2

#define FACTORY_TEST 1

struct isx012_work_t {
	struct work_struct work;
};

static struct  isx012_work_t *isx012_sensorw;
static struct  i2c_client *isx012_client;

static struct isx012_ctrl_t *isx012_ctrl;
int iscapture = 0;
int gLowLight_value = 0;
int DtpTest = 0;
unsigned int g_ae_auto = 0, g_ersc_auto = 0, g_ae_now = 0, g_ersc_now = 0;

static DECLARE_WAIT_QUEUE_HEAD(isx012_wait_queue);
DECLARE_MUTEX(isx012_sem);



static int isx012_start(void);
 
//#define ISX012_WRITE_LIST(A) \
 //   isx012_i2c_write_list(A,(sizeof(A) / sizeof(A[0])),#A);



 static int isx012_i2c_read_multi(unsigned short subaddr, unsigned long *data)
 {
	 unsigned char buf[4];
	 struct i2c_msg msg = {isx012_client->addr, 0, 2, buf};
 
	 int err = 0;
 
	 if (!isx012_client->adapter) {
		 //dev_err(&isx012_client->dev, "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		 return -EIO;
	 }
 
	 buf[0] = subaddr>> 8;
	 buf[1] = subaddr & 0xff;
 
	 err = i2c_transfer(isx012_client->adapter, &msg, 1);
	 if (unlikely(err < 0)) {
		 //dev_err(&isx012_client->dev, "%s: %d register read fail\n", __func__, __LINE__);
		 return -EIO;
	 }
 
	 msg.flags = I2C_M_RD;
	 msg.len = 4;
 
	 err = i2c_transfer(isx012_client->adapter, &msg, 1);
	 if (unlikely(err < 0)) {
		 //dev_err(&isx012_client->dev, "%s: %d register read fail\n", __func__, __LINE__);
		 return -EIO;
	 }
 
	 /*
	  * Data comes in Little Endian in parallel mode; So there
	  * is no need for byte swapping here
	  */
	 *data = *(unsigned long *)(&buf);
 
	 return err;
 }

static int isx012_i2c_read(unsigned short subaddr, unsigned short *data)
{
	unsigned char buf[2];
	struct i2c_msg msg = {isx012_client->addr, 0, 2, buf};

	int err = 0;

	if (!isx012_client->adapter) {
		//dev_err(&isx012_client->dev, "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		return -EIO;
	}

	buf[0] = subaddr>> 8;
	buf[1] = subaddr & 0xff;

	err = i2c_transfer(isx012_client->adapter, &msg, 1);
	if (unlikely(err < 0)) {
		//dev_err(&isx012_client->dev, "%s: %d register read fail\n", __func__, __LINE__);
		return -EIO;
	}

	msg.flags = I2C_M_RD;

	err = i2c_transfer(isx012_client->adapter, &msg, 1);
	if (unlikely(err < 0)) {
		//dev_err(&isx012_client->dev, "%s: %d register read fail\n", __func__, __LINE__);
		return -EIO;
	}

	/*
	 * Data comes in Little Endian in parallel mode; So there
	 * is no need for byte swapping here
	 */
	*data = *(unsigned short *)(&buf);

	return err;
}

static int isx012_i2c_write_multi(unsigned short addr, unsigned int w_data, unsigned int w_len)
{
	unsigned char buf[w_len+2];
	struct i2c_msg msg = {isx012_client->addr, 0, w_len+2, buf};

	int retry_count = 5;
	int err = 0;

	if (!isx012_client->adapter) {
		//dev_err(&isx012_client->dev, "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		return -EIO;
	}

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;

	/*
	 * Data should be written in Little Endian in parallel mode; So there
	 * is no need for byte swapping here
	 */
	if(w_len == 1) {
		buf[2] = (unsigned char)w_data;
	} else if(w_len == 2)	{
		*((unsigned short *)&buf[2]) = (unsigned short)w_data;
	} else {
		*((unsigned int *)&buf[2]) = w_data;
	}

#if 0 //def ISX012_DEBUG
	{
		int j;
		printk("isx012 i2c write W: ");
		for(j = 0; j <= w_len+1; j++)
		{
			printk("0x%02x ", buf[j]);
		}
		printk("\n");
	}
#endif

	while(retry_count--) {
		err  = i2c_transfer(isx012_client->adapter, &msg, 1);
		if (likely(err == 1))
			break;
//		msleep(POLL_TIME_MS);
	}

	return (err == 1) ? 0 : -EIO;
}



static int isx012_i2c_write_list(isx012_short_t regs[], int size, char *name)
{

#ifdef CONFIG_LOAD_FILE
	isx012_write_regs_from_sd(name);
#else
	int err = 0;
	int i = 0;

	if (!isx012_client->adapter) {
		printk(KERN_ERR "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		return -EIO;
	}

	for (i = 0; i < size; i++) {
		if(regs[i].subaddr == 0xFFFF)
		{
		    msleep(regs[i].value);
                    printk("delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
		}
                else
                {
        			err = isx012_i2c_write_multi(regs[i].subaddr, regs[i].value, regs[i].len);

        		if (unlikely(err < 0)) {
        			printk(KERN_ERR "%s: register set failed\n",  __func__);
        			return -EIO;
        		}
                }
	}
#endif

	return 0;
}
#ifdef CONFIG_LOAD_FILE
void isx012_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	set_fs(get_ds());

	filp = filp_open("/mnt/sdcard/sec_isx012_reg.h", O_RDONLY, 0);

	if (IS_ERR_OR_NULL(filp)) {
		printk(KERN_DEBUG "file open error\n");
		return PTR_ERR(filp);
	}

	l = filp->f_path.dentry->d_inode->i_size;
	printk(KERN_DEBUG "l = %ld\n", l);
	//dp = kmalloc(l, GFP_KERNEL);
	dp = vmalloc(l);
	if (dp == NULL) {
		printk(KERN_DEBUG "Out of Memory\n");
		filp_close(filp, current->files);
	}

	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if (ret != l) {
		printk(KERN_DEBUG "Failed to read file ret = %d\n", ret);
		/*kfree(dp);*/
		vfree(dp);
		filp_close(filp, current->files);
		return -EINVAL;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	isx012_regs_table = dp;

	isx012_regs_table_size = l;

	*((isx012_regs_table + isx012_regs_table_size) - 1) = '\0';

	printk("isx012_reg_table_init\n");
	return 0;
}


void isx012_regs_table_exit(void)
{
	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	if (isx012_regs_table) {
		vfree(isx012_regs_table);
		isx012_regs_table = NULL;
	}
}

static int isx012_write_regs_from_sd(char *name)
{
	char *start, *end, *reg, *size;
	unsigned short addr;
	unsigned int len, value;
	char reg_buf[7], data_buf1[5], data_buf2[7], len_buf[5];

	*(reg_buf + 6) = '\0';
	*(data_buf1 + 4) = '\0';
	*(data_buf2 + 6) = '\0';
	*(len_buf + 4) = '\0';

	printk(KERN_DEBUG "isx012_regs_table_write start!\n");
	printk(KERN_DEBUG "E string = %s", name);

	start = strstr(isx012_regs_table, name);
	end = strstr(start, "};");

	while (1) {
		/* Find Address */
		reg = strstr(start,"{0x");

		if ((reg == NULL) || (reg > end))
			break;

		/* Write Value to Address */
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), 6);
			memcpy(data_buf2, (reg + 8), 6);
			size = strstr(data_buf2,",");
			if (size) { /* 1 byte write */
				memcpy(data_buf1, (reg + 8), 4);
				memcpy(len_buf, (reg + 13), 4);
				addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16);
				value = (unsigned int)simple_strtoul(data_buf1, NULL, 16);
				len = (unsigned int)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 20);  //{0x000b,0x04,0x01},
			} else {/* 2 byte write */
				memcpy(len_buf, (reg + 15), 4);
				addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16);
				value = (unsigned int)simple_strtoul(data_buf2, NULL, 16);
				len = (unsigned int)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 22);  //{0x000b,0x0004,0x01},
			}
			size = NULL;

			if (addr == 0xFFFF) {
				msleep(value);
			} else {
				isx012_i2c_write_multi(addr, value, len);
			}
		}
	}

	printk(KERN_DEBUG "isx005_regs_table_write end!\n");

	return 0;
}
#endif

static int isx012_get_LowLightCondition()
{
	int err = -1;
	u8 r_data[0] = {0};

	err = isx012_i2c_read(0x01A5, r_data);
	//printk(KERN_ERR "func(%s):line(%d) 0x%x\n",__func__, __LINE__, r_data[0]);

	if (err < 0)
		pr_err("%s: isx012_get_LowLightCondition() returned error, %d\n", __func__, err);

	gLowLight_value = r_data[0];
	//printk(KERN_ERR "func(%s):line(%d) gLowLight_value : 0x%x\n",__func__, __LINE__, gLowLight_value);

	return err;
}

void isx012_mode_transtion_OM(void)
{
	int count = 0;
	int status = 0;

	printk("[isx012] %s/%d\n", __func__, __LINE__);

	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E (1) read : %x\n", status);

		if((status & 0x1) == 0x1)
			break;
		else
			mdelay(1);
	}
	isx012_i2c_write_multi(0x0012, 0x01, 0x01);
	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E (2) read : %x\n", status);

		if((status & 0x1) == 0x0)
			break;
		else
			mdelay(1);
	}
}


void isx012_mode_transtion_CM(void)
{
	int count = 0;
	int status = 0;

	printk("[isx012] %s/%d\n", __func__, __LINE__);

	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E (1) read : %x\n", status);

		if((status & 0x2) == 0x2)
			break;
		else
			mdelay(1);
	}
	isx012_i2c_write_multi(0x0012, 0x02, 0x01);
	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E (2) read : %x\n", status);

		if((status & 0x2) == 0x0)
			break;
		else
			mdelay(1);
	}
}

void isx012_Sensor_Calibration(void)
{
	int count = 0;
	int status = 0;
	int temp = 0;

	printk(KERN_DEBUG "[isx012] %s/%d\n", __func__, __LINE__);

/* Read OTP1 */
	isx012_i2c_read(0x004F, (unsigned short *)&status);
	printk(KERN_DEBUG "[isx012] 0x004F read : %x\n", status);

	if ((status & 0x1) == 0x1) {
		/* Read ShadingTable */
		isx012_i2c_read(0x005C, (unsigned short *)&status);
		temp = (status&0x03C0)>>6;
		printk(KERN_DEBUG "[isx012] Read ShadingTable read : %x\n", temp);

		/* Write Shading Table */
		if (temp == 0x0) {
			ISX012_WRITE_LIST(ISX012_Shading_0);
		} else if (temp == 0x1) {
			ISX012_WRITE_LIST(ISX012_Shading_1);
		} else if (temp == 0x2) {
			ISX012_WRITE_LIST(ISX012_Shading_2);
		}

		/* Write NorR */
		isx012_i2c_read(0x0054, (unsigned short *)&status);
		temp = status&0x3FFF;
		printk(KERN_DEBUG "[isx012] NorR read : %x\n", temp);
		isx012_i2c_write_multi(0x6804, temp, 0x02);

		/* Write NorB */
		isx012_i2c_read(0x0056, (unsigned short *)&status);
		temp = status&0x3FFF;
		printk(KERN_DEBUG "[isx012] NorB read : %x\n", temp);
		isx012_i2c_write_multi(0x6806, temp, 0x02);

		/* Write PreR */
		isx012_i2c_read(0x005A, (unsigned short *)&status);
		temp = (status&0x0FFC)>>2;
		printk(KERN_DEBUG "[isx012] PreR read : %x\n", temp);
		isx012_i2c_write_multi(0x6808, temp, 0x02);

		/* Write PreB */
		isx012_i2c_read(0x005B, (unsigned short *)&status);
		temp = (status&0x3FF0)>>4;
		printk(KERN_DEBUG "[isx012] PreB read : %x\n", temp);
		isx012_i2c_write_multi(0x680A, temp, 0x02);
	} else {
		/* Read OTP0 */
		isx012_i2c_read(0x0040, (unsigned short *)&status);
		printk(KERN_DEBUG "[isx012] 0x0040 read : %x\n", status);

		if ((status & 0x1) == 0x1) {
			/* Read ShadingTable */
			isx012_i2c_read(0x004D, (unsigned short *)&status);
			temp = (status&0x03C0)>>6;
			printk(KERN_DEBUG "[isx012] Read ShadingTable read : %x\n", temp);

			/* Write Shading Table */
			if (temp == 0x0) {
				ISX012_WRITE_LIST(ISX012_Shading_0);
			} else if (temp == 0x1) {
				ISX012_WRITE_LIST(ISX012_Shading_1);
			} else if (temp == 0x2) {
				ISX012_WRITE_LIST(ISX012_Shading_2);
			}

			/* Write NorR */
			isx012_i2c_read(0x0045, (unsigned short *)&status);
			temp = status&0x3FFF;
			printk(KERN_DEBUG "[isx012] NorR read : %x\n", temp);
			isx012_i2c_write_multi(0x6804, temp, 0x02);

			/* Write NorB */
			isx012_i2c_read(0x0047, (unsigned short *)&status);
			temp = status&0x3FFF;
			printk(KERN_DEBUG "[isx012] NorB read : %x\n", temp);
			isx012_i2c_write_multi(0x6806, temp, 0x02);

			/* Write PreR */
			isx012_i2c_read(0x004B, (unsigned short *)&status);
			temp = (status&0x0FFC)>>2;
			printk(KERN_DEBUG "[isx012] PreR read : %x\n", temp);
			isx012_i2c_write_multi(0x6808, temp, 0x02);

			/* Write PreB */
			isx012_i2c_read(0x004C, (unsigned short *)&status);
			temp = (status&0x3FF0)>>4;
			printk(KERN_DEBUG "[isx012] PreB read : %x\n", temp);
			isx012_i2c_write_multi(0x680A, temp, 0x02);
		} else
			ISX012_WRITE_LIST(ISX012_Shading_Nocal);
	}
}

static int32_t isx012_i2c_write_32bit(unsigned long packet)
{
	int32_t err = -EFAULT;
	int retry_count = 1;
	int i;
	
	unsigned char buf[4];
	struct i2c_msg msg = {
		.addr = isx012_client->addr,
		.flags = 0,
		.len = 4,
		.buf = buf,
	};
	*(unsigned long *)buf = cpu_to_be32(packet);

	//for(i=0; i< retry_count; i++) {
	err = i2c_transfer(isx012_client->adapter, &msg, 1); 		
  	//}

	return err;
}




#if 0


static int32_t isx012_i2c_write(unsigned short subaddr, unsigned short val)
{
	unsigned long packet;
	packet = (subaddr << 16) | (val&0xFFFF);

	return isx012_i2c_write_32bit(packet);
}


static int32_t isx012_i2c_read(unsigned short subaddr, unsigned short *data)
{

	int ret;
	unsigned char buf[2];

	struct i2c_msg msg = {
		.addr = isx012_client->addr,
		.flags = 0,		
		.len = 2,		
		.buf = buf,	
	};

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(isx012_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	
	if (ret == -EIO) 
	    goto error;
	
	msg.flags = I2C_M_RD;

	ret = i2c_transfer(isx012_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
	    goto error;

	*data = ((buf[0] << 8) | buf[1]);
	

error:
	return ret;

}

#endif


static void isx012_set_ae_lock(char value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);
	printk("isx012_set_ae_lock ");
#if 0
    	switch (value) {
        case EXT_CFG_AE_LOCK:
            	isx012_ctrl->setting.ae_lock = EXT_CFG_AE_LOCK;
            	 ISX012_WRITE_LIST(isx012_ae_lock);
        	break;
        case EXT_CFG_AE_UNLOCK:
            	isx012_ctrl->setting.ae_lock = EXT_CFG_AE_UNLOCK;
            	ISX012_WRITE_LIST(isx012_ae_unlock);
        	break;
        case EXT_CFG_AWB_LOCK:
            	isx012_ctrl->setting.awb_lock = EXT_CFG_AWB_LOCK;
               ISX012_WRITE_LIST(isx012_awb_lock);
        	break;
        case EXT_CFG_AWB_UNLOCK:
            	isx012_ctrl->setting.awb_lock = EXT_CFG_AWB_UNLOCK;
            	ISX012_WRITE_LIST(isx012_awb_unlock);
        	break;
        default:
		cam_err("Invalid(%d)", value);
        	break;
    	}
#endif		
     return err;


}

static long isx012_set_effect(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);
	
retry:
	switch (value) {
		case EFFECT_OFF:
			ISX012_WRITE_LIST(isx012_Effect_Normal);
			err =0;
			break;

		case EFFECT_SEPIA:
			ISX012_WRITE_LIST(isx012_Effect_Sepia);
			err =0;
			break;

		case EFFECT_MONO:
			ISX012_WRITE_LIST(isx012_Effect_Black_White);
			err =0;
			break;

		case EFFECT_NEGATIVE:
			ISX012_WRITE_LIST(ISX012_Effect_Negative);
			err =0;
			break;

		default:
			cam_err("Invalid(%d)", value);
			value = CAMERA_EFFECT_OFF;
			goto retry;
	}
	
	isx012_ctrl->setting.effect = value;
	return err;
}

static int isx012_set_whitebalance(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);

    	switch (value) {
		case WHITE_BALANCE_AUTO :
			ISX012_WRITE_LIST (isx012_WB_Auto);
			err =0;
			break;

		case WHITE_BALANCE_SUNNY:
			ISX012_WRITE_LIST(isx012_WB_Sunny);
			err =0;
			break;

		case WHITE_BALANCE_CLOUDY :
			ISX012_WRITE_LIST(isx012_WB_Cloudy);
			err =0;
			break;

		case WHITE_BALANCE_FLUORESCENT:
			ISX012_WRITE_LIST(isx012_WB_Fluorescent);
			err =0;
			break;

		case WHITE_BALANCE_INCANDESCENT:
			ISX012_WRITE_LIST(isx012_WB_Tungsten);
			err =0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			break;
        }

	isx012_ctrl->setting.whiteBalance = value;
	return err;
}

static int isx012_set_flash(int8_t value1, int8_t value2)
{
	int err = -EINVAL;
	int i = 0;
	int torch,torch2,torch3 =0;
   
	if ((value1 > -1) && (value2 == 50)) {
		printk(KERN_ERR "[FLASH] CASE 1 \n");
		switch (value1) {
			case 0:	//off
				printk(KERN_DEBUG "[kidggang] LED_MODE_OFF\n");
				gpio_set_value_cansleep(62, 0); //62
				gpio_set_value_cansleep(63, 0);	//63
				err = 0;
				break;

			case 1: //Auto
				printk(KERN_DEBUG "[kidggang] LED_MODE_AUTO\n");
				if ( gLowLight_value >= GLOWLIGHT_DEFAULT ) {
					printk(KERN_DEBUG "[kidggang] LED_MODE_AUTO - ON/gLowLight_value(0x%x)\n", gLowLight_value);
					gpio_set_value_cansleep(62, 1); //62
					gpio_set_value_cansleep(63, 0);	//63
				}
				else {
					printk(KERN_DEBUG "[kidggang] LED_MODE_AUTO - OFF/gLowLight_value(0x%x)\n", gLowLight_value);
					gpio_set_value_cansleep(62, 0); //62
					gpio_set_value_cansleep(63, 0);	//63
				}
				
				err = 0;
				break;

			case 2:	//on
				printk(KERN_DEBUG "[kidggang] INIT LED_MODE_ON\n");
				gpio_set_value_cansleep(62, 1); //62
				gpio_set_value_cansleep(63, 0);	//63
				err = 0;
				break;

			case 3:	//torch
				printk(KERN_DEBUG "FLASH MOVIE MODE LED_MODE_TORCH\n");
				gpio_set_value_cansleep(62, 0);	//62
				torch = gpio_get_value(62);
				//printk("2.FLASH MOVIE MODE LED_MODE_TORCH : %d\n", torch);

				for (i = 5; i > 1; i--) {
					gpio_set_value_cansleep(63, 1);	//63
					torch2 = gpio_get_value(63);
					//printk("3.FLASH MOVIE MODE LED_MODE_TORCH  torch2: %d\n", torch2);
					udelay(1);
					gpio_set_value_cansleep(63, 0);	//63
					torch3 = gpio_get_value(63);
					//printk("4.FLASH MOVIE MODE LED_MODE_TORCH torch3: %d\n", torch3);
					udelay(1);
				}
				gpio_set_value_cansleep(63, 1);	//63
				usleep(2*1000);
				err = 0;
				break;

			default :
				cam_err("Invalid value1(%d) value2(%d)", value1, value2);
				break;
		}
	} else if ((value1 == 50 && value2 > -1)) {
		printk(KERN_ERR "[FLASH] CASE 2 \n");
		isx012_ctrl->setting.flash_mode = value2;
		cam_err("flash value1(%d) value2(%d) isx012_ctrl->setting.flash_mode(%d)", value1, value2, isx012_ctrl->setting.flash_mode);
		err = 0;
	}

	cam_err("FINAL flash value1(%d) value2(%d) isx012_ctrl->setting.flash_mode(%d)", value1, value2, isx012_ctrl->setting.flash_mode);

	return err;
}

static int  isx012_set_brightness(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d", value);

	switch (value) {
		case EV_MINUS_4 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_M4Step);
			err = 0;
			break;

		case EV_MINUS_3 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_M3Step);
			err = 0;
			break;

		case EV_MINUS_2 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_M2Step);
			err = 0;
			break;

		case EV_MINUS_1 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_M1Step);
			err = 0;
			break;

		case EV_DEFAULT :
			ISX012_WRITE_LIST(ISX012_ExpSetting_Default);
			err = 0;
			break;

		case EV_PLUS_1 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_P1Step);
			err = 0;
			break;

		case EV_PLUS_2 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_P2Step);
			err = 0;
			break;

		case EV_PLUS_3 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_P3Step);
			err = 0;
			break;

		case EV_PLUS_4 :
			ISX012_WRITE_LIST(ISX012_ExpSetting_P4Step);
			err = 0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			break;
	}

	isx012_ctrl->setting.brightness = value;
	return err;
}


static int  isx012_set_iso(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d", value);
	
	switch (value) {
		case ISO_AUTO :
			ISX012_WRITE_LIST(isx012_ISO_Auto);
			err = 0;
			break;

		case ISO_50 :
			ISX012_WRITE_LIST(isx012_ISO_50);
			err = 0;
			break;

		case ISO_100 :
			ISX012_WRITE_LIST(isx012_ISO_100);
			err = 0;
			break;

		case ISO_200 :
			ISX012_WRITE_LIST(isx012_ISO_200);
			err = 0;
			break;

		case ISO_400 :
			ISX012_WRITE_LIST(isx012_ISO_400);
			err = 0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			break;
	}

	isx012_ctrl->setting.iso = value;    
	return err;
}


static int isx012_set_metering(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d", value);

retry:
	switch (value) {
		case METERING_MATRIX:
			ISX012_WRITE_LIST(isx012_Metering_Matrix);
			err = 0;
			break;

		case METERING_CENTER:
			ISX012_WRITE_LIST(isx012_Metering_Center);
			err = 0;
			break;

		case METERING_SPOT:
			ISX012_WRITE_LIST(isx012_Metering_Spot);
			err = 0;
			break;

		default:
			cam_err("Invalid(%d)", value);
			value = METERING_CENTER;
			goto retry;
	}

	isx012_ctrl->setting.metering = value;
	return err;
}



static int isx012_set_contrast(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);

retry:
	switch (value) {
		case CONTRAST_MINUS_2 :
			ISX012_WRITE_LIST(isx012_Contrast_Minus_2);
			err = 0;
			break;

		case CONTRAST_MINUS_1 :
			ISX012_WRITE_LIST(isx012_Contrast_Minus_1);
			err = 0;
			break;

		case CONTRAST_DEFAULT :
			ISX012_WRITE_LIST(isx012_Contrast_Default);
			err = 0;
			break;

		case CONTRAST_PLUS_1 :
			ISX012_WRITE_LIST(isx012_Contrast_Plus_1);
			err = 0;
			break;

		case CONTRAST_PLUS_2 :
			ISX012_WRITE_LIST(isx012_Contrast_Plus_2);
			err = 0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			value = METERING_CENTER;
			goto retry;
   	}

	isx012_ctrl->setting.contrast = value;	
	return err;
}


static int isx012_set_saturation(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);

retry:
	switch (value) {
		case SATURATION_MINUS_2 :
			ISX012_WRITE_LIST (isx012_Saturation_Minus_2);
			err = 0;
			break;

		case SATURATION_MINUS_1 :
			ISX012_WRITE_LIST(isx012_Saturation_Minus_1);
			err = 0;
			break;

		case SATURATION_DEFAULT :
			ISX012_WRITE_LIST(isx012_Saturation_Default);
			err = 0;
			break;

		case SATURATION_PLUS_1 :
			ISX012_WRITE_LIST(isx012_Saturation_Plus_1);
			err = 0;
			break;

		case SATURATION_PLUS_2 :
			ISX012_WRITE_LIST(isx012_Saturation_Plus_2);
			err = 0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			value = METERING_CENTER;
			goto retry;
   	}

	isx012_ctrl->setting.saturation = value;
	return err;
}


static int isx012_set_sharpness(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);

retry:
	switch (value) {
		case SHARPNESS_MINUS_2 :
			ISX012_WRITE_LIST(isx012_Sharpness_Minus_2);
			err = 0;
			break;

		case SHARPNESS_MINUS_1 :
			ISX012_WRITE_LIST(isx012_Sharpness_Minus_1);
			err = 0;
			break;

		case SHARPNESS_DEFAULT :
			ISX012_WRITE_LIST(isx012_Sharpness_Default);
			err = 0;
			break;

		case SHARPNESS_PLUS_1 :
			ISX012_WRITE_LIST(isx012_Sharpness_Plus_1);
			err = 0;
			break;

		case SHARPNESS_PLUS_2 :
			ISX012_WRITE_LIST(isx012_Sharpness_Plus_2);
			err = 0;
			break;

		default :
			cam_err("Invalid(%d)", value);
			value = METERING_CENTER;
			goto retry;
   	}

	isx012_ctrl->setting.sharpness = value;
	return err;
}




static int  isx012_set_scene(int8_t value)
{
	int err = -EINVAL;
	CAM_DEBUG("%d",value);

	if (value != SCENE_OFF) {
	     ISX012_WRITE_LIST(isx012_Scene_Default);
	}
	
	switch (value) {
		case SCENE_OFF:
			ISX012_WRITE_LIST(isx012_Scene_Default);
			err = 0;
			break;

		case SCENE_PORTRAIT: 
			ISX012_WRITE_LIST(isx012_Scene_Portrait);
			err = 0;
			break;

		case SCENE_LANDSCAPE:
			ISX012_WRITE_LIST(isx012_Scene_Landscape);
			err = 0;
			break;

		case SCENE_SPORTS:
			ISX012_WRITE_LIST(isx012_Scene_Sports);
			err = 0;
			break;

		case SCENE_PARTY:
			ISX012_WRITE_LIST(isx012_Scene_Party_Indoor);
			err = 0;
			break;

		case SCENE_BEACH:
			ISX012_WRITE_LIST(isx012_Scene_Beach_Snow);
			err = 0;
			break;

		case SCENE_SUNSET:
			ISX012_WRITE_LIST(isx012_Scene_Sunset);
			err = 0;
			break;

		case SCENE_DAWN:
			ISX012_WRITE_LIST(isx012_Scene_Duskdawn);
			err = 0;
			break;

		case SCENE_FALL:
			ISX012_WRITE_LIST(isx012_Scene_Fall_Color);
			err = 0;
			break;

		case SCENE_NIGHTSHOT:
			ISX012_WRITE_LIST(isx012_Scene_Nightshot);
			err = 0;
			break;

		case SCENE_BACKLIGHT:
			ISX012_WRITE_LIST(isx012_Scene_Backlight);
			err = 0;
			break;

		case SCENE_FIREWORK:
			ISX012_WRITE_LIST(isx012_Scene_Fireworks);
			err = 0;
			break;

		case SCENE_TEXT:
			ISX012_WRITE_LIST(isx012_Scene_Text);
			err = 0;
			break;

		case SCENE_CANDLE:
			printk(KERN_DEBUG "[kidggang] scene candle\n");
			ISX012_WRITE_LIST(isx012_Scene_Candle_Light);
			err = 0;
			break;

		default:
			cam_err("Invalid(%d)", value);
			break;
	}


	isx012_ctrl->setting.scene = value;

	return err;
}

static int isx012_set_preview_size( int32_t value1, int32_t value2) 
{
	CAM_DEBUG("value1 %d value2 %d ",value1,value2);

	printk("isx012_set_preview_size ");

	if (value1 == 1280 && value2 ==720){
		printk("[INSOOK]isx012_set_preview_size isx012_1280_Preview_E");
		ISX012_WRITE_LIST(isx012_1280_Preview_E);
		}
	else if (value1 == 800 && value2 ==480){
		printk("[INSOOK]isx012_set_preview_size isx012_800_Preview_E");
		ISX012_WRITE_LIST(isx012_800_Preview);
		}
	else if (value1 == 720 && value2 ==480){
		printk("[INSOOK]isx012_set_preview_size isx012_720_Preview_E");
		ISX012_WRITE_LIST(isx012_720_Preview);
		}
	else{
		printk("[INSOOK]isx012_set_preview_size isx012_640_Preview_E");
		ISX012_WRITE_LIST(isx012_640_Preview);
		}

		ISX012_WRITE_LIST(ISX012_Preview_Mode);
    	isx012_mode_transtion_CM(); 		
		
#if 0
	if(HD_mode) {
		    HD_mode = 0;
		    isx012_WRITE_LIST(isx012_1280_Preview_D)
	}

	switch (value1) {
	case 1280: 
		//HD_mode = 1;
		printk("isx012_set_preview_size isx012_1280_Preview_E");
		ISX012_WRITE_LIST(isx012_1280_Preview_E);
		ISX012_WRITE_LIST(ISX012_Preview_Mode); 
		break;
	case 720:
		printk("isx012_set_preview_size isx012_720_Preview_E");
		ISX012_WRITE_LIST(isx012_720_Preview);
		ISX012_WRITE_LIST(ISX012_Preview_Mode); 
		break;
	case 640:
		printk("isx012_set_preview_size isx012_640_Preview_E");
		ISX012_WRITE_LIST(isx012_640_Preview);
		ISX012_WRITE_LIST(ISX012_Preview_Mode); 
		break;
	case 320: 
		printk("isx012_set_preview_size isx012_320_Preview_E");
		ISX012_WRITE_LIST(isx012_320_Preview);
		ISX012_WRITE_LIST(ISX012_Preview_Mode);
		break;
	case 176:
		printk("isx012_set_preview_size isx012_176_Preview_E");
		ISX012_WRITE_LIST(isx012_176_Preview);
		ISX012_WRITE_LIST(ISX012_Preview_Mode);
		break;
	default:
		cam_err("Invalid");
		break;

}
		
#endif		

	isx012_ctrl->setting.preview_size= value1;


	return 0;
}




static int isx012_set_picture_size(int value)
{
	CAM_DEBUG("%d",value);
	printk("isx012_set_picture_size ");
#if 0	
	switch (value) {
	case EXT_CFG_SNAPSHOT_SIZE_2560x1920_5M:
		ISX012_WRITE_LIST(isx012_5M_Capture);
		//isx012_set_zoom(EXT_CFG_ZOOM_STEP_0);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_2560x1536_4M_WIDE:
		ISX012_WRITE_LIST(isx012_4M_WIDE_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_2048x1536_3M:
		ISX012_WRITE_LIST(isx012_5M_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_2048x1232_2_4M_WIDE:
		ISX012_WRITE_LIST(isx012_2_4M_WIDE_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_1600x1200_2M:
		ISX012_WRITE_LIST(isx012_5M_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_1600x960_1_5M_WIDE:
		ISX012_WRITE_LIST(isx012_1_5M_WIDE_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_1280x960_1M:
		ISX012_WRITE_LIST(isx012_1M_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_800x480_4K_WIDE:
		ISX012_WRITE_LIST(isx012_4K_WIDE_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_640x480_VGA:
		ISX012_WRITE_LIST(isx012_VGA_Capture);
		break;
	case EXT_CFG_SNAPSHOT_SIZE_320x240_QVGA:
		ISX012_WRITE_LIST(isx012_QVGA_Capture);
		break;
	default:
		cam_err("Invalid");
		return -EINVAL;
	}


//	if(size != EXT_CFG_SNAPSHOT_SIZE_2560x1920_5M && isx012_status.zoom != EXT_CFG_ZOOM_STEP_0)
//		isx012_set_zoom(isx012_status.zoom);


	isx012_ctrl->setting.snapshot_size = value;
#endif
	return 0;
}

static int isx012_set_focus_mode(int fmode)
{
	CAM_DEBUG("E");

retry:
	switch (fmode) {
		case FOCUS_AUTO :
			if(isx012_ctrl->status.initialized == 1) {
				cam_err("SET param AUTO");
				ISX012_WRITE_LIST(ISX012_AF_Macro_OFF);
				ISX012_WRITE_LIST(ISX012_AF_ReStart);
			}
			isx012_ctrl->setting.afmode = FOCUS_AUTO; 
			break;

		case FOCUS_MACRO :
			if(isx012_ctrl->status.initialized == 1) {
				cam_err("SET param MACRO");
				ISX012_WRITE_LIST(ISX012_AF_Macro_ON);
				ISX012_WRITE_LIST(ISX012_AF_ReStart);
			}
			isx012_ctrl->setting.afmode = FOCUS_MACRO; 
			break;

		default :
			cam_err("Invalid(%d)", fmode);
			fmode = FOCUS_AUTO;
			goto retry;
   	}

	return 0;
}

static int isx012_set_movie_mode(int mode)
{
	CAM_DEBUG("E");

	if(mode == SENSOR_MOVIE){
		printk("isx012_set_movie_mode Camcorder_Mode_ON");
		ISX012_WRITE_LIST(ISX012_Camcorder_Mode_ON);
		printk("isx012_set_movie_mode preview resizing");
 	  	ISX012_WRITE_LIST(isx012_1280_Preview_E);
	  	ISX012_WRITE_LIST(ISX012_Preview_Mode);  // resizing  video preview
		}
	if ((mode != SENSOR_CAMERA) && (mode != SENSOR_MOVIE)) {
		return -EINVAL;
	}

	return 0;
}


static int isx012_check_dataline(s32 val)
{
	int err = -EINVAL;

	CAM_DEBUG("%s", val ? "ON" : "OFF");
	if (val) {
		printk(KERN_DEBUG "[kidggang] DTP ON\n");
		ISX012_WRITE_LIST(isx012_DTP_init);
		DtpTest = 1;
		err = 0;
		
	} else {
		printk(KERN_DEBUG "[kidggang] DTP OFF\n");
		ISX012_WRITE_LIST(isx012_DTP_stop);
		DtpTest = 0;
		err = 0;
	}

	return err;
}

static int isx012_mipi_mode(int mode)
{
	int err = 0;
	struct msm_camera_csi_params isx012_csi_params;
	
	CAM_DEBUG("E");

	if (!isx012_ctrl->status.config_csi1) {
		isx012_csi_params.lane_cnt = 2;
		isx012_csi_params.data_format = 0x1E;
		isx012_csi_params.lane_assign = 0xe4;
		isx012_csi_params.dpcm_scheme = 0;
		isx012_csi_params.settle_cnt = 24;// 0x14; //0x7; //0x14;
		err = msm_camio_csi_config(&isx012_csi_params);
		
		if (err < 0)
			printk(KERN_ERR "config csi controller failed \n");
		
		isx012_ctrl->status.config_csi1 = 1;
	}
	
	CAM_DEBUG("X");
	return err;
}


static int isx012_start(void)
{
	int err = 0;

	CAM_DEBUG("E");

	printk("isx012_start ");

	if (isx012_ctrl->status.started) {
		CAM_DEBUG("X : already started");
		return -EINVAL;
	}
	isx012_mipi_mode(1);
	msleep(30); //=> Please add some delay 
	
	
	//ISX012_WRITE_LIST(isx012_init_reg1);
	msleep(10);
	
        //ISX012_WRITE_LIST(isx012_init_reg2);

	isx012_ctrl->status.initialized = 1;
	isx012_ctrl->status.started = 1;

	CAM_DEBUG("X");

	return err;
}



static long isx012_video_config(int mode)
{
	int err = 0; //-EINVAL;
	CAM_DEBUG("E");

	return err;
}

static long isx012_snapshot_config(int mode)
{
	int err = -EINVAL;
	CAM_DEBUG("E");

	 ISX012_WRITE_LIST(ISX012_Capture_SizeSetting);
	 ISX012_WRITE_LIST(isx012_Capture_Start);
	 
	 isx012_mode_transtion_CM();

	return err;
}

static long isx012_set_sensor_mode(int mode)
{
	int err = -EINVAL;
	u8 r_data[0] = {0};
	int modesel_fix = 0, awbsts = 0;
	int timeout_cnt = 0;

	printk("isx012_set_sensor_mode\n");

	switch (mode) {
		case SENSOR_PREVIEW_MODE:
			CAM_DEBUG("SENSOR_PREVIEW_MODE START");

			if (((isx012_ctrl->setting.flash_mode == 1) && (gLowLight_value >= GLOWLIGHT_DEFAULT)) || (isx012_ctrl->setting.flash_mode == 2)) {
				isx012_set_flash(0, 50);
				ISX012_WRITE_LIST(ISX012_Flash_OFF);
				isx012_i2c_write_multi(0x8800, 0x01, 0x01);
			}

			if(iscapture == 1)
			{
				iscapture = 0;
				ISX012_WRITE_LIST(ISX012_Preview_Mode);		
				isx012_mode_transtion_CM();			
			}
			isx012_start();

			isx012_set_focus_mode(isx012_ctrl->setting.afmode);
				
			//if (isx012_ctrl->sensor_mode != SENSOR_MOVIE)
			//err= isx012_video_config(SENSOR_PREVIEW_MODE);

			break;

		case SENSOR_SNAPSHOT_MODE:
		case SENSOR_RAW_SNAPSHOT_MODE:	
			CAM_DEBUG("SENSOR_SNAPSHOT_MODE START");
			switch (isx012_ctrl->setting.flash_mode) {
				case 0 ://LED_MODE_OFF
					CAM_DEBUG("FLASH_MODE OFF");
					isx012_set_flash(0, 50);
					break;

				case 1 :	//LED_MODE_AUTO
					CAM_DEBUG("FLASH_MODE AUTO");
					isx012_set_flash(1, 50);
					break;

				case 2 :	//LED_MODE_ON
					CAM_DEBUG("FLASH_MODE ON");
					isx012_set_flash(2, 50);
					break;

				default :
					CAM_DEBUG("FLASH_MODE invalid");
					break;
			}

			if (((isx012_ctrl->setting.flash_mode == 1) && (gLowLight_value >= GLOWLIGHT_DEFAULT)) || (isx012_ctrl->setting.flash_mode == 2)) {
				ISX012_WRITE_LIST(ISX012_Capture_Mode);
				isx012_i2c_write_multi(0x8800, 0x01, 0x01);

				timeout_cnt = 0;
				do {
					if (timeout_cnt > 0){
						mdelay(10);
					}
					timeout_cnt++;
					err = isx012_i2c_read(0x0080, r_data);
					modesel_fix = r_data[0];
					//printk(KERN_DEBUG "[kidggang][%s:%d] modesel_fix(0x%x)\n", __func__, __LINE__, modesel_fix);
					if (timeout_cnt > ISX012_DELAY_RETRIES) {
						pr_err("%s: %d :Entering delay timed out \n", __func__, __LINE__);
						break;
					}
				}while ((modesel_fix == 0x2));

				//wait 1V time (66ms)
				mdelay(66);

				timeout_cnt = 0;
				do {
					if (timeout_cnt > 0){
						mdelay(10);
					}
					timeout_cnt++;
					err = isx012_i2c_read(0x8A24, r_data);
					awbsts = r_data[0];
					//printk(KERN_DEBUG "[kidggang][%s:%d] awbsts(0x%x)\n", __func__, __LINE__, awbsts);
					if (timeout_cnt > ISX012_DELAY_RETRIES) {
						pr_err("%s: %d :Entering delay timed out \n", __func__, __LINE__);
						break;
					}
				}while ((awbsts == 0x2));
			}

			iscapture = 1;
			err = isx012_snapshot_config(SENSOR_SNAPSHOT_MODE);

			break;

		
		case SENSOR_SNAPSHOT_TRANSFER:
			CAM_DEBUG("SENSOR_SNAPSHOT_TRANSFER START");

			break;
			
		default:
			return 0;//-EFAULT;
	}

	return 0;
}

#ifdef FACTORY_TEST
struct class *camera_class;
struct device *isx012_dev;

static ssize_t cameraflash_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* Reserved */
	return 0;
}

static ssize_t cameraflash_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	if (value == 0) {
		printk(KERN_INFO "[Factory Test] flash OFF\n");
		isx012_set_flash(0, 50);
	} else {
		printk(KERN_INFO "[Factory Test] flash ON\n");
		isx012_set_flash(3, 50);
	}

	return size;
}

static DEVICE_ATTR(flash, 0660, cameraflash_file_cmd_show, cameraflash_file_cmd_store);

static ssize_t camtype_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char camType[] = "SONY_ISX012_NONE";

	return sprintf(buf, "%s", camType);
}

static ssize_t camtype_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(type, 0660, camtype_file_cmd_show, camtype_file_cmd_store);

#endif

static int isx012_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int err = 0;
	int temp = 0;
	int status = 0;
	int count = 0;

	printk("POWER ON START\n");
	printk("isx012_sensor_init_probe\n");

	gpio_tlmm_config(GPIO_CFG(50, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //reset
	gpio_tlmm_config(GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //stanby
	gpio_tlmm_config(GPIO_CFG(41, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(62, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);// flash EN
	gpio_tlmm_config(GPIO_CFG(63, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);// flash set

	gpio_set_value_cansleep(62, 0);
	gpio_set_value_cansleep(63, 0);

	gpio_set_value_cansleep(50, 0);
	temp = gpio_get_value(50);
	printk("[isx012] CAM_5M_RST : %d\n", temp);

	gpio_set_value_cansleep(49, 0);
	temp = gpio_get_value(49);
	printk("[isx012] CAM_5M_STBY : %d\n", temp);

	cam_ldo_power_on();
 
	msm_camio_clk_rate_set(24000000); //MCLK on
	udelay(50); //min 50us
 
	gpio_set_value_cansleep(41, 1); //VT Rest on
	temp = gpio_get_value(41);
	printk("[isx012] VT Rest ON : %d\n", temp);
	mdelay(125);

	gpio_set_value_cansleep(CAM_VGA_EN, LOW); //VT STBY off
	temp = gpio_get_value(CAM_VGA_EN);
	printk("[isx012] VT STBY off : %d\n", temp);
	udelay(10);

	gpio_set_value_cansleep(50, 1); // 5M REST
	temp = gpio_get_value(50);
	printk("[isx012] CAM_5M_RST : %d\n", temp);
	mdelay(7);

	gpio_set_value_cansleep(49, 1); //STBY 0 -> 1
	temp = gpio_get_value(49);
	printk("[isx012] CAM_5M_ISP_STNBY : %d\n", temp);

	mdelay(12);


	printk("[isx012] Mode Trandition 1\n");

	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E(1) read : %x\n", status);

		if(status == 1)
			break;
		else
			mdelay(1);
	}
	isx012_i2c_write_multi(0x0012, 0x01, 0x01);
	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E(2) read : %x\n", status);

		if(status == 0)
			break;
		else
			mdelay(1);
	}
	mdelay(10);
	ISX012_WRITE_LIST(ISX012_Pll_Setting_2);
	printk("[isx012] Mode Trandition 2\n");

#if defined(CONFIG_USA_MODEL_SGH_I577)
	isx012_i2c_write_multi(0x008C, 0x03, 0x01);	//READVECT_CAP : 180
	isx012_i2c_write_multi(0x008D, 0x03, 0x01);	//READVECT_CAP : 180
	isx012_i2c_write_multi(0x008E, 0x03, 0x01);	//READVECT_CAP : 180
#endif

	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E(3) read : %x\n", status);

		if(status == 1)
			break;
		else
			mdelay(1);
	}
	isx012_i2c_write_multi(0x0012, 0x01, 0x01);
	for(count = 0; count < 100 ; count++)
	{
		isx012_i2c_read(0x000E, (unsigned short*)&status);
		//printk("[isx012] 0x000E(4) read : %x\n", status);

		if(status == 0)
			break;
		else
			mdelay(1);
	}
	printk("[isx012] MIPI write\n");

	//isx012_i2c_write_multi_temp(0x5008, 0x00, 0x01);
	isx012_i2c_write_multi(0x5008, 0x00, 0x01);
	
	isx012_Sensor_Calibration();

	ISX012_WRITE_LIST(ISX012_Init_Reg);
	ISX012_WRITE_LIST(ISX012_Preview_SizeSetting);
	ISX012_WRITE_LIST(ISX012_Preview_Mode);


	isx012_mode_transtion_OM();

	isx012_mode_transtion_CM();

	mdelay(50);
	
	CAM_DEBUG("POWER ON END ");

	return err;
}

int isx012_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int err = 0;
	CAM_DEBUG("E");
	printk("isx012_sensor_open_init ");
	
	isx012_ctrl = kzalloc(sizeof(struct isx012_ctrl_t), GFP_KERNEL);
	if (!isx012_ctrl) {
		cam_err("failed!");
		err = -ENOMEM;
		goto init_done;
	}	
	
	if (data)
		isx012_ctrl->sensordata = data;
 
#ifdef CONFIG_LOAD_FILE
		isx012_regs_table_init();
#endif	

	err = isx012_sensor_init_probe(data);
	if (err < 0) {
		cam_err("isx012_sensor_open_init failed!");
		goto init_fail;
	}


	
	isx012_ctrl->status.started = 0;
	isx012_ctrl->status.initialized = 0;
	isx012_ctrl->status.config_csi1 = 0;
	
	isx012_ctrl->setting.check_dataline = 0;
	isx012_ctrl->setting.camera_mode = SENSOR_CAMERA;

	

	CAM_DEBUG("X");
init_done:
	return err;

init_fail:
	kfree(isx012_ctrl);
	return err;
}

static int isx012_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&isx012_wait_queue);
	return 0;
}

static int isx012_sensor_af_status(void)
{
	int ret = 0;
	int status = 0;

	//printk(KERN_DEBUG "[isx012] %s/%d\n", __func__, __LINE__);

	isx012_i2c_read(0x8B8A, (unsigned short *)&status);
	if ((status & 0x8) == 0x8) {
		ret = 1;
		printk(KERN_DEBUG "[isx012] success\n");
	}

	return ret;
}

static int isx012_sensor_af_result(void)
{
	int ret = 0;
	int status = 0;

	printk(KERN_DEBUG "[isx012] %s/%d\n", __func__, __LINE__);

	isx012_i2c_read(0x8B8B, (unsigned short *)&status);
	if ((status & 0x1) == 0x1) {
		printk(KERN_DEBUG "[isx012] AF success\n");
		ret = 1;
	} else if ((status & 0x1) == 0x0) {
		printk(KERN_DEBUG "[isx012] AF fail\n");
		ret = 2;
	}
	return ret;
}

static int calculate_AEgain_offset(unsigned int ae_auto, unsigned int ae_now, unsigned int ersc_auto, unsigned int ersc_now)
{
	int err = -EINVAL;
	int aediff, aeoffset;

	//AE_Gain_Offset = Target - ERRSCL_NOW
	aediff = (ae_now + ersc_now) - (ae_auto + ersc_auto);
	
	if(aediff < 0) {
		aediff = 0;
	}

	if(ersc_now < 0) {
		if(aediff >= AE_MAXDIFF) {
			aeoffset = -AE_OFSETVAL - ersc_now;
		} else {
			aeoffset = -aeoffset_table[aediff/10] - ersc_now;
		}
	} else {
		if(aediff >= AE_MAXDIFF) {
			aeoffset = -AE_OFSETVAL;
		} else {
			aeoffset = -aeoffset_table[aediff/10];
		}
	}
	printk(KERN_DEBUG "[kidggang] aeoffset(%d) | aediff(%d) = (ae_now(%d) + ersc_now(%d)) - (ae_auto(%d) + ersc_auto(%d))\n", aeoffset, aediff, ae_now, ersc_now, ae_auto, ersc_auto);
	

	// SetAE Gain offset
	err = isx012_i2c_write_multi(CAP_GAINOFFSET, aeoffset, 2);

	return err;
}

int isx012_sensor_ext_config(void __user *argp)
{
	sensor_ext_cfg_data cfg_data;
	int err = 0;

	u8 r_data[0] = {0};
	int modesel_fix = 0, half_move_sts = 0;
	int timeout_cnt = 0;

	//printk("isx012_sensor_ext_config ");

	if (copy_from_user((void *)&cfg_data, (const void *)argp, sizeof(cfg_data))){
		cam_err("fail copy_from_user!");
	}

	if(cfg_data.cmd != EXT_CFG_SET_AF_START) {
		printk("[%s][line:%d] cfg_data.cmd(%d) cfg_data.value_1(%d),cfg_data.value_2(%d)\n", __func__, __LINE__, cfg_data.cmd, cfg_data.value_1, cfg_data.value_2);
	}

	if(DtpTest == 1) {
		if ((cfg_data.cmd != EXT_CFG_SET_VT_MODE)
		&& (cfg_data.cmd != EXT_CFG_SET_MOVIE_MODE)
		&& (cfg_data.cmd != EXT_CFG_SET_DTP)
		&& (!isx012_ctrl->status.initialized)) {
			printk("[%s][line:%d] camera isn't initialized(status.initialized:%d)\n", __func__, __LINE__, isx012_ctrl->status.initialized);
			printk("[%s][line:%d] cfg_data.cmd(%d) cfg_data.value_1(%d),cfg_data.value_2(%d)\n", __func__, __LINE__, cfg_data.cmd, cfg_data.value_1, cfg_data.value_2);
			return 0;
		}
	}

	switch (cfg_data.cmd) {
		case EXT_CFG_SET_AF_START:
			if (cfg_data.value_1 == 0) {
				if (((isx012_ctrl->setting.flash_mode == 1) && (gLowLight_value >= GLOWLIGHT_DEFAULT)) || (isx012_ctrl->setting.flash_mode == 2)) {
					printk(KERN_DEBUG "[kidggang][%s:%d] gLowLight_value(0x%x)\n", __func__, __LINE__, gLowLight_value);
					//AE line change - AE line change value write
					ISX012_WRITE_LIST(ISX012_Flash_AELINE);
					
					//wait 1V time (60ms)
					mdelay(60);

					//Read AE scale - ae_auto, ersc_auto
					err = isx012_i2c_read(0x01CE, r_data);
					g_ae_auto = r_data[0];
					//printk(KERN_DEBUG "[kidggang] g_ae_auto(0x%x)\n", g_ae_auto);
					err = isx012_i2c_read(0x01CA, r_data);
					g_ersc_auto = r_data[0];
					//printk(KERN_DEBUG "[kidggang] g_ersc_auto(0x%x)\n", g_ersc_auto);
					
					//Flash On set
					ISX012_WRITE_LIST(ISX012_Flash_ON);

					//Fast AE, AWB, AF start
					isx012_i2c_write_multi(0x8800, 0x01, 0x01);

					//wait 1V time (40ms)
					mdelay(40);
	
					isx012_set_flash(3 , 50);
					timeout_cnt = 0;
					do {
						if (timeout_cnt > 0){
							mdelay(10);
						}
						timeout_cnt++;
						err = isx012_i2c_read(0x0080, r_data);
						modesel_fix = r_data[0];
						//printk(KERN_DEBUG "[kidggang][%s:%d] modesel_fix(0x%x)\n", __func__, __LINE__, modesel_fix);
						if (timeout_cnt > ISX012_DELAY_RETRIES) {
				                        pr_err("%s: %d :Entering delay timed out \n", __func__, __LINE__);
                				        break;
                				}
					}while ((modesel_fix == 0x1));

					timeout_cnt = 0;

					do {
						if (timeout_cnt > 0){
							mdelay(10);
						}
						timeout_cnt++;
						err = isx012_i2c_read(0x01B0, r_data);
						half_move_sts = r_data[0];
						//printk(KERN_DEBUG "[kidggang][%s:%d] half_move_sts(0x%x)\n", __func__, __LINE__, half_move_sts);
						if (timeout_cnt > ISX012_DELAY_RETRIES) {
				                        pr_err("%s: %d :Entering delay timed out \n", __func__, __LINE__);
                				        break;
                				}
					}while ((half_move_sts == 0x0));
				}
				ISX012_WRITE_LIST(ISX012_Halfrelease_Mode);
			} else if (cfg_data.value_1 == 1) {
				cfg_data.value_2 = isx012_sensor_af_status();
				if (cfg_data.value_2 == 1) {
					isx012_i2c_write_multi(0x0012, 0x10, 0x01);
				}
			} else if (cfg_data.value_1 == 2) {
				cfg_data.value_2 = isx012_sensor_af_result();
				if (((isx012_ctrl->setting.flash_mode == 1) && (gLowLight_value >= GLOWLIGHT_DEFAULT)) || (isx012_ctrl->setting.flash_mode == 2)) {
					printk(KERN_DEBUG "[kidggang][%s:%d] gLowLight_value(0x%x)\n", __func__, __LINE__, gLowLight_value);
					isx012_set_flash(0 , 50);
					ISX012_WRITE_LIST(ISX012_AF_SAF_OFF);

					//wait 1V time (66ms)
					mdelay(66);

					//Read AE scale - ae_now, ersc_now
					err = isx012_i2c_read(0x01CC, r_data);
					g_ersc_now = r_data[0];
					//printk(KERN_DEBUG "[kidggang] g_ersc_now(0x%x)\n", g_ersc_now);
					err = isx012_i2c_read(0x01D0, r_data);
					g_ae_now = r_data[0];
					//printk(KERN_DEBUG "[kidggang] g_ae_now(0x%x)\n", g_ae_now);

					err = calculate_AEgain_offset(g_ae_auto, g_ae_now, g_ersc_auto, g_ersc_now);
					isx012_i2c_write_multi(0x5000, 0x0A, 0x01);
				}
			}
			break;

		case EXT_CFG_SET_FLASH:
			err = isx012_set_flash(cfg_data.value_1, cfg_data.value_2);
			break;
			
		case EXT_CFG_SET_BRIGHTNESS:
			err = isx012_set_brightness(cfg_data.value_1);
			break;

		case EXT_CFG_SET_EFFECT:
			err = isx012_set_effect(cfg_data.value_1);
			break;	
			
		case EXT_CFG_SET_ISO:
			err = isx012_set_iso(cfg_data.value_1);
			break;
			
		case EXT_CFG_SET_WB:
			err = isx012_set_whitebalance(cfg_data.value_1);
			break;

		case EXT_CFG_SET_SCENE:
			err = isx012_set_scene(cfg_data.value_1);
			break;

		case EXT_CFG_SET_METERING:	// auto exposure mode
			err = isx012_set_metering(cfg_data.value_1);
			break;

		case EXT_CFG_SET_CONTRAST:
			err = isx012_set_contrast(cfg_data.value_1);
			break;

		case EXT_CFG_SET_SHARPNESS:
			err = isx012_set_sharpness(cfg_data.value_1);
			break;

		case EXT_CFG_SET_SATURATION:
			err = isx012_set_saturation(cfg_data.value_1);
			break;
		
		case EXT_CFG_SET_PREVIEW_SIZE:
			err = isx012_set_preview_size(cfg_data.value_1,cfg_data.value_2);
			break;

		case EXT_CFG_SET_PICTURE_SIZE:	
			err = isx012_set_picture_size(cfg_data.value_1);
			break;

		case EXT_CFG_SET_JPEG_QUALITY:	
			//err = isx012_set_jpeg_quality(cfg_data.value_1);
			break;
			
		case EXT_CFG_SET_FPS:
			//err = isx012_set_frame_rate(cfg_data.value_1,cfg_data.value_2);
			break;

		case EXT_CFG_SET_DTP:
			cam_info("DTP mode : %d",cfg_data.value_1);
			err = isx012_check_dataline(cfg_data.value_1);
			break;

		case EXT_CFG_SET_VT_MODE:
			cam_info("VTCall mode : %d",cfg_data.value_1);
			break;
			
		case EXT_CFG_SET_MOVIE_MODE:
			cam_info("MOVIE mode : %d",cfg_data.value_1);
			isx012_set_movie_mode(cfg_data.value_1);
			break;

		case EXT_CFG_SET_AF_OPERATION:
			cam_info("AF mode : %d",cfg_data.value_1);
#if 0		
			if (ctrl_info.address == 0) {
				ISX012_WRITE_LIST(ISX012_Halfrelease_Mode);
			} else if (ctrl_info.address == 1) {
				ctrl_info.value_1 = isx012_sensor_af_status();
				if (ctrl_info.value_1 == 1)
				 isx012_i2c_write_multi(0x0012, 0x10, 0x01);
			} else if (ctrl_info.address == 2) {
				ctrl_info.value_1 = isx012_sensor_af_result();
			}
#endif		
			break;

		case EXT_CFG_SET_AF_MODE :
			cam_info("Focus mode : %d",cfg_data.value_1);
			err = isx012_set_focus_mode(cfg_data.value_1);
			break;

		case EXT_CFG_SET_LOW_LEVEL :
			err = isx012_get_LowLightCondition();
			break;

		default:
			break;
	}

	if (copy_to_user((void *)argp, (const void *)&cfg_data, sizeof(cfg_data))){
		cam_err("fail copy_from_user!");
	}
	
	return err;	
}

int isx012_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	int err = 0;

	if (copy_from_user(&cfg_data,(void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CAM_DEBUG("cfgtype = %d, mode = %d", cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			err = isx012_set_sensor_mode(cfg_data.mode);
			break;
		
		default:
			err = 0;//-EFAULT;
			break;
	}

	return err;
}

int isx012_sensor_release(void)
{
	int err = 0;
	int temp = 0;

	CAM_DEBUG("POWER OFF START");

	gpio_tlmm_config(GPIO_CFG(50, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //reset
	gpio_tlmm_config(GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //stanby
	gpio_tlmm_config(GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(41, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(62, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);// flash EN
	gpio_tlmm_config(GPIO_CFG(63, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);// flash set

	gpio_set_value_cansleep(62, 0);//flash off
	gpio_set_value_cansleep(63, 0);// flash off

	gpio_set_value_cansleep(49, 0);
	temp = gpio_get_value(49);
	printk("[isx012] CAM_5M_STBY : %d\n", temp);
	mdelay(120);

	gpio_set_value_cansleep(50, 0);
	temp = gpio_get_value(50);
	printk("[isx012] CAM_5M_RST : %d\n", temp);

	cam_ldo_power_off();	// have to turn off MCLK before PMIC	

	isx012_ctrl->status.initialized = 0;
	DtpTest = 0;
	kfree(isx012_ctrl);
	
#ifdef CONFIG_LOAD_FILE
	isx012_regs_table_exit();
#endif
	CAM_DEBUG("POWER OFF END");

	return err;
}


 static int isx012_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	CAM_DEBUG("E");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENOTSUPP;
		goto probe_failure;
	}

	isx012_sensorw = kzalloc(sizeof(struct isx012_work_t), GFP_KERNEL);

	if (!isx012_sensorw) {
		err = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, isx012_sensorw);
	isx012_init_client(client);
	isx012_client = client;

#ifdef FACTORY_TEST
	camera_class = class_create(THIS_MODULE, "camera");                                                                                                                                                                                     
         
        if (IS_ERR(camera_class))
                pr_err("Failed to create class(camera)!\n");

	isx012_dev = device_create(camera_class, NULL, 0, NULL, "rear");
	if (IS_ERR(isx012_dev)) {
		pr_err("Failed to create device!");
		goto probe_failure;
	}

       if (device_create_file(isx012_dev, &dev_attr_type) < 0) {
                CDBG("failed to create device file, %s\n",
                        dev_attr_type.attr.name);
        }
       if (device_create_file(isx012_dev, &dev_attr_flash) < 0) {
                CDBG("failed to create device file, %s\n",
                        dev_attr_flash.attr.name);
        }
#endif
        
	CAM_DEBUG("E");

	return err;

probe_failure:
	kfree(isx012_sensorw);
	isx012_sensorw = NULL;
	cam_err("isx012_i2c_probe failed!");
	return err;
}


static int __exit isx012_i2c_remove(struct i2c_client *client)
{
	struct isx012_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);

#ifdef FACTORY_TEST
	device_remove_file(isx012_dev, &dev_attr_type);
	device_remove_file(isx012_dev, &dev_attr_flash);
#endif

//	i2c_detach_client(client);
	isx012_client = NULL;
	isx012_sensorw = NULL;
	kfree(sensorw);
	return 0;

}


static const struct i2c_device_id isx012_id[] = {
    { "isx012_i2c", 0 },
    { }
};

//PGH MODULE_DEVICE_TABLE(i2c, isx012);

static struct i2c_driver isx012_i2c_driver = {
	.id_table	= isx012_id,
	.probe  	= isx012_i2c_probe,
	.remove 	= __exit_p(isx012_i2c_remove),
	.driver 	= {
		.name = "isx012",
	},
};


int32_t isx012_i2c_init(void)
{
	int32_t err = 0;

	CAM_DEBUG("E");

	err = i2c_add_driver(&isx012_i2c_driver);

	if (IS_ERR_VALUE(err))
		goto init_failure;

	return err;



init_failure:
	cam_err("failed to isx012_i2c_init, err = %d", err);
	return err;
}


void isx012_exit(void)
{
	i2c_del_driver(&isx012_i2c_driver); 	
}


int isx012_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int err = 0;

	printk("############# isx012_sensor_probe ##############\n");
/*	struct msm_camera_sensor_info *info =
		(struct msm_camera_sensor_info *)dev; 

	struct msm_sensor_ctrl *s =
		(struct msm_sensor_ctrl *)ctrl;
*/

 
	err = isx012_i2c_init();
	if (err < 0)
		goto probe_done;

 	s->s_init	= isx012_sensor_open_init;
	s->s_release	= isx012_sensor_release;
	s->s_config	= isx012_sensor_config;
	s->s_ext_config	= isx012_sensor_ext_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = 0;

probe_done:
	cam_err("error, err = %d", err);
	return err;
	
}


static int __sec_isx012_probe(struct platform_device *pdev)
{
	printk("############# __sec_isx012_probe ##############\n");
	return msm_camera_drv_start(pdev, isx012_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __sec_isx012_probe,
	.driver = {
		.name = "msm_camera_isx012",
		.owner = THIS_MODULE,
	},
};

static int __init sec_isx012_camera_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

static void __exit sec_isx012_camera_exit(void)
{
	platform_driver_unregister(&msm_camera_driver);
}

module_init(sec_isx012_camera_init);
module_exit(sec_isx012_camera_exit);

