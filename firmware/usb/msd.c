#include "usb.h"
#include "msd_rodata.h"
#include "../fatfs/diskio.h"

extern DiskInfo_TypeDef DiskInfo;
extern PayloadInfo_TypeDef PayloadInfo;

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
	  //if last data transaction was just completed
	  if(MSDinfo.BytesLeft == MAXPACKET_MSD)
	    {
	      getData();//save last data packet to flash memory
	      sendCSW(0);//return good status	      
	      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
	    }
	  
	  //if there are still data transactions left in this transfer
	  else
	    {
	      USB->EP2R = (1<<8)|(1<<7)|(1<<6)|(2<<0);//respond to OUT packets with ACK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<8)|(1<<7)|(3<<0);//respond to IN packets with NAK, ingore OUT packets
	      getData();//save data packet to flash memory
	    }
	}
    }
  
  //in case of IN transaction
  else
    {      
      if(MSDinfo.MSDstage == MSD_IN)
	{
	  //if last data transaction was just completed
	  if( (BTABLE->COUNT3_TX == 0) || (BTABLE->COUNT3_RX == 0) )
	    {
	      sendCSW(0);//return good status
	      
	      //if device returned less data than host expected
	      if( (MSDinfo.CSW).dCSWDataResidue != 0 )
		{
		  USB->EP2R = (1<<15)|(1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets
		  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<5)|(3<<0);//respond to IN packets with STALL, ignore OUT packets, clear CTR_TX flag
		}
	      //if device returned as much data as host expected
	      else
		{
		  USB->EP2R = (1<<15)|(1<<8)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets
		  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(3<<0);//respond to IN packets with CSW, ignore OUT packets, clear CTR_TX flag
		}
	    }
	  
	  //if there are still data transactions left in this transfer
	  else
	    {
	      USB->EP2R = (1<<15)|(1<<8)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets
	      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(3<<0);//respond to IN packets with data, ignore OUT packets, clear CTR_TX flag
	      
	      //pre-fill the next buffer with data
	      if( MSDinfo.TargetFlag ) sendData();//if data should come from the medium
	      else sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//if data should come from RAM or ROM	      		  
	    }
	}
      
      else if(MSDinfo.MSDstage == STATUS)
	{
	  MSDinfo.MSDstage = READY;//CSW was just acknowledged by the host, device expects new CBW now
	  
	  USB->EP2R = (1<<15)|(1<<8)|(1<<7)|(1<<6)|(2<<0);//respond with ACK to OUT packets, ignore IN packets
	  USB->EP3R = (1<<15)|(1<<8)|(1<<4)|(3<<0);//respond to IN packets with NAK, ignore OUT packets, clear CTR_TX flag
	}
    }
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

