obj-$(CONFIG_MSM8X60_RTAC) += rtac.o
obj-y += audio_dev_ctl.o
ifdef CONFIG_USA_MODEL_SGH_I717
obj-y += board-msm8x60-audio-Q1.o
else ifdef CONFIG_USA_MODEL_SGH_I757
obj-y += board-msm8x60-audio-CELOXHD.o
else ifdef CONFIG_USA_MODEL_SGH_T769
obj-y += board-msm8x60-audio-APEX40.o
else ifdef CONFIG_USA_MODEL_SGH_I577
obj-y += board-msm8x60-audio-A2.o
else
obj-y += board-msm8x60-audio.o
endif
obj-$(CONFIG_TIMPANI_CODEC) += snddev_icodec.o
obj-y += snddev_ecodec.o snddev_mi2s.o snddev_virtual.o
obj-y += apr.o apr_tal.o q6core.o dsp_debug.o
obj-y += audio_acdb.o
obj-y += q6asm.o q6adm.o q6afe.o
obj-y += pcm_out.o pcm_in.o fm.o
obj-y += audio_lpa.o
obj-y += q6voice.o
obj-y += snddev_hdmi.o
obj-y += aac_in.o qcelp_in.o evrc_in.o amrnb_in.o audio_utils.o
obj-y += audio_mvs.o
obj-y += audio_wma.o audio_wmapro.o audio_aac.o
obj-$(CONFIG_FB_MSM_HDMI_MSM_PANEL) += lpa_if_hdmi.o
