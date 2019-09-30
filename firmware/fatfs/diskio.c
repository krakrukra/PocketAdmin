#include "../cmsis/stm32f0xx.h"
#include "diskio.h"

#define CS_LOW  GPIOA->BSRR = (1<<31)
#define CS_HIGH GPIOA->BSRR = (1<<15)

extern unsigned char MSDbuffer[1024];

//terminology: EB = physical 64KiB Erase Block; EBI = Erase Block Index (from 0 to 511);
//PB = 512Byte Physical Block (sector); PBO = Physical Block Offset (from 0 to 127) of a PB in it's EB;
//LB = 512Byte Logical Block (sector); LBA = Logical Block Address; LS = 55KiB Logical Set (of 110 LB's);
//LSI = Logical Set Index (from 0 to 447); LBO = Logical Block Offset (from 0 to 109) of a LB in it's LS;

unsigned short EBImap[512];//contains mapping from LSI to EBI in this way: EBImap[LBA/110] & 0x3FFF = EBI
unsigned short PBOmap[128];//contains mapping from LBO to PBO in this way: PBOmap[LBA%110] & 0x3FFF = PBO

//blocks of 110 contiguous LBA's all belong to the same LS and are all stored in the same EB. Values in EBImap[LSI] have this
//format: 14 LSbits contain the index of EB where the specified LS data is stored into; 2 MSbits are status bits, where
//11 = EB is erased, 01 = EB has valid data, 00 = EB has invalid data, 10 = EB is marked as bad; The status bits are NOT
//using LSI (same as LBA/110) as an index, but use actual EBI. That is, status bits in EBImap[3] always contain status for
//the physical EBI = 3, regardless of what is stored in the other 14 bits. If EBImap[LSI] & 0x3FFF == 0x3FFF, that means none
//of the blocks from the specified LS were programmed yet, and hence there is no EBI associated with this LSI.

//To minimize RAM usage the LB's are not mapped to PB's for the entire flash memory (using absolute addresses), but are only
//mapped for a single LS + EB pair; the index of that EB is stored in PBOmap[127] (14 LSbits hold EBI, 2 MSbits are set to 0);
//so after you know what LSI is mapped to which EBI you can do LB to PB mapping using the offsets instead (map LBO to PBO);
//the values in PBOmap[LBO] have this format: 14LSbits contain the PB offset of where a LB with a specific LBO is stored;
//2 MSbits are status bits, where 11 = PB is erased, 01 = PB has valid data, 00 = PB has invalid data, 10 = PB is marked as bad;
//The status bits are NOT using LBO (same as LBA%110) as an index, but use actual PBO. That is, status bits in PBOmap[5] always
//contain status for the actual physical PBO = 5 in EBI = PBO[127] & 0x3FFF, regardless of what is sotred in the other 14 bits.
//If PBOmap[LBO] & 0x3FFF == 0x3FFF, that means the specified LB was not programmed yet, and hence hass no PBO associated with this LB.

unsigned short RelocationBuffer[256];

DiskInfo_TypeDef DiskInfo =
{
  .TransferStatus = DISK_IDLE,
  .pdrv0_status = STA_NOINIT,
  .TransferByte = 0xFF,
  .LastErasedEB = 0,
  .DataPointer = 0,
  .BytesLeft = 0
};

static unsigned short findfree_EB(unsigned short startEBI);
static unsigned short findfree_PB();
static void erase_EB(unsigned short EBindex);
static void metawrite_EB(unsigned short EBindex, unsigned short status);
static void read_PB(unsigned char* targetAddr, unsigned short EBindex, unsigned short PBoffset);
static void write_PB(unsigned char* sourceAddr, unsigned short EBindex, unsigned short PBoffset);
static void dmaread_PB(unsigned char* targetAddr, unsigned short EBindex, unsigned short PBoffset);
static void dmawrite_PB(unsigned char* sourceAddr, unsigned short EBindex, unsigned short PBoffset);
static void write_enable();
static void wait_notbusy();
static unsigned char spi_transfer(unsigned int txdata);
static void restart_timer();
  
//----------------------------------------------------------------------------------------------------------------------

DSTATUS disk_initialize (BYTE pdrv)
{
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status == 0) return 0;//if drive is already initialized, do nothing
  
  CS_LOW;//pull CS pin low
  spi_transfer(0xB7);//Enter 4-Byte Address Mode command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  CS_HIGH;//pull CS pin high

  readmap_EBI(); //initialize EBImap[] based on flash metadata
  readmap_PBO(0);//initialize PBOmap[] based on flash metadata
  
  //reinitialize disk state machine, set pdrv0_status to 0
  DiskInfo.TransferStatus = DISK_IDLE;
  DiskInfo.pdrv0_status = 0;
  DiskInfo.TransferByte = 0xFF;
  DiskInfo.LastErasedEB = 0;
  DiskInfo.DataPointer = 0;
  DiskInfo.BytesLeft = 0;
  
  return 0;
}

DSTATUS disk_status (BYTE pdrv)
{  
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available

  return DiskInfo.pdrv0_status;
}