static void processNewCBW()
{
  unsigned short bufferOffset;//holds packet memory offset to the buffer which should be used by software (other buffer is used by USB peripheral)
  unsigned short bytesReceived;//holds how many bytes a particular packet memory buffer has received
  unsigned char  error = 0;//this variable will be set to 1 in case of any errors found in CBW
  
  //set bufferOffset and bytesReceived to correct values
  if(USB->EP2R & (1<<14)) {bufferOffset = BTABLE->ADDR2_TX; bytesReceived = BTABLE->COUNT2_TX & 0x03FF;}
  else                    {bufferOffset = BTABLE->ADDR2_RX; bytesReceived = BTABLE->COUNT2_RX & 0x03FF;}
  
  //copy CBW from PMA to RAM, to save state and to allow word access to be used (4 byte access)
  bufferCopy( (unsigned short*) (BTABLE_BaseAddr + bufferOffset), (unsigned short*) &(MSDinfo.CBW), 31 );
  MSDinfo.BytesLeft = (MSDinfo.CBW).dCBWDataTransferLength;//try to send as much data as host expects
  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CBW).dCBWDataTransferLength;//no data was received / sent yet
  MSDinfo.ActiveBuffer = 0;//start reading/writing from the first 512 bytes of MSDbuffer[]
  MSDinfo.TargetFlag = 0;//read data from MCU memory space, unless Read or Write commands are being processed
  
  //check if CBW is 31 bytes long, check if CBW signature is valid, check if target LUN is 0
  if(  bytesReceived != 31 )                      error = 1;
  if( (MSDinfo.CBW).dCBWSignature != 0x43425355 ) error = 1;
  if( (MSDinfo.CBW).bCBWLUN != 0x00 )             error = 1;
  if(error)
    {
      sendCSW(2);//return error status, request reset recovery
      
      USB->EP2R = (1<<13)|(1<<8)|(1<<7)|(1<<6)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
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
	      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
	    }
	  
	  else//if host wants to send data
	    {
	      USB->EP2R = (1<<13)|(1<<8)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
	    }
	}
      
      else//if host does not want to send or receive any data
	{
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
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
	  sendResponse( &InquiryData_VPDpagelist, sizeof(InquiryData_VPDpagelist) );//pre-fill the first packet buffer	  
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
	  sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
	  break;
	  
	default://specified VPD page code is not recognized
	  sendCSW(1);//return error status
	  
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
	  break;
	}
    }
  else//if EVPD is 0
    {
      sendResponse( &InquiryData_Standard, sizeof(InquiryData_Standard) );//pre-fill the first packet buffer      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets      
      sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
    }
  
  return;
}

static void processReadCapacityCommand_10()
{
  unsigned int sendLastLBA = 0x000301FF - PayloadInfo.LBAoffset;//value that will be sent to host as a last accessible LBA
  if(PayloadInfo.FakeCapacity) sendLastLBA = PayloadInfo.FakeCapacity * 2048 - 1;//if necessary, use fake capacity
  
  //write necessary value into ReadCapacity command response
  ReadCapacity_Data[0] = sendLastLBA >> 24;
  ReadCapacity_Data[1] = sendLastLBA >> 16;
  ReadCapacity_Data[2] = sendLastLBA >>  8;
  ReadCapacity_Data[3] = sendLastLBA >>  0;
  
  //copy READ CAPACITY response from RAM to PMA
  sendResponse( &ReadCapacity_Data, sizeof(ReadCapacity_Data) );//pre-fill the first packet buffer
  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
  sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
  return;
}

static void processTestUnitReadyCommand_6()
{
  //do nothing. unit is always ready
  
  sendCSW(0);//return good status
  
  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
  
  return;
}

static void processRequestSenseCommand_6()
{
  sendResponse( &SenseData_Fixed, sizeof(SenseData_Fixed) );//pre-fill the first packet buffer  
  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
  sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
  
  return;
}

static void processStartStopUnitCommand_6()
{
  //do nothing, since no special load/eject or low power modes are necessary
  
  sendCSW(0);//return good status
  
  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
  
  return;
}

static void processPreventAllowMediumRemovalCommand_6()
{
  //do some medium access control here

  sendCSW(0);//return good status
  
  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets

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
      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
      return;
    }
  
  //if default, current or saved values are requested
  switch( (MSDinfo.CBW).CBWCB[2] & 0x3F )
    {
    case 0x05://requested mode page = Flexible Disk
      sendResponse( &ModeSenseData_pagelist, sizeof(ModeSenseData_pagelist) );//pre-fill the first packet buffer      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets      
      sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
      break;
      
    case 0x3F://request for all available mode pages
      sendResponse( &ModeSenseData_pagelist, sizeof(ModeSenseData_pagelist) );//pre-fill the first packet buffer      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
      sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//start to pre-fill the next packet buffer
      break;
     
    default://requested page number not recognized
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
      break;
    }
  
  return;
}

