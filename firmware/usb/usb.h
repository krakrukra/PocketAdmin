#ifndef USB_H
#define USB_H

//externally visible functions
void usb_init();
void usb_reset();
void usb_handler() __attribute__((interrupt));
void bufferCopy(unsigned short* whereFrom, unsigned short* whereTo, unsigned short howMuch);

//----------------------------------------------------------------------------------------------------------------------

//types and macros related to STM32F072C8T6 USB peripheral
typedef struct
{
  volatile unsigned int EP0R;//offset 0x00
  volatile unsigned int EP1R;//offset 0x04
  volatile unsigned int EP2R;//offset 0x08
  volatile unsigned int EP3R;//offset 0x0C
  volatile unsigned int EP4R;//offset 0x10
  volatile unsigned int EP5R;//offset 0x14
  volatile unsigned int EP6R;//offset 0x18
  volatile unsigned int EP7R;//offset 0x1C
  volatile unsigned int RESERVED[8];//offset 0x20, size 32 bytes
  volatile unsigned int CNTR;//offset 0x40
  volatile unsigned int ISTR;//offset 0x44
  volatile unsigned int FNR;//offset 0x48
  volatile unsigned int DADDR;//offset 0x4C
  volatile unsigned int BTABLE;//offset 0x50
  volatile unsigned int LPMCSR;//offset 0x54
  volatile unsigned int BCDR;//offset 0x58
} USB_TypeDef;

//base address of USB peripheral is 0x40005C00
#define USB_BaseAddr 0x40005C00U
#define USB ((USB_TypeDef*) USB_BaseAddr)

typedef struct
{
  volatile unsigned short ADDR0_TX;
  volatile unsigned short COUNT0_TX;
  volatile unsigned short ADDR0_RX;
  volatile unsigned short COUNT0_RX;
  volatile unsigned short ADDR1_TX;
  volatile unsigned short COUNT1_TX;
  volatile unsigned short ADDR1_RX;
  volatile unsigned short COUNT1_RX;
  volatile unsigned short ADDR2_TX;
  volatile unsigned short COUNT2_TX;
  volatile unsigned short ADDR2_RX;
  volatile unsigned short COUNT2_RX;
  volatile unsigned short ADDR3_TX;
  volatile unsigned short COUNT3_TX;
  volatile unsigned short ADDR3_RX;
  volatile unsigned short COUNT3_RX;
  volatile unsigned short ADDR4_TX;
  volatile unsigned short COUNT4_TX;
  volatile unsigned short ADDR4_RX;
  volatile unsigned short COUNT4_RX;
  volatile unsigned short ADDR5_TX;
  volatile unsigned short COUNT5_TX;
  volatile unsigned short ADDR5_RX;
  volatile unsigned short COUNT5_RX;
  volatile unsigned short ADDR6_TX;
  volatile unsigned short COUNT6_TX;
  volatile unsigned short ADDR6_RX;
  volatile unsigned short COUNT6_RX;
  volatile unsigned short ADDR7_TX;
  volatile unsigned short COUNT7_TX;
  volatile unsigned short ADDR7_RX;
  volatile unsigned short COUNT7_RX;
} BTABLE_TypeDef;

//base address of PMA buffer description table is 0x40006000 (if USB->BTABLE == 0)
#define BTABLE_BaseAddr 0x40006000U
#define BTABLE ((BTABLE_TypeDef*) BTABLE_BaseAddr)

//----------------------------------------------------------------------------------------------------------------------

//types, macros for descriptor structures

//MaxPacketSize of EP0
#define MAXPACKET_0 64
//MaxPacketSize of MSD interface endpoints
#define MAXPACKET_MSD 64

typedef struct
{
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned short bcdUSB;
  unsigned char bDeviceClass;
  unsigned char bDeviceSubClass;
  unsigned char bDeviceProtocol;
  unsigned char bMaxPacketSize0;
  unsigned short idVendor;
  unsigned short idProduct;
  unsigned short bcdDevice;
  unsigned char iManufacturer;
  unsigned char iProduct;
  unsigned char iSerialNumber;
  unsigned char bNumConfigurations;
} DeviceDescriptor_TypeDef;

