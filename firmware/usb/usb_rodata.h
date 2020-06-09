#ifndef USB_RODATA_H
#define USB_RODATA_H

static unsigned char ReportDescriptor[] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  0x05, 0x01,//Usage Page (Generic Desktop)
  0x09, 0x06,//Usage (Keyboard)
  0xA1, 0x01,//Collection (Application)
  0x85, 0x01,//Report ID 1
  
  //modifier byte
  0x05, 0x07,//Usage Page (Keyboard / Keypad)
  0x19, 0xE0,//Usage Minimum (224)
  0x29, 0xE7,//Usage Maximum (231)
  0x15, 0x00,//Logical Minimum (0)
  0x25, 0x01,//Logical Maximum (1)
  0x75, 0x01,//Report Size (1)
  0x95, 0x08,//Report Count (8)
  0x81, 0x02,//Input (Variable, Absolute)
  
  //reserved byte
  0x75, 0x08,//Report Size (8)
  0x95, 0x01,//Report Count (1)
  0x81, 0x01,//Input (Constant, Absolute)

  //LED states
  0x05, 0x08,//Usage Page (LED)
  0x19, 0x01,//Usage Minimum (1)
  0x29, 0x05,//Usage Maximum (5)
  0x75, 0x01,//Report Size (1)
  0x95, 0x05,//Report Count (5)
  0x91, 0x02,//Output (Variable, Absolute)
  
  //LED state padding bits
  0x75, 0x01,//Report Size (1)
  0x95, 0x03,//Report Count (3)
  0x91, 0x01,//Output (Constant, Absolute)
  
  //pressed keys
  0x05, 0x07,//Usage Page (Keyboard / Keypad)
  0x19, 0x00,//Usage Minimum (0)
  0x29, 0xDD,//Usage Maximum (221)
  0x15, 0x00,//Logical Minimum (0)
  0x25, 0x65,//Logical Maximum (101)
  0x75, 0x08,//Report Size (8)
  0x95, 0x06,//Report Count (6)    
  0x81, 0x00,//Input (Array, Absolute)
  
  0xC0,       //End Collection
  
  //------------------------------------------
  
  0x05, 0x01,//Usage Page (Generic Desktop)
  0x09, 0x02,//Usage (Mouse)
  0xA1, 0x01,//Collection (Application)
  0x85, 0x02,//Report ID 2
  
  0x09, 0x01,//Usage (Pointer)
  0xA1, 0x00,//Collection (Physical)
  
  0x05, 0x09,//Usage Page (Buttons)
  0x19, 0x01,//Usage Minimum (1)
  0x29, 0x03,//Usage Maximum (3)
  0x15, 0x00,//Logical Minimum (0)
  0x25, 0x01,//Logical Maximum (1)
  0x75, 0x01,//Report Size (1)
  0x95, 0x03,//Report Count (3)
  0x81, 0x02,//Input (Variable, Absolute)
  
  //reserved bits
  0x75, 0x01,//Report Size (1)
  0x95, 0x05,//Report Count (5)
  0x81, 0x01,//Input (Constant, Absolute)
  
  0x05, 0x01,//Usage Page (Generic Desktop)
  0x09, 0x30,//Usage (X axis)
  0x09, 0x31,//Usage (Y axis)
  0x09, 0x38,//Usage (Wheel)
  0x75, 0x08,//Report Size (8)
  0x95, 0x03,//Report Count (3)
  0x15, 0x81,//Logical Minimum (-127)
  0x25, 0x7F,//Logical Maximum (+127)
  0x81, 0x06,//Input (Variable, Relative)
  
  0xC0,       //End Collection
  0xC0        //End Collection  
};

