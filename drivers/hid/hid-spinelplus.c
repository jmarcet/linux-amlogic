/*
 *  HID driver for "PHILIPS MCE USB IR Receiver- Spinel plus" remotes
 *
 *  Copyright (c) 2010 Panagiotis Skintzos
 *
 *  Renamed to Spinel, cleanup and modified to also support
 *  Spinel Plus 0471:20CC by Stephan Raue 2012.
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/device.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/module.h>

#include "hid-ids.h"

#define spinelplus_map_key(c)	set_bit(EV_REP, hi->input->evbit); \
						hid_map_usage_clear(hi, usage, bit, max, EV_KEY, (c))

static int spinelplus_input_mapping(struct hid_device *hdev,
		struct hid_input *hi, struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	switch (usage->hid) {
	case 0xffbc000d:	spinelplus_map_key(KEY_MEDIA);		break;
	case 0xffbc0024:	spinelplus_map_key(KEY_MEDIA);		break;
	case 0xffbc0027:	spinelplus_map_key(KEY_ZOOM);		break;
	case 0xffbc0033:	spinelplus_map_key(KEY_HOME);		break;
	case 0xffbc0035:	spinelplus_map_key(KEY_CAMERA);		break;
	case 0xffbc0036:	spinelplus_map_key(KEY_EPG);		break;
	case 0xffbc0037:	spinelplus_map_key(KEY_DVD);		break;
	case 0xffbc0038:	spinelplus_map_key(KEY_HOME);		break;
	case 0xffbc0039:	spinelplus_map_key(KEY_MP3);		break;
	case 0xffbc003a:	spinelplus_map_key(KEY_VIDEO);		break;
	case 0xffbc005a:	spinelplus_map_key(KEY_TEXT);		break;
	case 0xffbc005b:	spinelplus_map_key(KEY_RED);		break;
	case 0xffbc005c:	spinelplus_map_key(KEY_GREEN);		break;
	case 0xffbc005d:	spinelplus_map_key(KEY_YELLOW);		break;
	case 0xffbc005e:	spinelplus_map_key(KEY_BLUE);		break;
	default:
		return 0;
	}
	return 1;
}

static int spinelplus_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int ret;
	/* Connect only to hid input (not hiddev & hidraw)*/
	unsigned int cmask = HID_CONNECT_HIDINPUT;

	ret = hid_parse(hdev);
	if (ret) {
		dev_err(&hdev->dev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, cmask);
	if (ret) {
		dev_err(&hdev->dev, "hw start failed\n");
		goto err_free;
	}

	return 0;
err_free:
	return ret;
}

static const struct hid_device_id spinelplus_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_PHILIPS,USB_DEVICE_ID_PHILIPS_SPINEL_PLUS_1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_PHILIPS,USB_DEVICE_ID_PHILIPS_SPINEL_PLUS_2) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_PHILIPS,USB_DEVICE_ID_PHILIPS_SPINEL_PLUS_3) },
	{ }
};
MODULE_DEVICE_TABLE(hid, spinelplus_devices);

static struct hid_driver spinelplus_driver = {
	.name = 		"SpinelPlus",
	.id_table = 		spinelplus_devices,
	.input_mapping = 	spinelplus_input_mapping,
	.probe = 		spinelplus_probe,
};

static int __init spinelplus_init(void)
{
	return hid_register_driver(&spinelplus_driver);
}

static void __exit spinelplus_exit(void)
{
	hid_unregister_driver(&spinelplus_driver);
}

module_init(spinelplus_init);
module_exit(spinelplus_exit);
MODULE_LICENSE("GPL");