typedef struct
{
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned short wTotalLength;
  unsigned char bNumInterfaces;
  unsigned char bConfigurationValue;
  unsigned char iConfiguration;
  unsigned char bmAttributes;
  unsigned char bMaxPower;
} __attribute__(( packed )) ConfigurationDescriptor_TypeDef;

typedef struct
{
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned char bInterfaceNumber;
  unsigned char bAlternateSetting;
  unsigned char bNumEndpoints;
  unsigned char bInterfaceClass;
  unsigned char bInterfaceSubClass;
  unsigned char bInterfaceProtocol;
  unsigned char iInterface;
} __attribute__(( packed )) InterfaceDescriptor_TypeDef;

//this is a HID class specific descriptor
typedef struct
{
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned short bcdHID;
  unsigned char bCountryCode;
  unsigned char bNumDescriptors;
  unsigned char bDescriptorType_ClassSpecific_1;
  unsigned short wDescriptorLength_ClassSpecific_1;
  //if necessary you can add pairs of fields with ClassSpecific_2, ClassSpecific_3, etc
} __attribute__(( packed )) HIDdescriptor_TypeDef;

typedef struct
{
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned char bEndpointAddress;
  unsigned char bmAttributes;
  unsigned short wMaxPacketSize;
  unsigned char bInterval;
} __attribute__(( packed )) EndpointDescriptor_TypeDef;

//one of the following structures will be returned when GetConfigurationDescriptor request is
//sent by the host. change this structure if it is necessary to use different set of descriptors.
// ! when initializing this structure make sure it is aligned at 2 byte boundary !
typedef struct
{
  ConfigurationDescriptor_TypeDef ConfigurationDescriptor_1;
  InterfaceDescriptor_TypeDef InterfaceDescriptor_0;//HID interface
  HIDdescriptor_TypeDef HIDdescriptor;//HID descriptor
  EndpointDescriptor_TypeDef EndpointDescriptor_1;//HID endpoint
  InterfaceDescriptor_TypeDef InterfaceDescriptor_1;//MSD interface
  EndpointDescriptor_TypeDef EndpointDescriptor_2;//MSD endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_3;//MSD endpoint
} __attribute__(( packed )) GetConfigResponse_default_TypeDef;

typedef struct
{
  ConfigurationDescriptor_TypeDef ConfigurationDescriptor_1;
  InterfaceDescriptor_TypeDef InterfaceDescriptor_0;//HID interface
  HIDdescriptor_TypeDef HIDdescriptor;//HID descriptor
  EndpointDescriptor_TypeDef EndpointDescriptor_1;//HID endpoint
} __attribute__(( packed )) GetConfigResponse_HIDonly_TypeDef;

typedef struct
{
  ConfigurationDescriptor_TypeDef ConfigurationDescriptor_1;
  InterfaceDescriptor_TypeDef InterfaceDescriptor_1;//MSD interface
  EndpointDescriptor_TypeDef EndpointDescriptor_2;//MSD endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_3;//MSD endpoint
} __attribute__(( packed )) GetConfigResponse_MSDonly_TypeDef;

//----------------------------------------------------------------------------------------------------------------------

//types and macros for control transfer state machine 
typedef enum //names for device states as in USB specification
  {
    DEFAULT,
    ADDRESS,
    CONFIGURED
  } DeviceState_TypeDef;

typedef enum //names for stages of control transfer
  {
    IDLE,//idle stage, awaiting for SETUP packet
    DATA_IN,//data stage, transmitting data to host
    DATA_OUT,//data stage, receiving data from host
    STATUS_IN,//status stage, transmitting ZLP to host
    STATUS_OUT//status stage, receiving ZLP from host
  } TransferStage_TypeDef;

//names for values of bRequest field in a control request
#define GET_STATUS 0
#define CLEAR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADDRESS 5
#define GET_DESCRIPTOR 6
#define SET_DESCRIPTOR 7
#define GET_CONFIGURATION 8
#define SET_CONFIGURATION 9
#define GET_INTERFACE 10
#define SET_INTERFACE 11
#define SYNCH_FRAME 12