//device descriptor is not "static" and not in ".rodata" section to allow for dynamic VID/PID change
DeviceDescriptor_TypeDef DeviceDescriptor __attribute__(( aligned(2) )) =
{
  .bLength            = sizeof(DeviceDescriptor_TypeDef),
  .bDescriptorType    = 0x01,//DEVICE descriptor type
  .bcdUSB             = 0x0200,//USB 2.0
  .bDeviceClass       = 0,//class is specified at interface level
  .bDeviceSubClass    = 0,//class is specified at interface level
  .bDeviceProtocol    = 0,//protocol is specified at interface level
  .bMaxPacketSize0    = MAXPACKET_0,//max packet size for EP0 (default = 64 bytes)
  .idVendor           = 0x1C4F,//SiGma Micro
  .idProduct          = 0x0026,//keyboard
  .bcdDevice          = 0x0130,//device version 1.3
  .iManufacturer      = 0,//no symbolic name reserved in string descriptor
  .iProduct           = 0,//no symbolic name reserved in string descriptor
  .iSerialNumber      = 1,//serial number is in StringDescriptor_1
  .bNumConfigurations = 1//only one configuration supported
};

static GetConfigResponse_default_TypeDef GetConfigResponse_default __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  .ConfigurationDescriptor_1 =
  {
    .bLength                = sizeof(ConfigurationDescriptor_TypeDef),
    .bDescriptorType        = 0x02,//CONFIGURATION descriptor type
    .wTotalLength           = sizeof(GetConfigResponse_default_TypeDef),
    .bNumInterfaces         = 2,//2 interfaces supported in this configuration
    .bConfigurationValue    = 1,//configuration number is 1
    .iConfiguration         = 0,//no symbolic name reserved in string descriptor
    .bmAttributes           = 0x80,//not self-powered, no remote wakeup support
    .bMaxPower              = 20//20 * 2mA = 40mA max current consumption
  },
  .InterfaceDescriptor_0 =
  {
    .bLength                = sizeof(InterfaceDescriptor_TypeDef),
    .bDescriptorType        = 0x04,//INTERFACE descriptor type
    .bInterfaceNumber       = 0,//interface number is 0
    .bAlternateSetting      = 0,//alternate setting 0
    .bNumEndpoints          = 2,//two endpoints in this interface
    .bInterfaceClass        = 0x03,//HID class
    .bInterfaceSubClass     = 0x01,//boot protocol is supported
    .bInterfaceProtocol     = 0x01,//device can act as a boot keyboard
    .iInterface             = 0x00//no symbolic name reserved in string descriptor
  },
  .HIDdescriptor =
  {
    .bLength = sizeof(HIDdescriptor_TypeDef),
    .bDescriptorType = 0x21,//HID descriptor type
    .bcdHID = 0x0110,//HID specification version 1.1
    .bCountryCode = 0x00,//no target country
    .bNumDescriptors = 1,//only one extra class specific descriptor (REPORT)
    .bDescriptorType_ClassSpecific_1 = 0x22,//REPORT descriptor type
    .wDescriptorLength_ClassSpecific_1 = sizeof(ReportDescriptor)
  },
  .EndpointDescriptor_1_IN =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x81,//EP1_IN endpoint
    .bmAttributes           = 0x03,//INTERRUPT endpoint
    .wMaxPacketSize         = 32,//32 bytes maximum packet size
    .bInterval              = 0x01//poll this endpoint every 1ms
  },
  .EndpointDescriptor_1_OUT =
  {
   .bLength                = sizeof(EndpointDescriptor_TypeDef),
   .bDescriptorType        = 0x05,//ENDPOINT descriptor type
   .bEndpointAddress       = 0x01,//EP1_OUT endpoint
   .bmAttributes           = 0x03,//INTERRUPT endpoint
   .wMaxPacketSize         = 32,//32 bytes maximum packet size
   .bInterval              = 0x14//poll this endpoint every 20ms
  },
  .InterfaceDescriptor_1 =
  {
    .bLength                = sizeof(InterfaceDescriptor_TypeDef),
    .bDescriptorType        = 0x04,//INTERFACE descriptor type
    .bInterfaceNumber       = 1,//interface number is 1
    .bAlternateSetting      = 0,//alternate setting 0
    .bNumEndpoints          = 2,//two endpoints in this interface
    .bInterfaceClass        = 0x08,//MSD class
    .bInterfaceSubClass     = 0x06,//SCSI transparent command set
    .bInterfaceProtocol     = 0x50,//BULK-ONLY transport
    .iInterface             = 0x00//no symbolic name reserved in string descriptor
  },
  .EndpointDescriptor_2_OUT =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x02,//EP2_OUT endpoint
    .bmAttributes           = 0x02,//BULK endpoint
    .wMaxPacketSize         = MAXPACKET_MSD,//max packet size for EP2 (default = 64 bytes)
    .bInterval              = 0x00//this field is not used
  },
  .EndpointDescriptor_3_IN =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x83,//EP3_IN endpoint
    .bmAttributes           = 0x02,//BULK endpoint
    .wMaxPacketSize         = MAXPACKET_MSD,//max packet size for EP3 (default = 64 bytes)
    .bInterval              = 0x00//this field is not used
  }
  
};