DRESULT disk_read (
	BYTE pdrv,	/* Physical drive number to identify the drive */
	BYTE* buff,	/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count	/* Number of sectors to read */
)
{
  unsigned int i;//used in a for() loop
  unsigned short LSI;//holds LS index for currently processed LB
  unsigned short LBO;//holds LB offset of currently processed LB
  unsigned short EBI;//holds EB index for currently processed LB
  unsigned short PBO;//holds PB offset of currently processed LB
  
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  49280) return RES_PARERR;//last accessible LBA is 49279
  if(count == 0) return RES_OK;//read 0 sectors request immediately returns success  
  while(DiskInfo.TransferStatus != DISK_IDLE);//wait until any ongoing transfers are over
  
  //keep going until specified number of sectors is read
  while(count)
    {
      //split current LBA into LSI and LBO values
      LSI = sector / 110;//find LSI value for currently processed LB
      EBI = EBImap[LSI] & 0x3FFF;//find which EB holds data of current LS
      
      if( EBI < 512 )//if some EB is found
	{
	  if( EBI != PBOmap[127] ) readmap_PBO(EBI);//make sure PBOmap[] is valid for current EBI
	  
	  LBO = sector % 110;
	  PBO = PBOmap[LBO] & 0x3FFF;//find in which PB current LB is stored
	  
	  if(PBO < 127)//if some PB was found
	    {
	      read_PB(buff, EBI, PBO);//read specified data from flash
	    }
	  else//if specified LB has no PB linked to it
	    {
	      for(i=0; i<512; i++) buff[i] = 0xFF;//fill data buffer with all 0xFF values
	    }
	}
      
      else//if specified LS has no EB linked to it
	{	  
	  for(i=0; i<512; i++) buff[i] = 0xFF;//fill data buffer with all 0xFF values
	}
      
      buff = buff + 512;//512 more bytes have been written into data buffer
      sector++;//move on to the next LB
      count--;//one more LB was processed
    }
  
  restart_timer();//prevent background erasing for the next 500ms  
  return RES_OK;
}

DRESULT disk_write (
                    BYTE pdrv,         /* Physical drive number to identify the drive */
                    const BYTE* buff,  /* Data to be written */
                    DWORD sector,      /* Start sector in LBA */
                    UINT count	       /* Number of sectors to write */
)
{
  unsigned short LSI;//holds LS index for currently processed LB
  unsigned short LBO;//holds LB offset of currently processed LB
  unsigned short EBI;//holds EB index for currently processed LB
  unsigned short PBO;//holds PB offset of currently processed LB
  unsigned short LScount;//holds number of LS's that will be involved in the process
  
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  49280) return RES_PARERR;//last accessible LBA is 49279
  if(count == 0) return RES_OK;//write 0 sectors request immediately returns success
  while(DiskInfo.TransferStatus != DISK_IDLE);//wait until any ongoing transfers are over
    
  prepare_LB(sector, count);//prepare specified LB's to be written
  LScount = (sector + count - 1) / 110 - (sector / 110) + 1;//calculate how many LS's will be written to
  
  //keep going until there still are LS's to process
  while(LScount)
    { 
      LSI = sector / 110;//find which LS current LB belongs to
      LBO = sector % 110;//find which LBO value current LB has
      EBI = EBImap[LSI] & 0x3FFF;//find which EB current LB is stored in
      
      if( EBI < 512 )//if some EB is found (this should always be true after prepare_LB() )
	{
	  if( EBI != PBOmap[127] ) readmap_PBO(EBI);//make sure PBOmap[] is valid for current EBI
	  
	  //keep going until specified number of sectors is programmed or end of LS is reached
	  while((LBO < 110) && count)
	    {
	      PBO = findfree_PB();//find new free PB
	      
	      if(PBO < 127)//if some new free PB is found (this should always be true after prepare_LB() )
		{
		  PBOmap[PBO] &= 0x7FFF;//set new PB status to valid
		  PBOmap[LBO] &= (PBO | 0xC000);//link current LBO to offset of new PB
		  write_PB((unsigned char*) buff, EBI, PBO);//write data to new free PB from specified buffer
		}
	      else//if some error happened and no free PB was found
		{
		  return RES_ERROR;
		}
	      
	      buff = buff + 512;//512 more bytes have been written into data buffer
	      sector++;//move on to the next LB
	      LBO++;//move on to the next LB
	      count--;//one more LB was processed
	    }
	  
	  writemap_PBO();//save new PBOmap[] information in flash metadata
	}
      else//if some error happened and no free EB was found
	{
	  return RES_ERROR;
	}
      
      LScount--;
    }

  restart_timer();//prevent background erasing for the next 500ms
  return RES_OK;
}

DRESULT disk_ioctl (
	BYTE pdrv,   /* Physical drive number (0..) */
	BYTE cmd,    /* Control code */
	void* buff   /* Buffer to send/receive control data */
)
{
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  
  switch(cmd)
    {
    case CTRL_SYNC:
      return RES_OK;
      break;

    case GET_SECTOR_COUNT:
      *((DWORD*) buff) = 49280;
      return RES_OK;
      break;

    case GET_SECTOR_SIZE:
      *((WORD*) buff) = 512;
      return RES_OK;
      break;

    case GET_BLOCK_SIZE:
      *((DWORD*) buff) = 128;
      return RES_OK;
      break;

    case CTRL_TRIM:
      return RES_OK;
      break;
    }

  return RES_ERROR;
}

