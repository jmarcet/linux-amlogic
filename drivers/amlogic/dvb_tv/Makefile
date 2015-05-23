#
# Makefile for the DVB driver.
#

obj-$(CONFIG_AM_DVB) += aml.o

aml-objs=aml_dvb.o aml_dmx.o

obj-$(CONFIG_AM_DVB) += aml_fe.o

obj-$(CONFIG_AM_CI) += amlci/

obj-$(CONFIG_AM_DTVDEMOD) += amldemod/

obj-$(CONFIG_AM_ATVDEMOD) += amlatvdemod/

#obj-$(CONFIG_AM_ATBM8869) += atbm887x/

#obj-$(CONFIG_AM_SI2157) += si2157/

#obj-$(CONFIG_AM_SI2177) += si2177/

#obj-$(CONFIG_AM_SI2168) += si2168/

#obj-$(CONFIG_AM_AVL6211) += avl6211/

#obj-$(CONFIG_AM_AVL68xx) += avl68xx/

#obj-$(CONFIG_AM_MXL101) += mxl101/

#obj-$(CONFIG_AM_MXL241_FE) += mxl241/

#obj-$(CONFIG_AM_MXL683_FE) += mxl683/

#obj-$(CONFIG_AM_R840) += r840/

#obj-$(CONFIG_AM_SI2151) += si2151/

EXTRA_CFLAGS += -I.
