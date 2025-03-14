
#include <iostream>
#include "viper_usb.h"
#include <string.h>
#include <time.h>

using namespace std;

#define IN_EP	0x81
#define OUT_EP	0x02



viper_usb::viper_usb()
{
	m_usbdev = NULL;
}


viper_usb::~viper_usb()
{
}


uint32_t viper_usb::usb_connect() {

	if (libusb_init(NULL) < 0) {
	  cout<<"Unable to initialize USB\n";
		return -1;
	}

	libusb_device** list;
	libusb_device* found = NULL;


	ssize_t cnt = libusb_get_device_list(NULL, &list);
	ssize_t i = 0;

	if (cnt < 0) {
	  cout<<"Error getting USB device list\n";
		libusb_exit(NULL);
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		if (find_device(list[i])) {
			found = list[i];
			break;
		}
	}

	if (!found) {
	  cout<<"Could not find device\n";
		libusb_free_device_list(list, 1);
		libusb_exit(NULL);
		return -1;
	}

	// found device so open
	
	if (libusb_open(found, &m_usbdev) != 0) {
	  cout<<"Error opening device\n";
		m_usbdev = NULL;
		return -1;
	}

	libusb_claim_interface(m_usbdev, 0);
	libusb_free_device_list(list, 1);


	return 0;
}


int viper_usb::find_device(libusb_device* dev) {

	libusb_device_descriptor desc;
	int rv;

	rv = libusb_get_device_descriptor(dev, &desc);
	if (rv != 0)
		return -100;

	//	return ((desc.idVendor == 0x03eb) && (desc.idProduct == 0x2426));
	return ((desc.idVendor == 0x0f44) && (desc.idProduct == 0xbf01));


}

void viper_usb::usb_disconnect() {

	if (m_usbdev) {
		libusb_release_interface(m_usbdev, 0);
		libusb_close(m_usbdev);
		libusb_exit(0);
	}
}



uint32_t viper_usb::usb_send_cmd(uint8_t* cmd, uint32_t cmd_len){

  uint32_t br, rv;
  
  rv = libusb_bulk_transfer(m_usbdev, OUT_EP, cmd, cmd_len, (int*)&br, 1);
  
  return rv;


}


uint32_t viper_usb::usb_rec_resp( uint8_t* resp, uint32_t rec_len){

  uint32_t br;
  // const char* p;
  //int i;

  br=0;
  libusb_bulk_transfer(m_usbdev, IN_EP, resp, rec_len,(int*) &br, 1);
  

  return br;
}
