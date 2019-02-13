#include "usb.h"
#include "msd_rodata.h"
#include "../fatfs/diskio.h"

//macros for first and last accessible byte addresses on the device (not next available address)
#define MSD_FIRSTADDR 0xA0000000U
#define MSD_LASTADDR  0xA1FFFFFFU

extern unsigned char ReadBuffer[512] __attribute__(( aligned(2) ));
extern unsigned char WriteBuffer[4096] __attribute__(( aligned(2) ));
extern PayloadInfo_TypeDef PayloadInfo;

static void processNewCBW();
static void processInquiryCommand_6();
static void processReadCapacityCommand_10();
static void processTestUnitReadyCommand_6();
static void processPreventAllowMediumRemovalCommand_6();
static void processModeSenseCommand_6();
static void processReadCommand_10();
static void processWriteCommand_10();
static void sendResponse(void* responseAddress, unsigned int responseSize);
static void sendData();
static void sendCSW(unsigned char status);

//----------------------------------------------------------------------------------------------------------------------

//this data structure is needed for MSD transfers
MSDinfo_TypeDef MSDinfo;

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
	  //copy all bytes the host sent to destination address
	  bufferCopy( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR2_RX), (unsigned short*) ((unsigned int) &WriteBuffer + MSDinfo.DataPointer % 4096), BTABLE->COUNT2_RX & 0x03FF );	  
	  MSDinfo.DataPointer = MSDinfo.DataPointer + (BTABLE->COUNT2_RX & 0x03FF);
	  (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - (BTABLE->COUNT2_RX & 0x03FF);
	  MSDinfo.BytesLeft = MSDinfo.BytesLeft - (BTABLE->COUNT2_RX & 0x03FF);
	  
	  if( MSDinfo.BytesLeft )//if there is still data yet to be transmitted
	    {	      
	      //if WriteBuffer is full, write it to the medium
	      if( (MSDinfo.DataPointer % 4096) == 0 )
		{		  
		  //if blocks have to be erased before writing new data
		  if( (MSDinfo.DataPointer - MSD_FIRSTADDR) > MSDinfo.EraseStart )
		    {
		      //try to erase with biggest possible blocksize
		      if     ( (MSDinfo.BytesLeft >= (4096 * 15)) && !(MSDinfo.WriteStart % (4096 * 16)) ) {block_erase_64k(MSDinfo.WriteStart); MSDinfo.EraseStart = MSDinfo.EraseStart + 4096 * 16;}
		      else if( (MSDinfo.BytesLeft >= (4096 *  7)) && !(MSDinfo.WriteStart % (4096 *  8)) ) {block_erase_32k(MSDinfo.WriteStart); MSDinfo.EraseStart = MSDinfo.EraseStart + 4096 *  8;}
		      else                                                                                 {block_erase_4k(MSDinfo.WriteStart);  MSDinfo.EraseStart = MSDinfo.EraseStart + 4096 *  1;}
		    }
		  
		  write_pages(MSDinfo.WriteStart);
		  MSDinfo.WriteStart = MSDinfo.DataPointer - MSD_FIRSTADDR;//move WriteStart pointer to next sector that was not written yet
		}
	      
	      USB->EP2R = (1<<12)|(1<<7)|(2<<0);//respond to OUT packets with ACK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(3<<0);//respond to IN packets with NAK, ingore OUT packets
	    }
	  else//if last data transaction just got completed
	    {	      
	      //pre-fill WriteBuffer with existing data on the medium if necessary
	      if(MSDinfo.DataPointer % 4096) disk_read(0, (unsigned char*) &WriteBuffer + MSDinfo.DataPointer % 4096, (MSDinfo.DataPointer - MSD_FIRSTADDR) / 512, (4096 - MSDinfo.DataPointer % 4096) / 512 );

	      //erase last block if necessary
	      if( (MSDinfo.DataPointer - MSD_FIRSTADDR) > MSDinfo.EraseStart ) {block_erase_4k(MSDinfo.WriteStart);  MSDinfo.EraseStart = MSDinfo.EraseStart + 4096 *  1;}
	      
	      //save WriteBuffer to the medium
	      write_pages(MSDinfo.WriteStart);
	      MSDinfo.WriteStart = MSDinfo.DataPointer - MSD_FIRSTADDR;//move WriteStart pointer to next sector that was not written yet
	      
	      sendCSW(0);//return good status
	      
	      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
	      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with CSW, ingore OUT packets
	    }
	}
    }
  
  //in case of IN transaction
  else
    {
      
      if(MSDinfo.MSDstage == MSD_IN)
	{
	  //keep sending data to host, send CSW with good status at the end of transfer
	  if( (MSDinfo.DataPointer >= MSD_FIRSTADDR) && ((MSDinfo.DataPointer - 1) <= MSD_LASTADDR) ) sendData();//if data should come from the medium
	  else sendResponse( (void*) MSDinfo.DataPointer, MSDinfo.BytesLeft );//if data should come from RAM or ROM
	  
	  //if transfer is over but device actiually returned less data than host expected
	  if( ((MSDinfo.CSW).dCSWDataResidue != 0) && (MSDinfo.BytesLeft == 0) )
	    {
	      USB->EP2R = (1<<15)|(1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets
	      USB->EP3R = (1<<15)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets, clear CTR_TX flag
	    }
	  //if transfer is not over yet, or device actiually returned as much data as host expected
	  else
	    {
	      USB->EP2R = (1<<15)|(1<<7)|(2<<0);//respond with NAK to OUT packets, ignore IN packets
	      USB->EP3R = (1<<15)|(1<<4)|(3<<0);//respond to IN packets with data/CSW, ignore OUT packets, clear CTR_TX flag
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
    case 0x00://TEST UNIT READY command
      processTestUnitReadyCommand_6();
      break;
      
    case 0x12://INQUIRY command
      processInquiryCommand_6();
      break;

    case 0x1A://MODE SENSE (6) command
      processModeSenseCommand_6();
      break;      
      
    case 0x1E://PREVENT ALLOW MEDIUM REMOVAL command
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
      sendCSW(2);//return error status, request reset recovery

      USB->EP2R = (1<<13)|(1<<12)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets
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
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
  
  return;
}

static void processPreventAllowMediumRemovalCommand_6()
{
  //do some medium access control here

  sendCSW(0);//return good status
  
  USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
  USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets

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
      sendData();

      USB->EP2R = (1<<7)|(2<<0);//respond to OUT packets with NAK, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<4)|(3<<0);//respond to IN packets with data, ingore OUT packets
    }

  PayloadInfo.FirstRead = 1;//used to implement DELAY functionality in ducky interpreter from main.c
  return;
}

static void processWriteCommand_10()
{
  //convert logical block address from big endian to little endian, map LBA address to byte address in external device memory
  MSDinfo.DataPointer = ( ((MSDinfo.CBW).CBWCB[2] << 24) | ((MSDinfo.CBW).CBWCB[3] << 16) | ((MSDinfo.CBW).CBWCB[4] << 8) | ((MSDinfo.CBW).CBWCB[5] << 0) ) * 512 + MSD_FIRSTADDR;
  MSDinfo.WriteStart = MSDinfo.DataPointer - MSD_FIRSTADDR - MSDinfo.DataPointer % 4096;//start writing from specified 4096 byte block
  MSDinfo.EraseStart = MSDinfo.WriteStart;//start erasing blocks from specified 4096 byte block
  
  //if specified address range is outside the area from MSD_FIRSTADDR to MSD_LASTADDR
  if( (MSDinfo.DataPointer + MSDinfo.BytesLeft - 1) > MSD_LASTADDR )
    {
      sendCSW(2);//return error status, request reset recovery
      
      USB->EP2R = (1<<13)|(1<<12)|(1<<7)|(2<<0);//respond to OUT packets with STALL, ignore IN packets, clear CTR_RX flag
      USB->EP3R = (1<<15)|(1<<7)|(1<<5)|(1<<4)|(3<<0);//respond to IN packets with STALL, ignore OUT packets            
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
      else
	{
	  //pre-fill WriteBuffer with existing data on the medium if necessary
	  disk_read(0, (unsigned char*) &WriteBuffer, MSDinfo.WriteStart / 512, (MSDinfo.DataPointer % 4096) / 512 );

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
  if( responseSize < MSDinfo.BytesLeft) MSDinfo.BytesLeft = responseSize;

  //if last data transaction was just completed
  if(MSDinfo.BytesLeft == 0)
    {
      sendCSW(0);//return good status
    }

  //if one transaction is enough to transfer all remaining data
  else if(MSDinfo.BytesLeft <= MAXPACKET_MSD)
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
  //if last data transaction was just completed
  if(MSDinfo.BytesLeft == 0)
    {
      sendCSW(0);//return good status
    }

  //if all requested data was not sent yet
  else
    {
      //fill 512 byte read buffer with new data from the medium every time the block boundary is encountered
      if( (MSDinfo.DataPointer % 512) == 0 ) disk_read(0, (unsigned char*) &ReadBuffer, (MSDinfo.DataPointer - MSD_FIRSTADDR) / 512, 1);
      
      bufferCopy( (unsigned short*) ((unsigned int) &ReadBuffer + MSDinfo.DataPointer % 512), (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR3_TX), MAXPACKET_MSD );
      BTABLE->COUNT3_TX = MAXPACKET_MSD;
      
      (MSDinfo.CSW).dCSWDataResidue = (MSDinfo.CSW).dCSWDataResidue - MAXPACKET_MSD;      
      MSDinfo.BytesLeft = MSDinfo.BytesLeft - MAXPACKET_MSD;
      MSDinfo.DataPointer = (unsigned int) MSDinfo.DataPointer + MAXPACKET_MSD;
      MSDinfo.MSDstage = MSD_IN;
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