static GetConfigResponse_HIDonly_TypeDef GetConfigResponse_HIDonly __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  .ConfigurationDescriptor_1 =
  {
    .bLength                = sizeof(ConfigurationDescriptor_TypeDef),
    .bDescriptorType        = 0x02,//CONFIGURATION descriptor type
    .wTotalLength           = sizeof(GetConfigResponse_HIDonly_TypeDef),
    .bNumInterfaces         = 1,//1 interface supported in this configuration
    .bConfigurationValue    = 1,//configuration number is 1
    .iConfiguration         = 0,//no symbolic name reserved in string descriptor
    .bmAttributes           = 0x80,//not self-powered, no remote wakeup support
    .bMaxPower              = 20//20* 2mA = 40mA max current consumption
  },
  .InterfaceDescriptor_0 =
  {
    .bLength                = sizeof(InterfaceDescriptor_TypeDef),
    .bDescriptorType        = 0x04,//INTERFACE descriptor type
    .bInterfaceNumber       = 0,//interface number is 0
    .bAlternateSetting      = 0,//alternate setting 0
    .bNumEndpoints          = 2,//two endpoints in this interface
    .bInterfaceClass        = 0x03,//HID class
    .bInterfaceSubClass     = 0x01,//boot protocol is supported
    .bInterfaceProtocol     = 0x01,//device can act as a boot keyboard
    .iInterface             = 0x00//no symbolic name reserved in string descriptor
  },
  .HIDdescriptor =
  {
    .bLength = sizeof(HIDdescriptor_TypeDef),
    .bDescriptorType = 0x21,//HID descriptor type
    .bcdHID = 0x0110,//HID specification version 1.1
    .bCountryCode = 0x00,//no target country
    .bNumDescriptors = 1,//only one extra class specific descriptor (REPORT)
    .bDescriptorType_ClassSpecific_1 = 0x22,//REPORT descriptor type
    .wDescriptorLength_ClassSpecific_1 = sizeof(ReportDescriptor)
  },
  .EndpointDescriptor_1_IN =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x81,//EP1_IN endpoint
    .bmAttributes           = 0x03,//INTERRUPT endpoint
    .wMaxPacketSize         = 32,//32 bytes maximum packet size
    .bInterval              = 0x01//poll this endpoint every 1ms
  },
  .EndpointDescriptor_1_OUT =
  {
   .bLength                = sizeof(EndpointDescriptor_TypeDef),
   .bDescriptorType        = 0x05,//ENDPOINT descriptor type
   .bEndpointAddress       = 0x01,//EP1_OUT endpoint
   .bmAttributes           = 0x03,//INTERRUPT endpoint
   .wMaxPacketSize         = 32,//32 bytes maximum packet size
   .bInterval              = 0x14//poll this endpoint every 20ms
  }
  
};

static unsigned short StringDescriptor_0[2]  __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) = { 0x0304, 0x0409 };//LANGID list; only english (united states) is supported
       unsigned short StringDescriptor_1[13] = { 0x031A, '1', '3', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1' };//iSerial
//static unsigned short StringDescriptor_2[5]  __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) = { 0x030A, 'U', 'S', 'B', ' ' };//iManufacturer
//static unsigned short StringDescriptor_3[12] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) = { 0x0318, 'F', 'L', 'A', 'S', 'H', ' ', 'D', 'R', 'I', 'V', 'E' };//iProduct


#endif //USB_RODATA_H
