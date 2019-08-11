#ifndef USB_RODATA_H
#define USB_RODATA_H

static unsigned char ReportDescriptor[] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  0x05, 0x01,//Usage Page (Generic Desktop)
  0x09, 0x06,//Usage (Keyboard)
  0xA1, 0x01,//Collection (Application)
  
  0x05, 0x07,//Usage Page (Keyboard / Keypad)
  
  //modifier byte
  0x19, 0xE0,//Usage Minimum (224)
  0x29, 0xE7,//Usage Maximum (231)
  0x15, 0x00,//Logical Minimum (0)
  0x25, 0x01,//Logical Maximum (1)
  0x75, 0x01,//Report Size (1)
  0x95, 0x08,//Report Count (8)
  0x81, 0x02,//Input (Variable)
  
  //reserved byte
  0x95, 0x01,//Report Count (1)
  0x75, 0x08,//Report Size (8)
  0x81, 0x01,//Input (Constant)
  
  //pressed keys
  0x19, 0x00,//Usage Minimum (0)
  0x29, 0x65,//Usage Maximum (101)
  0x15, 0x00,//Logical Minimum (0)
  0x25, 0x65,//Logical Maximum (101)
  0x75, 0x08,//Report Size (8)
  0x95, 0x06,//Report Count (6)    
  0x81, 0x00,//Input (Array)
  
  0xC0       //End Collection    
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
  .bcdDevice          = 0x0100,//device version 1.0
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
    .bNumEndpoints          = 1,//one endpoint in this interface
    .bInterfaceClass        = 0x03,//HID class
    .bInterfaceSubClass     = 0x00,//no subclass
    .bInterfaceProtocol     = 0x00,//no boot protocol used
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
  .EndpointDescriptor_1 =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x81,//EP1_IN endpoint
    .bmAttributes           = 0x03,//INTERRUPT endpoint
    .wMaxPacketSize         = 8,//8 bytes maximum packet size
    .bInterval              = 0x01//poll this endpoint every 1ms
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
  .EndpointDescriptor_2 =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x02,//EP2_OUT endpoint
    .bmAttributes           = 0x02,//BULK endpoint
    .wMaxPacketSize         = MAXPACKET_MSD,//max packet size for EP2 (default = 64 bytes)
    .bInterval              = 0x00//this field is not used
  },
  .EndpointDescriptor_3 =
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
    .bNumEndpoints          = 1,//one endpoint in this interface
    .bInterfaceClass        = 0x03,//HID class
    .bInterfaceSubClass     = 0x00,//no subclass
    .bInterfaceProtocol     = 0x00,//no boot protocol used
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
  .EndpointDescriptor_1 =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x81,//EP1_IN endpoint
    .bmAttributes           = 0x03,//INTERRUPT endpoint
    .wMaxPacketSize         = 8,//8 bytes maximum packet size
    .bInterval              = 0x01//poll this endpoint every 1ms
  },
  
};

static GetConfigResponse_MSDonly_TypeDef GetConfigResponse_MSDonly __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  .ConfigurationDescriptor_1 =
  {
    .bLength                = sizeof(ConfigurationDescriptor_TypeDef),
    .bDescriptorType        = 0x02,//CONFIGURATION descriptor type
    .wTotalLength           = sizeof(GetConfigResponse_MSDonly_TypeDef),
    .bNumInterfaces         = 1,//1 interface supported in this configuration
    .bConfigurationValue    = 1,//configuration number is 1
    .iConfiguration         = 0,//no symbolic name reserved in string descriptor
    .bmAttributes           = 0x80,//not self-powered, no remote wakeup support
    .bMaxPower              = 20//20 * 2mA = 40mA max current consumption
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
  .EndpointDescriptor_2 =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x02,//EP2_OUT endpoint
    .bmAttributes           = 0x02,//BULK endpoint
    .wMaxPacketSize         = MAXPACKET_MSD,//max packet size for EP2 (default = 64 bytes)
    .bInterval              = 0x00//this field is not used
  },
  .EndpointDescriptor_3 =
  {
    .bLength                = sizeof(EndpointDescriptor_TypeDef),
    .bDescriptorType        = 0x05,//ENDPOINT descriptor type
    .bEndpointAddress       = 0x83,//EP3_IN endpoint
    .bmAttributes           = 0x02,//BULK endpoint
    .wMaxPacketSize         = MAXPACKET_MSD,//max packet size for EP3 (default = 64 bytes)
    .bInterval              = 0x00//this field is not used
  }
  
};

//string descriptors. only english (0x0409) is present in supported LANGID list
static unsigned short StringDescriptor_0[2] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) = { 0x0304, 0x0409 };
static unsigned short StringDescriptor_1[13] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) = { 0x031A, '1', '2', '0', '0', '0', '0', '0', '0', '0', '0', '0', '3' };

#endif //USB_RODATA_H