//control request structure
typedef struct
{
  volatile unsigned char bmRequestType;
  volatile unsigned char bRequest;
  volatile unsigned short wValue;
  volatile unsigned short wIndex;
  volatile unsigned short wLength;
} ControlRequest_TypeDef;

//this structure contains all necessary information for handling control transfers
typedef struct
{
  unsigned int DataPointer;//points to where in RAM to continue reading from (from this address to DataPointer+BytesLeft is not yet transmitted data for a given transfer)
  unsigned short BytesLeft;//how many bytes are yet to be transmitted in data stage
  unsigned char NewAddress;//address to assign to a device after STATUS_IN stage is completed
  unsigned char ConfigurationNumber;//current device configuration number
  unsigned char ZLPneeded;//0 means there is no need for last packet to be ZLP in DATA_IN TransferStage, 1 = ZLP is needed
  volatile unsigned char EnumerationMode;//0 means default HID+MSD enumeration, 1 means HID-only / MSD-only enumeration
  DeviceState_TypeDef DeviceState;//current device state
  TransferStage_TypeDef TransferStage;//current stage of control transfer
  ControlRequest_TypeDef ControlRequest;//structure to hold the control request currently being processed  
  unsigned int OSfingerprintData[10];//holds copies of bytes 0-3 from first 10 control requests after poweron
  volatile unsigned char OSfingerprintCounter;//keeps track of how many control requests have been received after poweron
} ControlInfo_TypeDef;

//----------------------------------------------------------------------------------------------------------------------

//types for MSD class transfer state machine
typedef struct
{
  unsigned int dCBWSignature;
  unsigned int dCBWTag;
  unsigned int dCBWDataTransferLength;
  unsigned char bmCBWFlags;
  unsigned char bCBWLUN;
  unsigned char bCBWCBLength;
  unsigned char CBWCB[16];
} CBW_TypeDef;

typedef struct
{
  unsigned int dCSWSignature;
  unsigned int dCSWTag;
  unsigned int dCSWDataResidue;
  unsigned char bCSWStatus;
} CSW_TypeDef;

typedef enum
  {
    READY,//waiting for host to send CBW
    MSD_OUT,//data is being sent from host to device
    MSD_IN,//data is being sent from device to host    
    STATUS,//CSW is being sent form device to host
  } MSDstage_TypeDef;

//this structure contains all necessary information for handling MSD transfers
typedef struct
{
  CBW_TypeDef CBW;//CBW currently being processed
  CSW_TypeDef CSW;//CSW corresponding to current CBW
  MSDstage_TypeDef MSDstage;//stage of MSD transfer
  unsigned int DataPointer;//byte address in RAM where to continue reading/writing at next MSD transaction
  unsigned int BytesLeft;//number of bytes yet to be transmitted in a given MSD transfer
  unsigned int WriteStart;//byte address of where to store new data on the medium with write_pages()
  unsigned int EraseStart;//byte address of where to start erasing blocks on medium again (region between WriteStart and EraseStart is pre-erased already)
} MSDinfo_TypeDef;

//----------------------------------------------------------------------------------------------------------------------

//types for payload interpreter state machine
typedef struct
{
  unsigned int DefaultDelay;//delay in milliseconds automatically inserted between commands
  char* PayloadPointer;//points to the next ducky command to execute
  unsigned short BytesLeft;//how many bytes of ducky script are not yet executed
  unsigned char ActiveBuffer;//0 means ducky commands are being read from bytes 0 to 511 in payloadBuffer. 1 means from 512 to 1023
  volatile unsigned char FirstRead;//intialized to 0, set to 1 when first read command is received by MSD interface
  unsigned int RepeatSize;//number of commands that should be re-run by REPEAT command
  unsigned char UseFingerprinter;//0 means run script from payload.txt, 1 means run script based on which OS is detected
} PayloadInfo_TypeDef;

#endif //USB_H