DWORD get_fattime (void)
{
  return (1<<21)|(1<<16);
}

//----------------------------------------------------------------------------------------------------------------------

DRESULT disk_dmaread (
	BYTE pdrv,	/* Physical drive number to identify the drive */
	BYTE* buff,	/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count	/* Number of sectors to read */
)
{
  unsigned short LSI;//holds LS index for currently processed LB
  unsigned short LBO;//holds LB offset of currently processed LB
  unsigned short EBI;//holds EB index for currently processed LB
  unsigned short PBO;//holds PB offset of currently processed LB
  
  //if no transfers are ongoing
  if(DiskInfo.TransferStatus == DISK_IDLE)
    {
      if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
      if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
      if( (sector + count) >  49280) return RES_PARERR;//last accessible LBA is 49279
      if(count == 0) return RES_OK;//if read 0 blocks was requested, do nothing
      
      //initialize a new read transfer
      DiskInfo.DataPointer = sector * 512;
      DiskInfo.BytesLeft = count * 512;
      DiskInfo.TransferStatus = DISK_READ;
    }
  
  if(DiskInfo.BytesLeft == 0)//if all requested transfers have been performed
    {      
      DiskInfo.TransferStatus = DISK_IDLE;//stop transferring the data
    }
  else//if there are still transfers to be performed
    {
      LSI = (DiskInfo.DataPointer / 512) / 110;//find LSI value for currently processed LB
      LBO = (DiskInfo.DataPointer / 512) % 110;//find LBO value for currently processed LB
      EBI = EBImap[LSI] & 0x3FFF;//find which EB holds data of current LS
      
      DiskInfo.DataPointer = DiskInfo.DataPointer + 512;//move on to the next LB
      DiskInfo.BytesLeft = DiskInfo.BytesLeft - 512;//512 more bytes were processed
      
      if( EBI < 512 )//if some EB is found
	{	  
	  if( EBI != PBOmap[127] ) readmap_PBO(EBI);//make sure PBOmap[] is valid for current EBI
	  PBO = PBOmap[LBO] & 0x3FFF;//find in which PB current LB is stored
	  
	  if(PBO < 127)//if some PB was found
	    {	      
	      dmaread_PB(buff, EBI, PBO);//read specified data from flash
	    }
	  else//if specified LB has no PB linked to it
	    {
	      //enable DMA channel 2, fill the specified buffer
	      DiskInfo.TransferByte = 0xFF;//prepare 0xFF value as DMA data source
	      DMA1_Channel2->CNDTR = 512;//transfer 512 bytes
	      DMA1_Channel2->CPAR = (unsigned int) &(DiskInfo.TransferByte);//do a memory-to-memory transfer from TransferByte
	      DMA1_Channel2->CMAR = (unsigned int) buff;//fill specified buffer with all 0xFF values
	      DMA1_Channel2->CCR = (1<<14)|(1<<7)|(1<<1)|(1<<0);//byte access, memory increment mode, enable TC interrupt
	    }
	}
      
      else//if specified LS has no EB linked to it
	{	  	  
	  //enable DMA channel 2, fill the specified buffer
	  DiskInfo.TransferByte = 0xFF;//prepare 0xFF value as DMA data source
	  DMA1_Channel2->CNDTR = 512;//transfer 512 bytes
	  DMA1_Channel2->CPAR = (unsigned int) &(DiskInfo.TransferByte);//do a memory-to-memory transfer from TransferByte
	  DMA1_Channel2->CMAR = (unsigned int) buff;//fill specified buffer with all 0xFF values
	  DMA1_Channel2->CCR = (1<<14)|(1<<7)|(1<<1)|(1<<0);//byte access, memory increment mode, enable TC interrupt	  
	}
    }
  
  
  restart_timer();//prevent background erasing for the next 500ms  
  return RES_OK;
}