static void processReadCommand_10()
{
  //convert logical block address from big endian to little endian, map LBA address to byte address in external device memory
  MSDinfo.DataPointer = ( ((MSDinfo.CBW).CBWCB[2] << 24) | ((MSDinfo.CBW).CBWCB[3] << 16) | ((MSDinfo.CBW).CBWCB[4] << 8) | ((MSDinfo.CBW).CBWCB[5] << 0) ) * 512;
  MSDinfo.DataPointer = MSDinfo.DataPointer + PayloadInfo.LBAoffset * 512;//if some blocks should be hidden, add necessary LBA offset to all read operations
  MSDinfo.TargetFlag = 1;//MSDinfo.DataPointer points into external flash memory now
  
  //if anything in specified address range is not accessible and fake capacity is not used (last real LBA is 197119)
  if( ((MSDinfo.DataPointer + MSDinfo.BytesLeft) > (197120 * 512)) && (PayloadInfo.FakeCapacity == 0) )
    {
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
    }
  //if specified address range is accessible
  else
    {
      if(MSDinfo.BytesLeft == 0)//if host requested 0 blocks to be read
	{
	  sendCSW(0);//return good status
	  
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
	}
      else//if host wants to read one or more blocks
	{	  
	  //preload the MSDbuffer[] with requested data
	  while(DiskInfo.BusyFlag);//make sure dmaread_LB() request can be accepted	  
	  dmaread_LB((unsigned char*) &MSDbuffer[0], MSDinfo.DataPointer / 512);	  
	  while(DiskInfo.BusyFlag);//wait until one full block was read from the medium into MSDbuffer[]
	  
	  //if host requested 2 or more blocks to be read, preload the next block as well
	  if(MSDinfo.BytesLeft >= 1024) dmaread_LB((unsigned char*) &MSDbuffer[512], (MSDinfo.DataPointer + 512) / 512);
	  
	  sendData();//start sending data to USB host	  
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
	  sendData();//start sending data to USB host
	}
    }

  PayloadInfo.DeviceFlags |= (1<<1);//indicate that a read command was received at least one time since poweron; used to implement DELAY functionality in ducky interpreter from main.c
  return;
}

