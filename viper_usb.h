#ifndef VIPER_USB_H_9f3ffd7f_9376_4f82_94a4_f5a87e1a5922
#define VIPER_USB_H_9f3ffd7f_9376_4f82_94a4_f5a87e1a5922

#include <stdint.h>
#include "libusb.h"


class viper_usb
{
public:
	viper_usb();
	~viper_usb();

	uint32_t usb_connect();
	void usb_disconnect();

	uint32_t usb_send_cmd(uint8_t* cmd, uint32_t cmd_len);
	uint32_t usb_rec_resp( uint8_t* resp, uint32_t rec_len);
       

private:

	libusb_device_handle* m_usbdev;
	int find_device(libusb_device*);

};



#endif