DRESULT disk_dmawrite (
                    BYTE pdrv,         /* Physical drive number to identify the drive */
                    const BYTE* buff,  /* Data to be written */
                    DWORD sector,      /* Start sector in LBA */
                    UINT count	       /* Number of sectors to write */
)
{  
  unsigned short LSI;//holds LS index for currently processed LB
  unsigned short LBO;//holds LB offset of currently processed LB
  unsigned short EBI;//holds EB index for currently processed LB
  unsigned short PBO;//holds PB offset of currently processed LB

  //if no transfers are ongoing
  if(DiskInfo.TransferStatus == DISK_IDLE)
    {
      if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
      if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
      if( (sector + count) >  49280) return RES_PARERR;//last accessible LBA is 49279
      if(count == 0) return RES_OK;//if write 0 blocks was requested, do nothing
      
      //initialize a new write transfer
      DiskInfo.DataPointer = sector * 512;
      DiskInfo.BytesLeft = count * 512;
      DiskInfo.TransferStatus = DISK_WRITE;
      
      //prepare all the blocks affected by the requested sequence of transfers, unless MSDbuffer[]
      //is used as data source; it is used for USB transfers, where prepare_LB() is called separately
      if( (buff < &MSDbuffer[0]) || (buff > &MSDbuffer[1023]) ) prepare_LB(sector, count);
    }
  
  if(DiskInfo.BytesLeft == 0)//if all requested transfers have been performed
    {      
      DiskInfo.TransferStatus = DISK_IDLE;//stop transferring the data
    }
  else//if there are still transfers to be performed
    {
      LSI = (DiskInfo.DataPointer / 512) / 110;//find which LS current LB belongs to
      LBO = (DiskInfo.DataPointer / 512) % 110;//find which LBO value current LB has
      EBI = EBImap[LSI] & 0x3FFF;//find which EB current LB is stored in
      
      DiskInfo.DataPointer = DiskInfo.DataPointer + 256;//move on to the next page
      DiskInfo.BytesLeft = DiskInfo.BytesLeft - 256;//256 more bytes were processed      
      
      if( EBI < 512 )//if some EB is found (this should always be true after prepare_LB() is called)
	{
	  if( EBI != PBOmap[127] ) readmap_PBO(EBI);//make sure PBOmap[] is valid for current EBI
	  
	  if((DiskInfo.DataPointer % 512) == 256) PBO = findfree_PB();//if current LB was not written yet, find new free PB for it
	  else PBO = PBOmap[LBO] & 0x3FFF;//if current LB is already half-written, find it's PB through PBOmap[]
	  
	  if(PBO < 127)//if some free/half-full PB is found (this should always be true after prepare_LB() is called)
	    {
	      PBOmap[PBO] &= 0x7FFF;//set new PB status to valid
	      PBOmap[LBO] &= (PBO | 0xC000);//link current LBO to offset of the new PB

	      //save new PBOmap[] information in flash metadata if LS boundary or last transfer is detected. But only do that
	      //if MSDbuffer[] is NOT used as data source; it is used for USB transfers, where writemap_PBO() is called separately
	      if( (buff < &MSDbuffer[0]) || (buff > &MSDbuffer[1023]) )
		{
		  if( (DiskInfo.BytesLeft == 0) || ( (DiskInfo.DataPointer % (512 * 110)) == 0 ) ) writemap_PBO();
		}
	      
	      dmawrite_PB((unsigned char*) buff, EBI, PBO);//write data to new free PB from specified buffer
	    }
	  else//if some error happened and no free PB was found
	    {
	      //stop transferring the data, return error
	      DiskInfo.TransferStatus = DISK_IDLE;
	      return RES_ERROR;
	    }
	}
      
      else//if some error happened and no valid EB was found
	{
	  //stop transferring the data, return error
	  DiskInfo.TransferStatus = DISK_IDLE;
	  return RES_ERROR;
	}
    }
  
  
  restart_timer();//prevent background erasing for the next 500ms
  return RES_OK;
}

