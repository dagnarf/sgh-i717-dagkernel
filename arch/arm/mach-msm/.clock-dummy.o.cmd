cmd_arch/arm/mach-msm/clock-dummy.o := /home/gabe/arm-2009q3/arm-2009q3/bin/arm-none-linux-gnueabi-gcc -Wp,-MD,arch/arm/mach-msm/.clock-dummy.o.d  -nostdinc -isystem /home/gabe/arm-2009q3/arm-2009q3/bin/../lib/gcc/arm-none-linux-gnueabi/4.4.1/include -I/home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(clock_dummy)"  -D"KBUILD_MODNAME=KBUILD_STR(clock_dummy)" -D"DEBUG_HASH=25" -D"DEBUG_HASH2=43" -c -o arch/arm/mach-msm/clock-dummy.o arch/arm/mach-msm/clock-dummy.c

deps_arch/arm/mach-msm/clock-dummy.o := \
  arch/arm/mach-msm/clock-dummy.c \
    $(wildcard include/config/arch/msm8960.h) \
  arch/arm/mach-msm/clock.h \
    $(wildcard include/config/arch/msm7x30.h) \
    $(wildcard include/config/arch/msm8x60.h) \
    $(wildcard include/config/debug/fs.h) \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/posix_types.h \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  include/linux/prefetch.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/processor.h \
    $(wildcard include/config/mmu.h) \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
    $(wildcard include/config/smp.h) \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/hwcap.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/system.h \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/arch/has/barriers.h) \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/32v6k.h) \
  include/linux/linkage.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/linkage.h \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/irqflags.h \
  /home/gabe/stock_note/sgh-i717-dagkernel/arch/arm/include/asm/outercache.h \
    $(wildcard include/config/outer/cache/sync.h) \
    $(wildcard include/config/outer/cache.h) \
  include/asm-generic/cmpxchg-local.h \
  arch/arm/mach-msm/include/mach/clk.h \

arch/arm/mach-msm/clock-dummy.o: $(deps_arch/arm/mach-msm/clock-dummy.o)

$(deps_arch/arm/mach-msm/clock-dummy.o):
