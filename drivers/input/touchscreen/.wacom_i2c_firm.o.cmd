cmd_drivers/input/touchscreen/wacom_i2c_firm.o := /home/gabe/arm-2009q3/arm-2009q3/bin/arm-none-linux-gnueabi-gcc -Wp,-MD,drivers/input/touchscreen/.wacom_i2c_firm.o.d  -nostdinc -isystem /home/gabe/arm-2009q3/arm-2009q3/bin/../lib/gcc/arm-none-linux-gnueabi/4.4.1/include -I/home/gabe/stock_note/dagkernel/arch/arm/include -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fomit-frame-pointer -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(wacom_i2c_firm)"  -D"KBUILD_MODNAME=KBUILD_STR(wacom_i2c_firm)" -D"DEBUG_HASH=22" -D"DEBUG_HASH2=42" -c -o drivers/input/touchscreen/wacom_i2c_firm.o drivers/input/touchscreen/wacom_i2c_firm.c

deps_drivers/input/touchscreen/wacom_i2c_firm.o := \
  drivers/input/touchscreen/wacom_i2c_firm.c \
    $(wildcard include/config/kor/model/shv/e160s.h) \
    $(wildcard include/config/kor/model/shv/e160k.h) \
    $(wildcard include/config/kor/model/shv/e160l.h) \
    $(wildcard include/config/jpn/model/sc/05d.h) \
    $(wildcard include/config/usa/model/sgh/i717.h) \
  drivers/input/touchscreen/wacom_i2c_firm_P6_REV02.h \
  drivers/input/touchscreen/wacom_i2c_firm_P6_REV05.h \

drivers/input/touchscreen/wacom_i2c_firm.o: $(deps_drivers/input/touchscreen/wacom_i2c_firm.o)

$(deps_drivers/input/touchscreen/wacom_i2c_firm.o):