void dma_handler()
{
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high  
  
  SPI1->CR1 &= ~(1<<6);//disable SPI1
  SPI1->CR2 &= ~((1<<1)|(1<<0));//disable DMA requests for SPI1
  SPI1->CR1 |= (1<<6);//enable SPI1 with new configuration
  
  DMA1_Channel2->CCR = 0;//disable DMA channel 2
  DMA1_Channel3->CCR = 0;//disable DMA channel 3
  DMA1->IFCR = (1<<5)|(1<<4);//clear DMA channel 2 TC flag

       if(DiskInfo.TransferStatus == DISK_READ)  disk_dmaread( 0, (unsigned char*) (DMA1_Channel2->CMAR + 512), 0, 0);
  else if(DiskInfo.TransferStatus == DISK_WRITE) disk_dmawrite(0, (unsigned char*) (DMA1_Channel3->CMAR + 256), 0, 0);
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//read the last sector in all EB's to fill the EBImap[] with correct values based on flash metadata
void readmap_EBI()
{
  unsigned short i;//used in a for() loop, holds index of currently processed EB
  unsigned short EBstatus;//holds EB status bits for currently processed EB
  unsigned short LSindex;//holds which LS was stored in currently processed EB
  unsigned int address;//holds byte address in flash memory where current EB metadata is
  
  for(i=0; i<512; i++) EBImap[i] = 0x3FFF;//pre-fill all of EBImap[] with 0x3FFF values
  
  //read the last 2 bytes in every EB, write corresponding values in EBImap[]
  for(i=0; i<512; i++)
    {
      address = ((i + 1) * 64 * 1024) - 2;//calculate the flash byte address where to read the EB metadata from
      wait_notbusy();//make sure that a new command can be accepted
      
      CS_LOW;//pull CS pin low
      spi_transfer(0x03);//Read Data command (4 byte address)
      spi_transfer(address >> 24);//send read address
      spi_transfer(address >> 16);//send read address
      spi_transfer(address >> 8);//send read address
      spi_transfer(address >> 0);//send read address
      address  = (spi_transfer(0x00) << 8);//get the data
      address |= (spi_transfer(0x00) << 0);//get the data
      while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
      CS_HIGH;//pull CS pin high
      
      //split the received data into EBstatus and LSindex
      EBstatus = address & 0xC000;
      LSindex = address & 0x3FFF;
      
      //if some set of LB's was stored in this EB and the mapping is still valid,
      //save this information in EBImap[], then set the status bits for current EB
      if( (LSindex < 448) && (EBstatus == 0x4000) ) EBImap[LSindex] &= (i | 0xC000);
      EBImap[i] |= EBstatus;
    }

  restart_timer();//prevent background erasing for the next 500ms
  return;
}

//read the last sector in a specified EB to fill the PBOmap[] with correct values based on flash metadata
void readmap_PBO(unsigned short EBI)
{
  unsigned short i;//used in a for() loop, represents currently processed PB
  unsigned short PBstatus;//holds PB status flags for currently processed PB
  unsigned short LBoffset;//holds which LB was stored in currently processed PB
  unsigned int address;//holds byte address in flash memory where current PB metadata is

  if(EBI > 511) return;//if invalid EBI was specified, do nothing
  
  for(i=0; i<128; i++) PBOmap[i] = 0x3FFF;//pre-fill all of PBOmap[] with 0x3FFF values
  address = ((EBI + 1) * 64 * 1024) - 256;//calculate the flash byte address where to read the PB's metadata from
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer(address >> 24);//send read address
  spi_transfer(address >> 16);//send read address
  spi_transfer(address >> 8);//send read address
  spi_transfer(address >> 0);//send read address
  
  //read 2 bytes for every PB, write corresponding values in PBOmap[]
  for(i=0; i<127; i++)
    {
      address  = (spi_transfer(0x00) << 8);//get the data
      address |= (spi_transfer(0x00) << 0);//get the data
      
      //split the received data into PBstatus and LBoffset
      PBstatus = address & 0xC000;
      LBoffset = address & 0x3FFF;           
      
      //if some LB was stored in this PB and the mapping is still valid,
      //save this information in PBOmap[], then set the status bits for current PB
      if( (LBoffset < 110) && (PBstatus == 0x4000) ) PBOmap[LBoffset] &= (i | 0xC000);
      PBOmap[i] |= PBstatus;
    }
  
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high  
  
  PBOmap[127] = EBI;//store the EBI value at the end of PBOmap[]
  restart_timer();//prevent background erasing for the next 500ms
  return;
}

//search through PBOmap[] to find information about every PB, then clear necessary bits in flash metadata; can not set any bits
void writemap_PBO()
{
  unsigned short i;//used in a for() loop
  unsigned short PBO;//hold PBO for currently processed LBO
  unsigned int address;//holds flash byte address of where target PB metadata is
  
  if(PBOmap[127] > 511 ) return;//if invalid EBI is specified, do nothing  
  address = ((PBOmap[127] + 1) * 64 * 1024) - 256;//calculate the flash byte address where to write the PB's metadata to
  
  for(i=0; i<127; i++) RelocationBuffer[i] = ( PBOmap[i] | 0x3FFF );//pre-fill RelocationBuffer[] with PB status bits, set all map bits to 1
  
  //convert PBOmap[] into flash metadata array ( temporarily stored in RelocationBuffer[] )
  for(i=0; i<110; i++)
    {
      PBO = PBOmap[i] & 0x3FFF;
      if(PBO < 127) RelocationBuffer[PBO] &= (i | 0xC000);
    }
  
  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x02);//Page Program command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  
  for(i=0; i<127; i++)
    {      
      spi_transfer( RelocationBuffer[i] >> 8 );
      spi_transfer( RelocationBuffer[i] >> 0 );
    }
  
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high    

  restart_timer();//prevent background erasing for the next 500ms
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//prepare specified LB's to be rewritten; write appropriate values in flash metadata, EBImap[], PBOmap[]
void prepare_LB(unsigned int LBaddress, unsigned int LBcount)
{
  unsigned short i;//used in a for() loop
  unsigned short freePBcount;//holds how many PB's are available in a given LS
  unsigned short needPBcount;//holds how many PB's will need to be written in a given LS
  unsigned short LScount;//holds how many LS's are affected by the operation
  
  unsigned short EBI;//holds EB index  which corresponds to currently processed LB
  unsigned short PBO;//holds PB offset which corresponds to currently processed LB
  unsigned short LSI;//holds LS index  which corresponds to currently processed LB
  unsigned short LBO;//holds LB offset which corresponds to currently processed LB
  
  if( (LBaddress + LBcount) > 49280 ) return;//if invalid LBA is specified, do nothing
  if(LBcount == 0) return;//if zero blocks need to be prepared, do nothing
  
  LScount = ((LBaddress + LBcount - 1) / 110) - (LBaddress / 110) + 1;//calculate how many LS's need to be prepared
  
  //keep going until all LS's are prepared
  while(LScount)
    {
      LSI = LBaddress/110;//find which LS current LB belongs to
      LBO = LBaddress%110;//find which LBO value current LB has
      EBI = EBImap[LSI] & 0x3FFF;//find which EB current LB is stored in
      
      if(EBI < 512)//if some EB is actually found
      {	
	if( EBI != PBOmap[127] ) readmap_PBO(EBI);//make sure PBOmap[] is valid for current EBI
	
	//calculate how many PB's are free in current EB
	freePBcount = 0;
	for(i=0; i<127; i++) if((PBOmap[i] & 0xC000) == 0xC000) freePBcount++;
	
	//calculate how many PB's need to be programmed in current EB
	if((110 - LBO) < LBcount) needPBcount = 110 - LBO;
	else needPBcount = LBcount;
	
	//mark all necessary PB's as invalid in PBOmap[]
	while((LBO < 110) && LBcount)
	  {
	    PBO = PBOmap[LBO] & 0x3FFF;//find which PB current LB is stored in
	    
	    //if some PB is actually found, set it's status to invalid in PBOmap[]
	    if(PBO < 127)
	      {
		PBOmap[PBO] &= 0x3FFF;//update PBO map with new information about target PB (set status to invalid)
		PBOmap[LBO] |= 0x3FFF;//specified LB is no longer linked to any PB in PBOmap[]
	      }
	    
	    LBaddress++;
	    LBO++;
	    LBcount--;
	  }
	
	//if more blocks need to be written than there are free blocks, relocate current LS to some new EB
	if(freePBcount < needPBcount) relocate_LS(LSI);
	else writemap_PBO();//if there are enough free blocks, keep LS in the current EB
      }
      
      else//if no EB is associated with specified LS
	{
	  relocate_LS(LSI);//link specified LS to a new EB	  
	  
	  //move on to the next LS
	  LBaddress = LBaddress + (110 - LBO);
	  LBcount = LBcount - (110 - LBO);
	}
      
      LScount--;//move on to the next LS
    }

  restart_timer();//prevent background erasing for the next 500ms
  return;
}

//move all the valid blocks that belong to specified LS into a new EB; update EBImap[] with new correct values
void relocate_LS(unsigned short LSindex)
{
  unsigned short oldEBI;//holds EB index of where LS is currently located
  unsigned short newEBI;//holds EB index of where LS will be moved into
  unsigned short i;//used in a for() loop
  unsigned short oldPBO;//holds PB offset of where current PB is stored at
  unsigned short newPBO;//holds PB offset of where current PB will be moved into
  
  if( LSindex > 447 ) return;//if invalid LSI is specified, do nothing     
  oldEBI = EBImap[LSindex] & 0x3FFF;//find which EB currently stores specified LS
  
  //if specified LS was already previously stored in some EB
  if(oldEBI < 512)
    {
      EBImap[oldEBI] &= 0x3FFF;//set old EB status bits to invalid in EBImap[]      
      EBImap[LSindex] |= 0x3FFF;//specified LSindex is no longer linked to any EB in EBImap[]
      metawrite_EB(oldEBI, 0x3FFF);//invalidate old EB status in flash metadata
      
      newEBI = findfree_EB((oldEBI + 1) % 512);//find a new and erased EB
      if(newEBI > 511) newEBI = makefree_EB((oldEBI + 1) % 512);//if no free EB is found, erase some old invalid block      
      
      EBImap[newEBI] &= 0x7FFF;//set new EB status bits to valid in EBImap[]
      EBImap[LSindex] &= (newEBI | 0xC000);//link specified LS to EB with index = newEBI
      metawrite_EB(newEBI, LSindex | 0x4000);//write new EB metadata in flash
      
      
      if( oldEBI != PBOmap[127] ) readmap_PBO(oldEBI);//make sure PBOmap[] is valid for current EBI
      newPBO = 0;//start writing PB's at the start of new EB
      
      //copy all the used PB's from old EB to new EB, rewrite mapping bits in PBOmap[] to reflect that
      for(i=0; i<110; i++)
	{
	  oldPBO = PBOmap[i] & 0x3FFF;//find which PB corresponds to current LB
	  
	  if(oldPBO < 127)//if some PB was found
	    {
	      //set new correct LB to PB mapping in PBOmap[]
	      PBOmap[i] |= 0x3FFF;
	      PBOmap[i] &= (newPBO | 0xC000);
	      
	      //copy PB data
	      read_PB((unsigned char*) RelocationBuffer, oldEBI, oldPBO);
	      write_PB((unsigned char*) RelocationBuffer, newEBI, newPBO);	     
	      
	      newPBO++;//move to the next PBO in new EB
	    }
	}

      //mark newly copied PB's as valid by rewriting status bits in PBOmap[]
      for(i=0; i<127; i++)
	{
	  PBOmap[i] |= 0xC000;
	  if(i < newPBO) PBOmap[i] &= 0x7FFF;
	}
            
      PBOmap[127] = newEBI;//PBOmap[] was now completely rewritten to reflect mapping for new EB
      writemap_PBO();//write appropriate metadata in flash based on new PBOmap[]
    }
  
  //if no previous EB was found for specified LS
  else
    {
      newEBI = findfree_EB(0);//find a new and erased EB
      if(newEBI > 511) newEBI = makefree_EB(0);//if no free EB is found, erase some old invalid EB
      
      EBImap[newEBI] &= 0x7FFF;//set new EB status bits to valid in EBImap[]
      EBImap[LSindex] &= (newEBI | 0xC000);//link specified LS to EB with index = newEBI
      metawrite_EB(newEBI, LSindex | 0x4000);//write new EB metadata in flash
    }

  restart_timer();//prevent background erasing for the next 500ms
  return;
}

//find and erase the first invalid EB, with search starting from specified EB index; then return newly erased EB index
unsigned short makefree_EB(unsigned short startEBI)
{
  unsigned short i;//used in a for() loop 
  
  if(startEBI > 511) return 0x3FFF;//if invalid EBI is specified, do nothing
  
  //if not a single free EB was found, erase some old invalid EB
  for(i=0; i<512; i++)
    { 
      if((EBImap[startEBI] & 0xC000) == 0x0000 )//stop searching if some invalid EB is found
      {
	EBImap[startEBI] |= 0xC000;//set newly found EB status to erased
	erase_EB(startEBI);//erase the EB; this also writes new correct flash metadata
	DiskInfo.LastErasedEB = startEBI;//remember index of the last EB that was erased
	return startEBI;
      }
      
      //move on to the next EB
      if(startEBI < 511) startEBI++;
      else startEBI = 0;
    }
  
  //if no invalid block was found to erase
  restart_timer();//prevent background erasing for the next 500ms
  return 0x3FFF;
}

//----------------------------------------------------------------------------------------------------------------------

//search through EBImap[] and return index of a first free EB, with search starting from specified EB index
static unsigned short findfree_EB(unsigned short startEBI)
{
  unsigned short i;//used in a for() loop 
  
  if(startEBI > 511) return 0x3FFF;//if invalid EBI is specified, do nothing
  
  //keep searching for new free EB
  for(i=0; i<512; i++)
    { 
      if((EBImap[startEBI] & 0xC000) == 0xC000 ) return startEBI;//stop searching if free EB is found
      
      //move on to the next EB
      if(startEBI < 511) startEBI++;
      else startEBI = 0;
    }
  
  return 0x3FFF;
}

//search through PBOmap[] and return offset of a first free PB, with search starting from PBO = 0
static unsigned short findfree_PB()
{
  unsigned short i;//used in a for() loop 

  //keep searching for new free EB
  for(i=0; i<127; i++)
    { 
      if((PBOmap[i] & 0xC000) == 0xC000 ) return i;//stop searching if free PB is found
    }
    
  return 0x3FFF;
}

//----------------------------------------------------------------------------------------------------------------------

//erase specified EB based on index
static void erase_EB(unsigned short EBindex)
{
  unsigned int address;//holds flash byte address of an EB to erase
  
  if(EBindex  > 511) return;//if invalid EBI is specified, do nothing
  address = EBindex * 64 * 1024;//convert EBI into flash byte address

  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0xD8);//64 KiB Block Erase command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//clears specified bits in flash metadata for a particular EB; can not set any bits
static void metawrite_EB(unsigned short EBindex, unsigned short EBmetadata)
{
  unsigned int address;//holds flash byte address of where specified EB metadata is
  
  if(EBindex > 511) return;//if invalid EBI is specified, do nothing  
  address = (EBindex + 1) * 64 * 1024 - 2;//find byte address in flash where specified EB metadata is  

  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();        
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x02);//Page Program command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  spi_transfer( EBmetadata >> 8 );
  spi_transfer( EBmetadata >> 0 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//read specified PB from flash to a RAM buffer located at targetAddr
static void read_PB(unsigned char* targetAddr, unsigned short EBindex, unsigned short PBoffset)
{
  unsigned short k;//used in a for() loop
  unsigned int address;//holds flash byte address of a PB to read
  
  if(EBindex  > 511) return;//if invalid EBI is specified, do nothing
  if(PBoffset > 127) return;//if invalid PBO is specified, do nothing
  
  address = (EBindex * 64 * 1024) + (PBoffset * 512);//calculate flash byte address from EBI and PBO
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  for(k=0; k<512; k++) targetAddr[k] = spi_transfer( 0x00 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//write specified PB in flash from a RAM buffer located at sourceAddr; medium must be pre-erased at target PB location
static void write_PB(unsigned char* sourceAddr, unsigned short EBindex, unsigned short PBoffset)
{
  unsigned short j;//used in a for() loop
  unsigned short k;//used in a for() loop
  unsigned int address;//holds flash byte address of a PB to write

  if(EBindex  > 511) return;//if invalid EBI is specified, do nothing
  if(PBoffset > 127) return;//if invalid PBO is specified, do nothing
  
  address = (EBindex * 64 * 1024) + (PBoffset * 512);//calculate flash byte address from EBI and PBO
  
  //keep going until specified number of pages is written
  for(j=0; j<2; j++)
    {
      //make sure that a new command can be accepted
      wait_notbusy();
      write_enable();
      
      CS_LOW;//pull CS pin low
      spi_transfer(0x02);//Page Program command (4 byte address)
      spi_transfer( (address + j * 256) >> 24 );
      spi_transfer( (address + j * 256) >> 16 );
      spi_transfer( (address + j * 256) >> 8 );
      spi_transfer( (address + j * 256) >> 0 );
      for(k=0; k<256; k++) spi_transfer( sourceAddr[j * 256 + k] );
      while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
      CS_HIGH;//pull CS pin high            
    }
  
  return;
}

//different implementation of read_PB() which uses DMA for transfers
static void dmaread_PB(unsigned char* targetAddr, unsigned short EBindex, unsigned short PBoffset)
{
  unsigned int address;//holds flash byte address of a PB to read
  
  if(EBindex  > 511) return;//if invalid EBI is specified, do nothing
  if(PBoffset > 127) return;//if invalid PBO is specified, do nothing
  
  address = (EBindex * 64 * 1024) + (PBoffset * 512);//calculate flash byte address from EBI and PBO
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  
  //enable DMA channel 2, get data from the medium
  DMA1_Channel2->CNDTR = 512;//transfer 512 bytes
  DMA1_Channel2->CPAR = (unsigned int) &(SPI1->DR);//receive data from SPI1 RXFIFO 
  DMA1_Channel2->CMAR = (unsigned int) targetAddr;//fill specified buffer with data
  DMA1_Channel2->CCR = (1<<7)|(1<<1)|(1<<0);//byte access, memory increment mode, enable TC interrupt
  
  //enable DMA channel 3, send all 0x00 values to the memory chip
  DiskInfo.TransferByte = 0x00;//prepare 0x00 value as DMA data source
  DMA1_Channel3->CNDTR = 512;//transfer 512 bytes
  DMA1_Channel3->CPAR = (unsigned int) &(SPI1->DR);//send data to SPI1 TXFIFO
  DMA1_Channel3->CMAR = (unsigned int) &(DiskInfo.TransferByte);//send all 0x00 values
  DMA1_Channel3->CCR = (1<<4)|(1<<0);//byte access, transfer from memory to peripheral
  
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  SPI1->CR1 &= ~(1<<6);//disable SPI1
  SPI1->CR2 |= (1<<1)|(1<<0);//enable DMA requests for SPI1
  SPI1->CR1 |= (1<<6);//enable SPI1 with new configuration
  
  return;
}

//different implementation of write_PB() which uses DMA for transfers
static void dmawrite_PB(unsigned char* sourceAddr, unsigned short EBindex, unsigned short PBoffset)
{
  unsigned int address;//holds flash byte address of a PB to write
  
  if(EBindex  > 511) return;//if invalid EBI is specified, do nothing
  if(PBoffset > 127) return;//if invalid PBO is specified, do nothing

  //calculate flash byte address from EBI and PBO; if target PB is already half-written, write to the next half
  if((DiskInfo.DataPointer % 512) == 256) address = (EBindex * 64 * 1024) + (PBoffset * 512);
  else                                    address = (EBindex * 64 * 1024) + (PBoffset * 512) + 256;
  
  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x02);//Page Program command (4 byte address)
  spi_transfer( address >> 24 );
  spi_transfer( address >> 16 );
  spi_transfer( address >> 8 );
  spi_transfer( address >> 0 );
  
  //enable DMA channel 2, get data from the medium
  DMA1_Channel2->CNDTR = 256;//transfer 256 bytes
  DMA1_Channel2->CPAR = (unsigned int) &(SPI1->DR);//receive data from RXFIFO
  DMA1_Channel2->CMAR = (unsigned int) &(DiskInfo.TransferByte);//ignore any data sent back
  DMA1_Channel2->CCR = (1<<1)|(1<<0);//byte access, enable TC interrupt
  
  //enable DMA channel 3, send data to the memory chip
  DMA1_Channel3->CNDTR = 256;//transfer 256 bytes
  DMA1_Channel3->CPAR = (unsigned int) &(SPI1->DR);//send data to TXFIFO
  DMA1_Channel3->CMAR = (unsigned int) sourceAddr;//send the data from specified buffer
  DMA1_Channel3->CCR = (1<<7)|(1<<4)|(1<<0);//byte access, memory increment mode, transfer from memory to peripheral

  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  SPI1->CR1 &= ~(1<<6);//disable SPI1
  SPI1->CR2 |= (1<<1)|(1<<0);//enable DMA requests for SPI1
  SPI1->CR1 |= (1<<6);//enable SPI1 with new configuration

  return;
}

//----------------------------------------------------------------------------------------------------------------------

static void write_enable()
{
  CS_LOW;//pull CS pin low  
  spi_transfer(0x06);//Write Enable command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  CS_HIGH;//pull CS pin high
  
  return;
}

static void wait_notbusy()
{    
  CS_LOW;//pull CS pin low
  spi_transfer(0x05);//Read Status Register 1 command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  while( spi_transfer(0x00) & (1<<0) );//keep sending clock until flash memory busy flag is reset back to 0
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//only the least significant byte will be sent
static unsigned char spi_transfer(unsigned int txdata)
{
  while( !(SPI1->SR & (1<<1)) );//wait until TX FIFO has enough space
  *( (unsigned char*) &(SPI1->DR) ) = txdata;//send txdata to the FIFO
  while( !(SPI1->SR & (1<<0)) );//wait until new packet is received
  
  return (unsigned char) SPI1->DR;
}

static void restart_timer()
{
  TIM3->CR1 = 0;//disable TIM3 (in case it was running)
  
  //start the timer, run for approximately 500ms
  TIM3->ARR = 24000;//timer 3 reload value is 24000
  TIM3->PSC = 999;//TIM2 prescaler = 1000
  TIM3->EGR = (1<<0);//generate update event
  TIM3->CR1 = (1<<7)|(1<<3)|(1<<0);//ARR is buffered, one pulse mode, start upcounting  

  return;
}