static void processWriteCommand_10()
{
  //convert logical block address from big endian to little endian, map LBA address to byte address in external device memory
  MSDinfo.DataPointer = ( ((MSDinfo.CBW).CBWCB[2] << 24) | ((MSDinfo.CBW).CBWCB[3] << 16) | ((MSDinfo.CBW).CBWCB[4] << 8) | ((MSDinfo.CBW).CBWCB[5] << 0) ) * 512;
  MSDinfo.DataPointer = MSDinfo.DataPointer + PayloadInfo.LBAoffset * 512;//if some blocks should be hidden, add necessary LBA offset to all write operations
  MSDinfo.TargetFlag = 1;//MSDinfo.DataPointer points into external flash memory now
  
  //if anything in specified address range is not accessible and fake capacity is not used (last real LBA is 197119)
  if( ((MSDinfo.DataPointer + MSDinfo.BytesLeft) > (197120 * 512)) && (PayloadInfo.FakeCapacity == 0) )
    {
      sendCSW(1);//return error status
      
      USB->EP2R = (1<<13)|(1<<8)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ignore OUT packets
    }
  //if specified address range is accessible
  else
    {
      if(MSDinfo.BytesLeft == 0)//if host requested 0 blocks to be written
	{
	  sendCSW(0);//return good status
	  
	  USB->EP2R = (1<<8)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<14)|(1<<8)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
	}
      else//if host wants to write one or more blocks
	{
	  //make sure that there is pre-erased space to write all specified logical blocks
	  while(DiskInfo.BusyFlag);//wait until prepare_LB() request can be sent
	  prepare_LB(MSDinfo.DataPointer / 512, (MSDinfo.CBW).dCBWDataTransferLength / 512 );
	  MSDinfo.MSDstage = MSD_OUT;
	  
	  USB->EP2R = (1<<8)|(1<<7)|(1<<6)|(2<<0);//respond to OUT packets with ACK, ignore IN packets, clear CTR_RX flag
	  USB->EP3R = (1<<15)|(1<<8)|(1<<7)|(3<<0);//respond to IN packets with NAK, ingore OUT packets
	}
    }
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//send host a data structure located at responseAddress, of size responseSize. only up to MAXPACKET_MSD bytes can be sent with one function call
//only works for ROM and RAM buffers, not used for reading data from the main data medium (W25N01GVZEIG flash memory chip)
static void sendResponse(void* responseAddress, unsigned int responseSize)
{
  unsigned short  bufferOffset;//holds packet memory offset to the buffer which should be used by software (other buffer is used by USB peripheral)
  unsigned short* countPointer;//holds where in packet memory should the number of bytes to transmit be stored
  
  //set bufferOffset and countPointer to correct values
  if(USB->EP3R & (1<<14)) {bufferOffset = BTABLE->ADDR3_RX; countPointer = (unsigned short*) (&BTABLE->COUNT3_RX);}
  else                    {bufferOffset = BTABLE->ADDR3_TX; countPointer = (unsigned short*) (&BTABLE->COUNT3_TX);}
  
  //try to return all data requested, but not more than is available
  if(responseSize < MSDinfo.BytesLeft) MSDinfo.BytesLeft = responseSize;

  //if one transaction is enough to transfer all remaining data
  if(MSDinfo.BytesLeft <= MAXPACKET_MSD)
    {
      bufferCopy( (unsigned short*) responseAddress, (unsigned short*) (BTABLE_BaseAddr + bufferOffset), MSDinfo.BytesLeft );
      *countPointer = MSDinfo.BytesLeft;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MSDinfo.BytesLeft;
      MSDinfo.BytesLeft = 0;
      MSDinfo.DataPointer = (unsigned int) responseAddress + MSDinfo.BytesLeft;
      MSDinfo.MSDstage = MSD_IN;
    }
  
  //if several transactions are needed to transfer all remaining data
  else
    {
      bufferCopy( (unsigned short*) responseAddress, (unsigned short*) (BTABLE_BaseAddr + bufferOffset), MAXPACKET_MSD );
      *countPointer = MAXPACKET_MSD;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MAXPACKET_MSD;      
      MSDinfo.BytesLeft = MSDinfo.BytesLeft - MAXPACKET_MSD;
      MSDinfo.DataPointer = (unsigned int) responseAddress + MAXPACKET_MSD;
      MSDinfo.MSDstage = MSD_IN;
    }
  
  return;
}

//send host a continuous chunk of data located at dataAddress, of size dataSize. only up to MAXPACKET_MSD bytes can be sent with one function call
//only works for reading data from the main data medium (W25N01GVZEIG flash memory chip)
static void sendData()
{
  unsigned short  bufferOffset;//holds packet memory offset to the buffer which should be used by software (other buffer is used by USB peripheral)
  unsigned short* countPointer;//holds where in packet memory should the number of bytes to transmit be stored
  
  //set bufferOffset and countPointer to correct values
  if(USB->EP3R & (1<<14)) {bufferOffset = BTABLE->ADDR3_RX; countPointer = (unsigned short*) (&BTABLE->COUNT3_RX);}
  else                    {bufferOffset = BTABLE->ADDR3_TX; countPointer = (unsigned short*) (&BTABLE->COUNT3_TX);}
  
  //if one transaction is enough to transfer all remaining data
  if(MSDinfo.BytesLeft <= MAXPACKET_MSD)
    {
      bufferCopy( (unsigned short*) &MSDbuffer[MSDinfo.ActiveBuffer * 512 + MSDinfo.DataPointer % 512], (unsigned short*) (BTABLE_BaseAddr + bufferOffset), MSDinfo.BytesLeft );
      *countPointer = MSDinfo.BytesLeft;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MSDinfo.BytesLeft;
      MSDinfo.BytesLeft = 0;
      MSDinfo.DataPointer = (unsigned int) MSDinfo.DataPointer + MSDinfo.BytesLeft;
      MSDinfo.MSDstage = MSD_IN;      
    }
  
  //if several transactions are needed to transfer all remaining data
  else
    {
      bufferCopy( (unsigned short*) &MSDbuffer[MSDinfo.ActiveBuffer * 512 + MSDinfo.DataPointer % 512], (unsigned short*) (BTABLE_BaseAddr + bufferOffset), MAXPACKET_MSD );
      *countPointer = MAXPACKET_MSD;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MAXPACKET_MSD;
      MSDinfo.BytesLeft = MSDinfo.BytesLeft - MAXPACKET_MSD;
      MSDinfo.DataPointer = (unsigned int) MSDinfo.DataPointer + MAXPACKET_MSD;
      MSDinfo.MSDstage = MSD_IN;
    }
  
  if((MSDinfo.DataPointer % 512) == 0)//if the block boundary is encountered
    {
      if(MSDinfo.BytesLeft > 512)//if there is a need to preload yet another block
	{
	  //preload a new block from the medium
	  while(DiskInfo.BusyFlag);//wait until one full block was read from the medium into MSDbuffer[]
	  dmaread_LB((unsigned char*) &MSDbuffer[MSDinfo.ActiveBuffer * 512], (MSDinfo.DataPointer + 512) / 512);//replace data in a previously active buffer
	}
      
      MSDinfo.ActiveBuffer = (MSDinfo.ActiveBuffer + 1) % 2;//use the other half of MSDbuffer[] for the next USB transfers
    }
  
  return;
}

