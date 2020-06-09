#ifndef MSD_RODATA_H
#define MSD_RODATA_H

//command response structures
static unsigned char InquiryData_Standard[36] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  0x00,//device is accessible at specified LUN, type = direct access device
  0x80,//device is removable
  0x02,//standard = ANSI X3.131.1994
  0x02,//response data format = SPC-2
  0x1F,//additional length = 31 bytes
  0x00,//no special features supported
  0x00,//no special features supported
  0x00,//no special features supported
  'O', 'r', 'd', 'i', 'n', 'a', 'r', 'y',//Vendor Identification
  ' ', 'F', 'l', 'a', 's', 'h', ' ', 'D',//Product Identification
  'r', 'i', 'v', 'e', ' ', ' ', ' ', ' ',//Product Identification
  ' ', ' ', ' ', ' ' //Product Revision Level
};

static unsigned char InquiryData_VPDpagelist[] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
  //supported VPD pages list contains only VPDpage 0
  0x00,//device is accessible at specified LUN, type = direct access device
  0x00,//VPD page code = 0x00
  0x00,//reserved
  0x01,//page length = 1
  0x00 //VPD page 0x00 supported
};

static unsigned char ModeSenseData_pagelist[36] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{  
  0x23,//mode data length = 35 bytes  
  0x00,//medium type = 0  
  0x00,//medium is not write protected; no support for DPO, FUA bits
  0x00,//total length of block descriptors = 0  
  0x05,//mode page = Flexible Disk, page is not saveable, not a subpage format
  0x1E,//page length = 30 bytes
  0x13,//transfer rate = 610KiB per second
  0x88,//transfer rate = 610KiB per second
  0x08,//number of heads = 8
  0x80,//sectors per track = 128
  0x02,//data bytes per sector = 512
  0x00,//data bytes per sector = 512
  0x00,//number of cylinders = 192
  0xC0 //number of cylinders = 192
};

static unsigned char SenseData_Fixed[18] __attribute__(( aligned(2), section(".rodata,\"a\",%progbits@") )) =
{
 0x70,//current error sense data
 0x00,//reserved
 0x05 //ILLEGAL REQUEST
};

static unsigned char ReadCapacity_Data[8] __attribute__(( aligned(2) )) =
{
  //last accessible LBA = 197119
  0x00,
  0x03,
  0x01,
  0xFF,
  //block size = 512 bytes
  0x00,
  0x00,
  0x02,
  0x00
};

#endif //MSD_RODATA_H
