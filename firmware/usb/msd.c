#include "usb.h"
#include "msd_rodata.h"
#include "../fatfs/diskio.h"

//external flash memory is mapped to address 0xA0000000 in MCU memory space. these macros are for
//first and last accessible byte addresses of the mapped flash memory area (not next available address)
#define MSD_FIRSTADDR 0xA0000000U
#define MSD_LASTADDR  0xA180FFFFU

extern unsigned short EBImap[512];
extern unsigned short PBOmap[128];
extern PayloadInfo_TypeDef PayloadInfo;
extern DiskInfo_TypeDef DiskInfo;

MSDinfo_TypeDef MSDinfo;
unsigned char MSDbuffer[1024];

static void processNewCBW();
static void processInquiryCommand_6();
static void processReadCapacityCommand_10();
static void processTestUnitReadyCommand_6();
static void processRequestSenseCommand_6();
static void processStartStopUnitCommand_6();
static void processPreventAllowMediumRemovalCommand_6();
static void processModeSenseCommand_6();
static void processReadCommand_10();
static void processWriteCommand_10();

static void sendResponse(void* responseAddress, unsigned int responseSize);
static void sendData();
static void getData();
static void sendCSW(unsigned char status);

//----------------------------------------------------------------------------------------------------------------------

void processMSDtransaction()
{
  //in case of OUT transaction
  if( USB->ISTR & (1<<4) )
    {
      if(MSDinfo.MSDstage == READY)
	{
	  processNewCBW();
	}
      
      else if(MSDinfo.MSDstage == MSD_OUT)
	{
	  getData();//keep saving data to flash memory

	  //if last data transaction was just completed
	  if(MSDinfo.BytesLeft == 0)
	    {
	      sendCSW(0);//return good status
	      
	      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
	    }
	  
	  //if there are still data transactions left in this transfer
	  else
	    {
	      USB->EP2R = (1<<12)|(1<<7)|(2<<0);//respond to OUT packets with ACK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(3<<0);//respond to IN packets with NAK, ingore OUT packets	      
	    }
	}
    }
  
  //in case of IN transaction
  else
    {      
      if(MSDinfo.MSDstage == MSD_IN)
	{
	  //if last data transaction was just completed
	  if(MSDinfo.BytesLeft == 0)
	    {
	      sendCSW(0);//return good status
	      
	      //if device returned less data than host expected
	      if( (MSDinfo.CSW).dCSWDataResidue != 0 )
		{
		  USB->EP2R = (1<<15)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets
		  USB->EP3R = (1<<15)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets, clear CTR_TX flag
		}
	      //if device returned as much data as host expected
	      else
		{
		  USB->EP2R = (1<<15)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets
		  USB->EP3R = (1<<15)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets, clear CTR_TX flag
		}
	    }
	  
	  //if there are still data transactions left in this transfer
	  else
	    {
	      //keep sending data to host
	      if( (MSDinfo.DataPointer >= MSD_FIRSTADDR) && ((MSDinfo.DataPointer - 1) <= MSD_LASTADDR) ) sendData();//if data should come from the medium
	      else sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//if data should come from RAM or ROM
	      
	      USB->EP2R = (1<<15)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets
	      USB->EP3R = (1<<15)|(1<<4)|(3<<0);//respond to IN packets with data, ignore OUT packets, clear CTR_TX flag
	    }
	}
      
      else if(MSDinfo.MSDstage == STATUS)
	{
	  MSDinfo.MSDstage = READY;//CSW was just acknowledged by the host, device expects new CBW now
	  
	  USB->EP2R = (1<<15)|(1<<12)|(1<<7)|(2<<0);//respond with ACK to OUT packets, ignore IN packets
	  USB->EP3R = (1<<15)|(3<<0);//respond to IN packets with NAK, ignore OUT packets, clear CTR_TX flag
	}
    }
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

static void processNewCBW()
{
  unsigned char error = 0;//this variable will be set to 1 in case of any errors found in CBW
  
  //copy CBW from PMA to RAM, to save state and to allow word access to be used (4 byte access)
  bufferCopy( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR2_RX), (unsigned short*) &(MSDinfo.CBW), 31 );
  MSDinfo.BytesLeft = (MSDinfo.CBW).dCBWDataTransferLength;//try to send as much data as host expects
  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CBW).dCBWDataTransferLength;//no data was received / sent yet
  MSDinfo.ActiveBuffer = 0;//start reading/writing from the first 512 bytes of MSDbuffer[]
  
  //check if CBW is 31 bytes long, check if CBW signature is valid, check if target LUN is 0
  if( (BTABLE->COUNT2_RX & 0x03FF) != 31 )        error = 1;
  if( (MSDinfo.CBW).dCBWSignature != 0x43425355 ) error = 1;
  if( (MSDinfo.CBW).bCBWLUN != 0x00 )             error = 1;
  if(error)
    {
      sendCSW(2);//return error status, request reset recovery
      
      USB->EP2R = (1<<13)|(1<<12)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
      return;
    }
  
  //check if specified command is supported
  switch( (MSDinfo.CBW).CBWCB[0] )
    {
    case 0x00://TEST UNIT READY (6) command
      processTestUnitReadyCommand_6();
      break;
      
    case 0x03://REQUEST SENSE (6) command
      processRequestSenseCommand_6();
      break;
      
    case 0x12://INQUIRY (6) command
      processInquiryCommand_6();
      break;
      
    case 0x1A://MODE SENSE (6) command
      processModeSenseCommand_6();
      break;      
      
    case 0x1B://START STOP UNIT (6) command
      processStartStopUnitCommand_6();
      break;
      
    case 0x1E://PREVENT ALLOW MEDIUM REMOVAL (6) command
      processPreventAllowMediumRemovalCommand_6();
      break;
      
    case 0x25://READ CAPACITY (10) command
      processReadCapacityCommand_10();
      break;
      
    case 0x28://READ (10) command
      processReadCommand_10();
      break;
      
    case 0x2A://WRITE (10) command
      processWriteCommand_10();
      break;
      
    default://command is not recognized
      sendCSW(1);//return error status
      
      if( (MSDinfo.CBW).dCBWDataTransferLength )//if host wants to send or receive any data
	{      
	  if( (MSDinfo.CBW).bmCBWFlags & (1<<7) )//if host wants to receive data
	    {
	      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
	    }
	  
	  else//if host wants to send data
	    {
	      USB->EP2R = (1<<13)|(1<<12)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
	    }
	}
      
      else//if host does not want to send or receive any data
	{
	  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
	}
      break;
    }

  return;
}

