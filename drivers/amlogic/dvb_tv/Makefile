#
# Makefile for the DVB driver.
#

obj-$(CONFIG_AM_DVB) += aml.o

aml-objs=aml_dvb.o aml_dmx.o

obj-$(CONFIG_AM_DVB) += aml_fe.o

obj-$(CONFIG_AM_CI) += amlci/

obj-$(CONFIG_AM_DTVDEMOD) += amldemod/

obj-$(CONFIG_AM_ATVDEMOD) += amlatvdemod/

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/altobeam/atbm887x),)
	obj-$(CONFIG_AM_ATBM8869) += drivers/amlogic/dvb_tv/altobeam/atbm887x/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/silabs/si2157),)
	obj-$(CONFIG_AM_SI2157) += drivers/amlogic/dvb_tv/silabs/si2157/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/silabs/si2177),)
	obj-$(CONFIG_AM_SI2177) += drivers/amlogic/dvb_tv/silabs/si2177/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/silabs/si2168),)
	obj-$(CONFIG_AM_SI2168) += drivers/amlogic/dvb_tv/silabs/si2168/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/availink/avl6211),)
	obj-$(CONFIG_AM_AVL6211) += drivers/amlogic/dvb_tv/availink/avl6211/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/availink/avl68xx),)
	obj-$(CONFIG_AM_AVL68xx) += drivers/amlogic/dvb_tv/availink/avl68xx/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/maxlinear/mxl101),)
	obj-$(CONFIG_AM_MXL101) += drivers/amlogic/dvb_tv/maxlinear/mxl101/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/maxlinear/drivers/mxl241),)
	obj-$(CONFIG_AM_MXL241_FE) += drivers/amlogic/dvb_tv/maxlinear/drivers/mxl241/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/maxlinear/drivers/mxl683),)
	obj-$(CONFIG_AM_MXL683_FE) += drivers/amlogic/dvb_tv/maxlinear/drivers/mxl683/
endif

ifneq ($(wildcard $(srctree)/drivers/amlogic/dvb_tv/rafael/drivers/r840),)
	obj-$(CONFIG_AM_R840) += drivers/amlogic/dvb_tv/rafael/drivers/r840/
endif

EXTRA_CFLAGS += -I.