static void getData()
{
  unsigned short bufferOffset;//holds packet memory offset to the buffer which should be used by software (other buffer is used by USB peripheral)
  unsigned short bytesReceived;//holds how many bytes a particular packet memory buffer has received
  
  //set bufferOffset and bytesReceived to correct values
  if(USB->EP2R & (1<<14)) {bufferOffset = BTABLE->ADDR2_TX; bytesReceived = BTABLE->COUNT2_TX & 0x03FF;}
  else                    {bufferOffset = BTABLE->ADDR2_RX; bytesReceived = BTABLE->COUNT2_RX & 0x03FF;}
  
  //copy all bytes the host sent to receive buffer
  bufferCopy( (unsigned short*) (BTABLE_BaseAddr + bufferOffset), (unsigned short*) &MSDbuffer[MSDinfo.ActiveBuffer * 512 + MSDinfo.DataPointer % 512], bytesReceived );
  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - bytesReceived;
  MSDinfo.BytesLeft = MSDinfo.BytesLeft - bytesReceived;
  MSDinfo.DataPointer = MSDinfo.DataPointer + bytesReceived;
  MSDinfo.MSDstage = MSD_OUT;
  
  if((MSDinfo.DataPointer % 512) == 0)//if the block boundary is encountered
    {
      while(DiskInfo.BusyFlag);//wait until previous block is written completely
      
      //send the new block to W25N01GVZEIG internal Data Buffer
      if( ( (MSDinfo.DataPointer % (512 * 4)) == 0 ) || (MSDinfo.BytesLeft == 0) ) 
	{
	  //if transfer is over or LP boundary is encountered, write data to internal Data Buffer and save the page to flash
	  dmawrite_LB((unsigned char*) &MSDbuffer[MSDinfo.ActiveBuffer * 512], (MSDinfo.DataPointer - 512) / 512, 1);
	}
      else
	{
	  //if there is more data to be stored in current LP, write data to internal Data Buffer only
	  dmawrite_LB((unsigned char*) &MSDbuffer[MSDinfo.ActiveBuffer * 512], (MSDinfo.DataPointer - 512) / 512, 0);
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
  bufferCopy( (unsigned short*) &(MSDinfo.CSW), (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_RX), 13 );//copy CSW from RAM to PMA
  BTABLE->COUNT3_TX = 13;
  BTABLE->COUNT3_RX = 13;
  MSDinfo.MSDstage = STATUS;//CSW is being sent, waiting for host to acknowledge it

  return;
}