static void processInquiryCommand_6()
{
  if( (MSDinfo.CBW).CBWCB[1] & 0x01 )//if EVPD is set
    {
      switch( (MSDinfo.CBW).CBWCB[2] )//check which VPD page is requested
	{
	case 0x00://VPD page code = supported VPD pages
	  sendResponse( &InquiryData_VPDpagelist, sizeof(InquiryData_VPDpagelist) );

	  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
	  break;

	default://specified VPD page code is not recognized
	  sendCSW(1);//return error status
	  
	  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
	  break;
	}
    }
  else//if EVPD is 0
    {
      sendResponse( &InquiryData_Standard, sizeof(InquiryData_Standard) );
      
      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
    }

  return;
}

static void processReadCapacityCommand_10()
{
  //copy READ CAPACITY response from ROM to PMA
  sendResponse( &ReadCapacity_Data, sizeof(ReadCapacity_Data) );

  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
  
  return;
}

static void processTestUnitReadyCommand_6()
{
  //do nothing. unit is always ready

  sendCSW(0);//return good status

  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
  
  return;
}

static void processRequestSenseCommand_6()
{
  sendResponse( &SenseData_Fixed, sizeof(SenseData_Fixed) );
  
  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets  
  
  return;
}

static void processStartStopUnitCommand_6()
{
  //do nothing, since no special load/eject or low power modes are necessary
  
  sendCSW(0);//return good status
  
  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
  
  return;
}

static void processPreventAllowMediumRemovalCommand_6()
{
  //do some medium access control here

  sendCSW(0);//return good status
  
  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets

  return;
}

static void processModeSenseCommand_6()
{
  unsigned char error = 0;
  
  if( ((MSDinfo.CBW).CBWCB[2] & (3<<6)) == (1<<6) ) error = 1;//if PC = 0b01 (changeable values requested)
  if( (MSDinfo.CBW).CBWCB[3] ) error = 1;//if subpage field is nonzero
  if(error)    
    {
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
      return;
    }
  
  //if default, current or saved values are requested
  switch( (MSDinfo.CBW).CBWCB[2] & 0x3F )
    {
    case 0x05://requested mode page = Flexible Disk
      sendResponse( &ModeSenseData_pagelist, sizeof(ModeSenseData_pagelist) );

      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
      break;
      
    case 0x3F://request for all available mode pages
      sendResponse( &ModeSenseData_pagelist, sizeof(ModeSenseData_pagelist) );
      
      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets  
      break;
     
    default://requested page number not recognized
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
      break;
    }
  
  return;
}

