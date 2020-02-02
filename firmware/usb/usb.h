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
  EndpointDescriptor_TypeDef EndpointDescriptor_1_IN;//HID_IN  endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_1_OUT;//HID_OUT endpoint
  InterfaceDescriptor_TypeDef InterfaceDescriptor_1;//MSD interface
  EndpointDescriptor_TypeDef EndpointDescriptor_2_OUT;//MSD_OUT endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_3_IN;//MSD_IN  endpoint
} __attribute__(( packed )) GetConfigResponse_default_TypeDef;

typedef struct
{
  ConfigurationDescriptor_TypeDef ConfigurationDescriptor_1;
  InterfaceDescriptor_TypeDef InterfaceDescriptor_0;//HID interface
  HIDdescriptor_TypeDef HIDdescriptor;//HID descriptor
  EndpointDescriptor_TypeDef EndpointDescriptor_1_IN;//HID_IN  endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_1_OUT;//HID_OUT endpoint
} __attribute__(( packed )) GetConfigResponse_HIDonly_TypeDef;

typedef struct
{
  ConfigurationDescriptor_TypeDef ConfigurationDescriptor_1;
  InterfaceDescriptor_TypeDef InterfaceDescriptor_0;//MSD interface
  EndpointDescriptor_TypeDef EndpointDescriptor_2_OUT;//MSD OUT endpoint
  EndpointDescriptor_TypeDef EndpointDescriptor_3_IN;//MSD IN endpoint
} __attribute__(( packed )) GetConfigResponse_MSDonly_TypeDef;

//----------------------------------------------------------------------------------------------------------------------

//types and macros for control transfer state machine 
typedef volatile enum //names for device states as in USB specification
  {
    DEFAULT,
    ADDRESS,
    CONFIGURED
  } DeviceState_TypeDef;

typedef volatile enum //names for stages of control transfer
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
  unsigned int DataPointer;//points to where in RAM to continue reading/writing from (from this address to DataPointer+BytesLeft is not yet transmitted data for a given transfer)
  unsigned short BytesLeft;//how many bytes are yet to be transmitted in data stage
  unsigned char NewAddress;//address to assign to a device after STATUS_IN stage is completed
  unsigned char ConfigurationNumber;//current device configuration number
  unsigned char ZLPneeded;//0 means there is no need for last packet to be ZLP in DATA_IN TransferStage, 1 = ZLP is needed  
  DeviceState_TypeDef DeviceState;//current device state
  TransferStage_TypeDef TransferStage;//current stage of control transfer
  ControlRequest_TypeDef ControlRequest;//structure to hold the control request currently being processed  
  unsigned int OSfingerprintData[10];//holds copies of bytes 0-3 from first 10 control requests after poweron
  volatile unsigned char OSfingerprintCounter;//keeps track of how many control requests have been received after poweron
  volatile unsigned char EnumerationMode;//0 means default HID+MSD enumeration, 1 means HID-only enumeration
  volatile unsigned char HIDprotocol;//0 means boot protocol is currently being used by HID interface, 1 means report protocol
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
  unsigned char ActiveBuffer;//0 = first 512 bytes of MSDbuffer[] are currently used by USB, 1 = last 512 bytes
  unsigned char TargetFlag;//0 = DataPointer points to MCU internal memory address, 1 = points to external flash memory
  MSDstage_TypeDef MSDstage;//stage of MSD transfer
  unsigned int DataPointer;//byte address in RAM where to continue reading/writing at next MSD transaction
  unsigned int BytesLeft;//number of bytes yet to be transmitted in a given MSD transfer
} MSDinfo_TypeDef;

//----------------------------------------------------------------------------------------------------------------------

//types for payload interpreter state machine
typedef struct
{
  unsigned int DefaultDelay;//delay in milliseconds automatically inserted between commands
  unsigned int StringDelay;//holds for how long STRING command should keep a key pressed and then released
  char* PayloadPointer;//points to the next ducky command to execute
  unsigned int BytesLeft;//how many bytes of ducky script are not yet executed
  unsigned int RepeatStart;//holds read/write pointer into a payload file (not PayloadBuffer), pointing to the beginning of a repeat block
  unsigned int RepeatCount;//number of times the repeat block sholud be repeated; 0xFFFFFFFF means no valid value was assigned yet
  unsigned int   HoldMousedata;//contains mouse keys and movement to be sent in a default report
  unsigned int   HoldKeycodes;//contains keycodes to be sent in a default report
  unsigned short HoldModifiers;//contains modifier keys to be sent in a default report
  unsigned short LBAoffset;//contains lowest LBA which is available to MSD interface (lower LBA's are hidden)
  unsigned short FakeCapacity;//contains a fake capacity value in MiB; use real capacity if FakeCapacity == 0
  char Filename[13];//holds the name of the file on which some particular action should be performed
  
  unsigned char PayloadFlags;//holds status flag bitmask with meanings of each bit specified below:
  // (1<<0) ActiveBufferFlag; 1 means last 1024 bytes of PayloadBuffer are being executed; 0 means first 1024 bytes
  // (1<<1) RepeatFlag; 1 means preserve the value of RepeatStart when runDuckyCommand() is called; 0 means let RepeatStart value to change
  // (1<<2) HoldFlag; 1 means Hold Modifiers, Keycodes and Mousedata should be set to a new value on command exit; 0 means no need to change
  // (1<<3) MouseFlag; 1 means mouse report should also be sent by current command; 0 means only send keyboard report
  // (1<<4) ActionFlag; 1 means ONACTION_DEFAULT_DELAY should be inserted after current command; 0 means ONACTION delay will not be inserted
  // (1<<5) Reserved
  // (1<<6) Reserved
  // (1<<7) Reserved
  
  volatile unsigned char DeviceFlags;//holds status flag bitmask with meanings of each bit specified below:
  // (1<<0) NoInsertFlag; 1 means do not run on-insertion payload; 0 means run on-insertion payload
  // (1<<1) FirstReadFlag; 1 means there was at least 1 read command sent to MSD interface; 0 means no MSD read command was received yet
  // (1<<2) FingerprinterFlag; 1 means run script based on which OS is detected; 0 means run script from payload.txt
  // (1<<3) DFUmodeFlag; 1 means request for on-demand payload number 20 or above results in reboot to DFU mode; 0 means reboot to MSD mode
  // (1<<4) OnactionDelayFlag; 1 means default delay is only placed after commands which press keys; 0 means add default delay after every single line
  // (1<<5) Reserved
  // (1<<6) Reserved
  // (1<<7) Reserved
} PayloadInfo_TypeDef;

#endif //USB_H