static void processReadCommand_10()
{
  //convert logical block address from big endian to little endian, map LBA address to byte address in external device memory
  MSDinfo.DataPointer = ( ((MSDinfo.CBW).CBWCB[2] << 24) | ((MSDinfo.CBW).CBWCB[3] << 16) | ((MSDinfo.CBW).CBWCB[4] << 8) | ((MSDinfo.CBW).CBWCB[5] << 0) ) * 512 + MSD_FIRSTADDR;

  //if specified address range is outside the area from MSD_FIRSTADDR to MSD_LASTADDR
  if( (MSDinfo.DataPointer + MSDinfo.BytesLeft - 1) > MSD_LASTADDR )
    {
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets            
    }
  //if specified address range is accessible
  else
    {
      if(MSDinfo.BytesLeft == 0)//if host requested 0 blocks to be read
	{
	  sendCSW(0);//return good status
	  
	  USB->EP2R = (1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
	}
      else//if host wants to read one or more blocks
	{ 
	  //preload the MSDbuffer[] with requested data	  
	  while(DiskInfo.TransferStatus != DISK_IDLE);//make sure disk_dmaread() request can be accepted
	  disk_dmaread(0, (unsigned char*) &MSDbuffer[0], (MSDinfo.DataPointer - MSD_FIRSTADDR) / 512, 1);
	  while(DiskInfo.TransferStatus != DISK_IDLE);//wait until one full block was read from the medium into MSDbuffer[]
	  
	  //if host requested 2 or more blocks to be read, preload the next block as well
	  if(MSDinfo.BytesLeft >= 1024) disk_dmaread(0, (unsigned char*) &MSDbuffer[512], (MSDinfo.DataPointer + 512 - MSD_FIRSTADDR) / 512, 1);
	  
	  sendData();//start sending data to USB host
	  
	  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
	}
    }
  
  PayloadInfo.FirstRead = 1;//indicate that a read command was received at least one time since poweron; used to implement DELAY functionality in ducky interpreter from main.c
  return;
}

static void processWriteCommand_10()
{
  //convert logical block address from big endian to little endian, map LBA address to byte address in external device memory
  MSDinfo.DataPointer = ( ((MSDinfo.CBW).CBWCB[2] << 24) | ((MSDinfo.CBW).CBWCB[3] << 16) | ((MSDinfo.CBW).CBWCB[4] << 8) | ((MSDinfo.CBW).CBWCB[5] << 0) ) * 512 + MSD_FIRSTADDR;
  
  //if specified address range is outside the area from MSD_FIRSTADDR to MSD_LASTADDR
  if( (MSDinfo.DataPointer + MSDinfo.BytesLeft - 1) > MSD_LASTADDR )
    {
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<13)|(1<<12)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
    }
  //if specified address range is accessible
  else
    {  
      if(MSDinfo.BytesLeft == 0)//if host requested 0 blocks to be written
	{
	  sendCSW(0);//return good status
	  
	  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
	}
      else//if host wants to write one or more blocks
	{
	  //make sure that there is pre-erased space to write all specified logical blocks
	  while(DiskInfo.TransferStatus != DISK_IDLE);//wait until prepare_LB() request can be sent
	  prepare_LB((MSDinfo.DataPointer - MSD_FIRSTADDR) / 512, (MSDinfo.CBW).dCBWDataTransferLength / 512 );
	  MSDinfo.MSDstage = MSD_OUT;
	  
	  USB->EP2R = (1<<12)|(1<<7)|(2<<0);//respond to OUT packets with ACK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<7)|(3<<0);//respond to IN packets with NAK, ingore OUT packets
	}
    }
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//send host a data structure located at responseAddress, of size responseSize. only up to MAXPACKET_MSD bytes can be sent with one function call
//only works for ROM and RAM buffers, not used for reading data from the main data medium (W25Q256FVFG flash memory chip)
static void sendResponse(void* responseAddress, unsigned int responseSize)
{
  //try to return all data requested, but not more than is available
  if(responseSize < MSDinfo.BytesLeft) MSDinfo.BytesLeft = responseSize;
  
  //if one transaction is enough to transfer all remaining data
  if(MSDinfo.BytesLeft <= MAXPACKET_MSD)
    {
      bufferCopy( (unsigned short*) responseAddress, (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_TX), MSDinfo.BytesLeft );
      BTABLE->COUNT3_TX = MSDinfo.BytesLeft;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MSDinfo.BytesLeft;
      MSDinfo.BytesLeft = 0;
      MSDinfo.DataPointer = (unsigned int) responseAddress + MSDinfo.BytesLeft;
      MSDinfo.MSDstage = MSD_IN;
    }

  //if several transactions are needed to transfer all remaining data
  else
    {
      bufferCopy( (unsigned short*) responseAddress, (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_TX), MAXPACKET_MSD );
      BTABLE->COUNT3_TX = MAXPACKET_MSD;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MAXPACKET_MSD;      
      MSDinfo.BytesLeft = MSDinfo.BytesLeft - MAXPACKET_MSD;
      MSDinfo.DataPointer = (unsigned int) responseAddress + MAXPACKET_MSD;
      MSDinfo.MSDstage = MSD_IN;
    }
  
  return;
}

//send host a continuous chunk of data located at dataAddress, of size dataSize. only up to MAXPACKET_MSD bytes can be sent with one function call
//only works for reading data from the main data medium (W25Q256FVFG flash memory chip)
static void sendData()
{  
  bufferCopy( (unsigned short*) &MSDbuffer[MSDinfo.ActiveBuffer * 512 + MSDinfo.DataPointer % 512], (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_TX), MAXPACKET_MSD );
  BTABLE->COUNT3_TX = MAXPACKET_MSD; 
  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MAXPACKET_MSD;
  MSDinfo.BytesLeft = MSDinfo.BytesLeft - MAXPACKET_MSD;
  MSDinfo.DataPointer = (unsigned int) MSDinfo.DataPointer + MAXPACKET_MSD;
  MSDinfo.MSDstage = MSD_IN;

  if((MSDinfo.DataPointer % 512) == 0)//if the block boundary is encountered
    {
      if(MSDinfo.BytesLeft > 512)//if there is a need to preload yet another block
	{
	  //preload a new block from the medium
	  while(DiskInfo.TransferStatus != DISK_IDLE);//wait until one full block was read from the medium into MSDbuffer[]
	  disk_dmaread(0, (unsigned char*) &MSDbuffer[MSDinfo.ActiveBuffer * 512], (MSDinfo.DataPointer + 512 - MSD_FIRSTADDR) / 512, 1);//replace data in a previously active buffer
	}
      
      MSDinfo.ActiveBuffer = (MSDinfo.ActiveBuffer + 1) % 2;//use the other half of MSDbuffer[] for the next USB transfers
    }
  
  return;
}

static void getData()
{
  //copy all bytes the host sent to receive buffer
  bufferCopy( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR2_RX), (unsigned short*) &MSDbuffer[MSDinfo.ActiveBuffer * 512 + MSDinfo.DataPointer % 512], BTABLE->COUNT2_RX & 0x03FF );
  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - (BTABLE->COUNT2_RX & 0x03FF);
  MSDinfo.BytesLeft = MSDinfo.BytesLeft - (BTABLE->COUNT2_RX & 0x03FF);
  MSDinfo.DataPointer = MSDinfo.DataPointer + (BTABLE->COUNT2_RX & 0x03FF);
  MSDinfo.MSDstage = MSD_OUT;
    
  if((MSDinfo.DataPointer % 512) == 0)//if the block boundary is encountered
    {
      //write new received block to the medium
      while(DiskInfo.TransferStatus != DISK_IDLE);//wait until previous block is written completely
      disk_dmawrite(0, (unsigned char*) &MSDbuffer[MSDinfo.ActiveBuffer * 512], (MSDinfo.DataPointer - 512 - MSD_FIRSTADDR) / 512, 1);//save new block in flash
      
      //if transfer is over or LS boundary is encountered
      if( ( ((MSDinfo.DataPointer - MSD_FIRSTADDR) % (512 * 110)) == 0 ) || (MSDinfo.BytesLeft == 0) )
	{
	  while(DiskInfo.TransferStatus != DISK_IDLE);//make sure writemap_PBO() request can be accepted
	  writemap_PBO();//write new flash metadata based on PBOmap[]
	}
      
      MSDinfo.ActiveBuffer = (MSDinfo.ActiveBuffer + 1) % 2;//use the other half of MSDbuffer[] for the next USB transfers
    }
  
  return;
}

//send host the CSW for current transfer. dCSWDataResidue and must already be set correctly
static void sendCSW(unsigned char status)
{
  (MSDinfo.CSW).dCSWTag = (MSDinfo.CBW).dCBWTag;//set CSW tag according to current CBW
  (MSDinfo.CSW).bCSWStatus = status;//return status as specified in argument
  bufferCopy( (unsigned short*) &(MSDinfo.CSW), (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_TX), 13 );//copy CSW from RAM to PMA
  BTABLE->COUNT3_TX = 13;
  MSDinfo.MSDstage = STATUS;//CSW is being sent, waiting for host to acknowledge it

  return;
}
