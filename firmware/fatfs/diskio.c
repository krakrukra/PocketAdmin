#include "../cmsis/stm32f0xx.h"
#include "diskio.h"

#define CS_LOW  GPIOB->BSRR = (1<<17)
#define CS_HIGH GPIOB->BSRR = (1<<1)

//EB = physical 128KiB Erase Block; EBI = Erase Block Index (from 0 to 1023);
//LS = 112KiB Logical Set (of 56 LP's / 224 LB's); LSI = Logical Set Index (from 0 to 879);
//PP = 2048Byte Physical Page; PPO = Physical Page Offset (from 0 to 63, inside some EB);
//LP = 2048Byte Logical Page;  LPO = Logical  Page Offset (from 0 to 55, inside some LS);
//PPA = Physical Page Address (in page units); LPA = Logical Page Address (in page units);
//PB = 512Byte Physical Block (sector); PBA = Physical Block Address (in block units);
//LB = 512Byte Logical  Block (sector); LBA = Logical  Block Address (in block units);
//LBO = Logical Block Offset (from 0 to 3, inside some LP); PBO = Physical Block Offset (equal to LBO)

static unsigned short EBImap[1024];//contains mapping from LSI to EBI in this way: EBImap[LPA/56] & 0x3FFF = EBI
static unsigned char  PPOmap[56];//  contains mapping from LPO to PPO in this way: PPOmap[LPA%56] = PPO

//blocks of 224 contiguous LBA's all belong to the same LS and are all stored in the same EB. Values in EBImap[LSI] have this
//format: 14 LSbits contain the index of EB where the specified LS data is stored into; 2 MSbits are status bits, where
//11 = EB is erased, 01 = EB has valid data, 00 = EB has invalid data, 10 = EB is marked as bad; The status bits are NOT
//using LSI (same as LBA/224) as an index, but use actual EBI. That is, status bits in EBImap[3] always contain status for
//the physical EBI = 3, regardless of what is stored in the other 14 bits. If EBImap[LSI] & 0x3FFF == 0x3FFF, that means none
//of the blocks from the specified LS were programmed yet, and hence there is no EBI associated with this LSI.

//To minimize RAM usage there is no LPA to PPA mapping table for the entire flash memory (using absolute addresses),
//but there  is a LPO to PPO mapping table for a single LS + EB pair, stored in PPOmap[]; index of the EB for which PPOmap[]
//is currently valid is stored in DiskInfo.PPOmapValidEBI (2 MSbits are set to 0); If PPOmap[LPO] == 0xFF, that means the
//specified LP was not programmed yet, and hence has no PP associated with it. The biggest valid PPO (0 to 63) in PPOmap[]
//is stored in DiskInfo.PPOmapLastPPO, which is the same as PPO of the last used page inside the EB for which PPOmap[] is valid

//in flash there is a 64Byte spare area for each page. This spare area will be used like this: first 2 bytes are a bad block
//marker (as specified in datasheet), next 2 bytes are LSI marker (2 MSbits are status bits for the EB that the page belongs to,
//14LSbits contain index of the LS which is stored inside the EB), LSImarker is stored in flash LSByte first; next 56 bytes are
//PPOmap[] which was valid for the page's EB at the time the page was written. So, the latest page that was written to in some EB
//has the most up to date PPOmap[] for the entire EB, as well as the current status of that EB (erased, valid, invalid, bad)

DiskInfo_TypeDef DiskInfo =
{
  .LastErasedEB = 0,
  .BufferPageAddr = 0,
  .PPOmapValidEBI = 0,
  .PPOmapLastPPO = 0xFF,
  .TransferByte = 0xFF,
  .pdrv0_status = STA_NOINIT,
  .WritePageFlag = 0,
  .BusyFlag = 0
};

static void relocate_LS(unsigned short LSindex);
static void readmap_EBI();
static unsigned short readmap_PPO(unsigned short EBI);
static unsigned short makefree_EB(unsigned short startEBI);
static unsigned short findfree_EB(unsigned short startEBI);
static void erase_EB(unsigned short EBindex);
static void read_PP(unsigned short PageAddress);
static void write_PP(unsigned short PageAddress);
static void read_buffer(unsigned char* targetAddress, unsigned short ColumnAddress, unsigned short dataSize);
static void write_buffer(unsigned char* sourceAddress, unsigned short ColumnAddress, unsigned short dataSize);
static void dmaread_buffer(unsigned char* targetAddress, unsigned short ColumnAddress, unsigned short dataSize);
static void dmawrite_buffer(unsigned char* sourceAddress, unsigned short ColumnAddress, unsigned short dataSize);
static void write_enable();
static void wait_notbusy();
static unsigned char spi_transfer(unsigned int txdata);
static void restart_tim7(unsigned short time);

//----------------------------------------------------------------------------------------------------------------------

DSTATUS disk_initialize (BYTE pdrv)
{
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status == 0) return 0;//if disk is already initialized, do nothing
  
  //SPI1 configuration
  SPI1->CR2 = (1<<12)|(1<<10)|(1<<9)|(1<<8);//8 bit frames, software CS output, RXNE set after 8 bits received, interrupts disabled
  SPI1->CR1 = (1<<9)|(1<<8)|(1<<6)|(1<<3)|(1<<2);//bidirectional SPI, master mode, MSb first, 12Mhz clock, mode 0; enable SPI1
  NVIC_EnableIRQ(10);//enable DMA interrupt
  
  wait_notbusy();//make sure that a new command can be accepted
  CS_LOW;//pull CS pin low
  spi_transfer(0x1F);//Write Status Register command
  spi_transfer(0xA0);//Select Status Register 1
  spi_transfer(0x00);//disable write protection
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  CS_HIGH;//pull CS pin high
  
  wait_notbusy();//make sure that a new command can be accepted  
  CS_LOW;//pull CS pin low
  spi_transfer(0x1F);//Write Status Register command
  spi_transfer(0xB0);//Select Status Register 2
  spi_transfer(0x08);//disable ECC, stay in buffer access mode
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  CS_HIGH;//pull CS pin high
  
  readmap_EBI(); //initialize EBImap[] based on flash metadata
  readmap_PPO(0);//initialize PPOmap[] based on flash metadata
  
  //reinitialize disk state machine, set pdrv0_status to 0
  DiskInfo.LastErasedEB = 0;
  DiskInfo.WritePageFlag = 0;
  DiskInfo.pdrv0_status = 0;
  DiskInfo.BusyFlag = 0;
  
  return 0;
}

DSTATUS disk_status (BYTE pdrv)
{  
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available

  return DiskInfo.pdrv0_status;
}

//DMA has to be configured before disk_read() is called
DRESULT disk_read (
	BYTE  pdrv,	/* Physical drive number to identify the drive */
	BYTE* buff,	/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT  count	/* Number of sectors to read */
)
{
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  197120) return RES_PARERR;//last accessible LBA is 197119
  if(count == 0) return RES_OK;//read 0 sectors request immediately returns success  
  
  //keep going until specified number of sectors is read
  while(count)
    {
      dmaread_LB(buff, sector);//start the block transfer
      while(DiskInfo.BusyFlag);//wait until LB transfer is over
      
      buff = buff + 512;//512 more bytes have been written into data buffer
      sector++;//move on to the next LB
      count--;//one more LB was processed
    }
  
  return RES_OK;
}

//DMA has to be configured before disk_write() is called
DRESULT disk_write (
                    BYTE pdrv,         /* Physical drive number to identify the drive */
                    const BYTE* buff,  /* Data to be written */
                    DWORD sector,      /* Start sector in LBA */
                    UINT  count	       /* Number of sectors to write */
)
{
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(DiskInfo.pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  197120) return RES_PARERR;//last accessible LBA is 197119
  if(count == 0) return RES_OK;//write 0 sectors request immediately returns success
  
  prepare_LB(sector, count);//prepare specified LB's to be written
  
  //keep going until there still are LB's to process
  while(count)
    { 
      //start the block transfer; save the Data Buffer to flash if the end of current Logical Page or end of the entire write sequence is reached
      if( ((sector % 4) == 3) || (count == 1) ) dmawrite_LB((unsigned char*) buff, sector, 1);
      else                                      dmawrite_LB((unsigned char*) buff, sector, 0);
      while(DiskInfo.BusyFlag);//wait until LB transfer is over
      
      buff = buff + 512;//512 more bytes have been written into data buffer
      sector++;//move on to the next LB
      count--;//one more LB was processed
    }
  
  return RES_OK;
}

DRESULT disk_ioctl (
	BYTE  pdrv,  /* Physical drive number (0..) */
	BYTE  cmd,   /* Control code */
	void* buff   /* Buffer to send/receive control data */
)
{
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  
  switch(cmd)
    {      
    case CTRL_SYNC:
      //do nothing, because disk_write() does not use any cache
      return RES_OK;
      break;
            
    case GET_SECTOR_COUNT:
      //number of usable sectors = 197120
      *((DWORD*) buff) = 197120;
      return RES_OK;
      break;
            
    case GET_SECTOR_SIZE:
      //sector size = 512 bytes
      *((WORD*) buff) = 512;
      return RES_OK;
      break;
            
    case GET_BLOCK_SIZE:
      //erase block size = 256 sectors (128KiB)
      *((DWORD*) buff) = 256;
      return RES_OK;
      break;
      
    case CTRL_TRIM:
      //do nothing, because TRIM requests are not supported
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

//make sure that there is a LSI to EBI link for all LB's that will be rewritten, and that target EB's have enough space
void prepare_LB(unsigned int LBaddress, unsigned int LBcount)
{
  unsigned short EBI;//holds EB index of where current LS is mapped to
  unsigned char  freePPcount;//holds how many unused PP's are available  in the current EB
  unsigned char  needLPcount;//holds how many LP's need to be programmed in the current LS
  unsigned char  needLBcount;//holds how many LB's need to be programmed in the current LS
  
  if( (LBaddress + LBcount) > 197120 ) return;//if invalid LBA is specified, do nothing
  if(LBcount == 0) return;//if zero blocks need to be prepared, do nothing
  while(DiskInfo.BusyFlag);//wait until previous DMA transfers are over
  
  //keep going until all LP's are prepared
  while(LBcount)
    {
      //calculate how many LB's need to be programmed in current LS
      if((224 - (LBaddress % 224)) < LBcount) needLBcount = (224 - (LBaddress % 224));
      else needLBcount = LBcount;
      
      //calculate how many LP's need to be programmed inside current LS
      needLPcount = ((LBaddress + needLBcount - 1) / 4) - (LBaddress / 4) + 1;
      
      EBI = EBImap[LBaddress / 224] & 0x3FFF;//find in which EB current LP is stored right now
      
      if(EBI < 1024)//if current LS is already stored in some EB
      {
	if( EBI != DiskInfo.PPOmapValidEBI ) readmap_PPO(EBI);//make sure PPOmap[] is valid for current EBI
	
	//calculate how many PP's are free in current EB
	if(DiskInfo.PPOmapLastPPO > 63) freePPcount = 64;
	else freePPcount = (63 - DiskInfo.PPOmapLastPPO);
	
	//if more pages need to be written than there are free, relocate current LS to some new EB
	if(freePPcount < needLPcount)
	  { 
	    //move through LB's inside current LS one by one
	    while(needLBcount)
	      {
		//if page boundary is encountered, and the current page will be completely overwritten
		if( ((LBaddress % 4) == 0) && (needLBcount > 3) ) PPOmap[(LBaddress / 4) % 56] = 0xFF;//erase old LPO to PPO link
		
		//move on to the next LB
		LBaddress++;
		LBcount--;
		needLBcount--;
	      }

	    relocate_LS((LBaddress - 1) / 224);//relocate all the pages that are still valid
	  }
      }

      else//if current LS has no EB associated with it
	{
	  //assign a new free EB to the required LS
	  relocate_LS(LBaddress / 224);
	}
      
      //move on to the next LS
      LBaddress = LBaddress + needLBcount;
      LBcount = LBcount - needLBcount;
    }
  
  return;
}

//read a single LB based on it's LBA, store the data into specified buffer in RAM
void dmaread_LB(unsigned char* buff, unsigned int sector)
{
  unsigned short LSI;//holds LS index for specified LB
  unsigned short EBI;//holds EB index for specified LB
  unsigned short PPO;//holds PP offset of specified LB
  
  if(DiskInfo.pdrv0_status) return;//disk must be initialized
  if(sector >  197119) return;//last accessible LBA is 197119
  while(DiskInfo.BusyFlag);//wait until previous DMA transfers are over
  
  LSI = sector / 224;//find LSI value for specified LB
  EBI = EBImap[LSI] & 0x3FFF;//find which EB holds data of specified LS
  
  if( EBI < 1024 )//if some EB is found
    {
      if( EBI != DiskInfo.PPOmapValidEBI ) readmap_PPO(EBI);//make sure PPOmap[] is valid for specified EBI
      PPO = PPOmap[(sector / 4) % 56];//find in which PP specified LB is stored
    
      if(PPO < 64)//if some PP was found
	{
	  //if required page was not already in internal Data Buffer, load it in there
	  if(DiskInfo.BufferPageAddr != (EBI * 64 + PPO)) read_PP(EBI * 64 + PPO);
	  
	  dmaread_buffer(buff, (sector % 4) * 512, 512);//read specified data from internal Data Buffer
	}
      else//if specified LB is not stored in any PP
	{
	  //enable DMA channel 2, fill the specified buffer
	  DiskInfo.BusyFlag = 1;
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
      DiskInfo.BusyFlag = 1;
      DiskInfo.TransferByte = 0xFF;//prepare 0xFF value as DMA data source
      DMA1_Channel2->CNDTR = 512;//transfer 512 bytes
      DMA1_Channel2->CPAR = (unsigned int) &(DiskInfo.TransferByte);//do a memory-to-memory transfer from TransferByte
      DMA1_Channel2->CMAR = (unsigned int) buff;//fill specified buffer with all 0xFF values
      DMA1_Channel2->CCR = (1<<14)|(1<<7)|(1<<1)|(1<<0);//byte access, memory increment mode, enable TC interrupt      
    }
  
  return;
}

//take the data from specified buffer in RAM, write it to internal Data Buffer; save WritePageFlag value for dma_handler() to use
//you have to use prepare_LB() function on the necessary range of blocks before overwriting them with dmawrite_LB();
//you are not allowed to write to a new LP before previous one was saved to flash (eg. dmawrite_LB(buff1, 442, 0) before
//dmawrite_LB(buff2, 447, 1) is illegal, but dmawrite_LB(buff1, 442, 1) before dmawrite_LB(buff1, 447, 1) is OK.
void dmawrite_LB(unsigned char* buff, unsigned int sector, unsigned char WritePageFlag)
{  
  unsigned short LSI;//holds LS index for specified LB
  unsigned short EBI;//holds EB index for specified LB
  unsigned short PPO;//holds PB offset of specified LB
  
  if(DiskInfo.pdrv0_status) return;//disk must be initialized
  if(sector >  197119) return;//last accessible LBA is 197119
  while(DiskInfo.BusyFlag);//wait until previous DMA transfers are over
  
  LSI = sector / 224;//find which LS specified LB belongs to
  EBI = EBImap[LSI] & 0x3FFF;//find which EB specified LB is stored in
  
  if( EBI < 1024 )//if an existing LS to EB link was found (should always be true after prepare_LB() is called)
    {
      if( EBI != DiskInfo.PPOmapValidEBI ) readmap_PPO(EBI);//make sure PPOmap[] is valid for current EBI
      PPO = PPOmap[(sector / 4) % 56];//find which PP current LB is stored in
      
      if(PPO < 64)//if specified LP already has a PP mapping
	{
	  //load a previous Physical Page inside the Data Buffer (if not loaded already)
	  if( DiskInfo.BufferPageAddr != ((EBI * 64) + PPO) ) read_PP((EBI * 64) + PPO);
	}
      else//if specified LP does not yet have a PP mapping
	{
	  //load an empty Physical Page inside the Data Buffer (if not loaded already)
	  if( DiskInfo.BufferPageAddr != ((EBI * 64) + (DiskInfo.PPOmapLastPPO + 1) % 64) ) read_PP((EBI * 64) + (DiskInfo.PPOmapLastPPO + 1) % 64);
	}
      
      if(WritePageFlag)//if a page write operation is requested after this block is transferred to Data Buffer
	{
	  DiskInfo.PPOmapLastPPO = (DiskInfo.PPOmapLastPPO + 1) % 64;//internal Data Buffer will be written into next available PP
	  PPOmap[(sector / 4) % 56] = DiskInfo.PPOmapLastPPO;//remap current LP to a new PP in PPOmap[]
	  LSI |= 0x4000;//transform LSI into LSImarker with status bits set to 0b01 (valid)
	  write_buffer((unsigned char*) &LSI, 2048 + 2, 2);//write LSImarker into the internal Data Buffer
	  write_buffer((unsigned char*) PPOmap, 2048 + 4, 56);//write PPOmap into the internal Data Buffer
	  DiskInfo.WritePageFlag = WritePageFlag;//save the WritePageFlag for later use in dma_handler()
	}

      dmawrite_buffer(buff, (sector % 4) * 512, 512);//send LB data into the Data Buffer
    }
  else//if some error has happened and no previous LS to EB link was found
    {
      return;
    }    
  
  return;
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
  
  //if the end of current Logical Page or end of the entire write sequence is reached
  if( DiskInfo.WritePageFlag )
    {
      write_PP((DiskInfo.PPOmapValidEBI * 64) + DiskInfo.PPOmapLastPPO);//write Data Buffer into the next available PP
      DiskInfo.WritePageFlag = 0;//the page and it's metadata have been stored in flash
    }
  
  DiskInfo.BusyFlag = 0;
  return;
}

void garbage_collect()
{
  //if there was 100ms without a single read or write command received, start erasing old invalid EB's
  if( !(TIM7->CR1 & (1<<0)) )
    {
      NVIC_DisableIRQ(31);//disable usb interrupt, so makefree_EB() and MSD access do not collide
      //if there are no more invalid blocks to erase, prevent background erasing for the next 100ms
      if( makefree_EB((DiskInfo.LastErasedEB + 1) % 1024) > 1023 ) restart_tim7(100);
      NVIC_EnableIRQ(31);//enable usb interrupt
    }
  
  return;
}

//erase all the EB's which are marked as valid or invalid data
void mass_erase()
{
  unsigned short i;
  
  for(i=0; i<1024; i++)
    {
      if((EBImap[i] & 0xC000) <= 0x4000) erase_EB(i);
    }

  readmap_EBI(); //reinitialize EBImap[]
  readmap_PPO(0);//reinitialize PPOmap[]
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//move all the valid pages that belong to specified LS into a new EB; update EBImap[] with new correct values
static void relocate_LS(unsigned short LSindex)
{
  unsigned short oldEBI;//holds EB index of where LS is currently located
  unsigned short newEBI;//holds EB index of where LS will be moved into
  unsigned char  i;//used in a for() loop
  unsigned char  LPcount = 0;//holds how many LP's should be relocated to the new EB
  unsigned char  newPPO = 0;//holds PPO into which current LP will be relocated (inside new EB)
  
  if( LSindex > 879 ) return;//if invalid LSI is specified, do nothing     
  oldEBI = EBImap[LSindex] & 0x3FFF;//find which EB currently stores specified LS
  
  if(oldEBI < 1024)//if specified LS was already previously stored in some EB
    {      
      if( oldEBI != DiskInfo.PPOmapValidEBI ) readmap_PPO(oldEBI);//make sure that PPOmap[] is valid for current EBI
      if( DiskInfo.PPOmapLastPPO > 63) return;//return if there is nothing stored in target LS
      //make sure that internal Data Buffer is loaded with the last written page of current EB
      if( DiskInfo.BufferPageAddr != ((oldEBI * 64) + DiskInfo.PPOmapLastPPO) ) read_PP((oldEBI * 64) + DiskInfo.PPOmapLastPPO);
      
      EBImap[oldEBI]  &= 0x3FFF;//set old EB status bits to invalid in EBImap[]      
      EBImap[LSindex] |= 0x3FFF;//erase previous LSI to EBI link in EBImap[]
      write_buffer((unsigned char*) &LSindex, 2048 + 2, 2);//set invalid status in old LSImarker
      write_PP((oldEBI * 64) + DiskInfo.PPOmapLastPPO);//save invalid status in flash metadata
      
      newEBI = findfree_EB((oldEBI + 1) % 1024);//find a new and erased EB
      if(newEBI > 1023) newEBI = makefree_EB((oldEBI + 1) % 1024);//if no free EB is found, erase some old invalid block
      
      EBImap[newEBI] &= 0x7FFF;//set new EB status bits to 0b01 (valid data) in EBImap[]
      EBImap[LSindex] &= (newEBI | 0xC000);//link specified LS to EB with index = newEBI
      LSindex |= 0x4000;//transform LSindex into LSImarker with status bits set to 0b01 (valid)
      
      for(i=0; i<56; i++) {if(PPOmap[i] < 64) LPcount++;}//calculate total number of LP's to relocate
      
      //copy all the valid LP's from old EB to new EB, rewrite PPOmap[] with new mapping entries
      for(i=0; i<56; i++)
	{
	  if(PPOmap[i] < 64)//if current LP was stored in some PP
	    { 
	      read_PP((oldEBI * 64) + PPOmap[i]);//copy LP data from old EB into internal Data Buffer	      
	      write_buffer((unsigned char*) &LSindex, 2048 + 2, 2);//write new LSImarker to internal Data Buffer
	      PPOmap[i] = newPPO;//rewrite one PPOmap[] entry with new data
	      	      
	      if(newPPO == (LPcount - 1))//if the last LP is being relocated
		{
		  //save the new updated PPOmap[] in page metadata
		  write_buffer((unsigned char*) PPOmap, 2048 + 4, 56);
		  
		  //PPOmap[] is now valid for the new EB
		  DiskInfo.PPOmapValidEBI = newEBI;
		  DiskInfo.PPOmapLastPPO  = newPPO;
		}
	      
	      write_PP((newEBI * 64) + newPPO);//store data from internal Data Buffer into a PP inside new EB	      
	      newPPO++;//store next LP in a new PP
	    }
	}     
    }
    
  else//if no previous EB was found for specified LS
    {
      newEBI = findfree_EB(0);//find a new and erased EB
      if(newEBI > 1023) newEBI = makefree_EB(0);//if no free EB is found, erase some old invalid EB
      
      EBImap[newEBI] &= 0x7FFF;//set new EB status bits to valid in EBImap[]
      EBImap[LSindex] &= (newEBI | 0xC000);//link specified LS to EB with index = newEBI
    }
  
  return;
}

//read some flash metadata in the first page of every EB to fill the EBImap[] with correct values
static void readmap_EBI()
{
  unsigned short i;//used in a for() loop, holds index of currently processed EB
  unsigned short PageAddr;//holds flash Physical Page Address to read metadata from
  unsigned short Markers[2];//Markers[0] is a bad block marker, Markers[1] is LSImarker
  
  for(i=0; i<1024; i++) EBImap[i] = 0x3FFF;//pre-fill all of EBImap[] with 0x3FFF values
  
  //search through every single EB
  for(i=0; i<1024; i++)
    {
      PageAddr = 64 * i;//convert EBI into Physical Page Address, select first page inside an EB
      read_PP(PageAddr);//fill internal Data Buffer with target page data
      read_buffer((unsigned char*) Markers, 2048, 4);//read markers from flash to check requested EB status
      
      if(Markers[0] < 0xFFFF) EBImap[i] |= 0x8000;//if a bad block marker is detected, set EB status to 0b10 (bad block)
      else                    EBImap[i] |= (Markers[1] & 0xC000);//if block is not bad, set EB status based on LSImarker
      
      if((Markers[1] & 0x3FFF) < 880)//if some LS was stored in current EB
	{
	  if( (EBImap[Markers[1] & 0x3FFF] & 0x3FFF) < 1024 )//if specified LSI is already mapped to some EBI
	    {	      
	      if((readmap_PPO(i) & 0xC000) == 0x4000)//if the latest status for current EB is valid
		{
		  Markers[0] = EBImap[Markers[1] & 0x3FFF] & 0x3FFF;//temporarily use Markers[0] to store old EBI
		  EBImap[Markers[0]] &= 0x3FFF;//set status of EB from the old entry to 0b00 (invalid data)
		  EBImap[Markers[1] & 0x3FFF] |= 0x3FFF;//erase old mapping entry
		  EBImap[Markers[1] & 0x3FFF] &= (i | 0xC000);//save new mapping entry
		}
	      else//if latest status of current EB is set to invalid
		{
		  EBImap[i] &= 0x3FFF;//set current EB status to invalid, keep the old mapping entry
		}
	    }
	  else//if specified LSI does not yet have any EBI associated with it
	    {
	      EBImap[Markers[1] & 0x3FFF] &= (i | 0xC000);//make new link in the EBImap[] from specified LSI to current EBI
	    }
	}
    }
  
  return;
}

//search through the specified EB, find the last PP that was written to, then get the latest PPOmap[] from it and return the latest LSImarker value
static unsigned short readmap_PPO(unsigned short EBI)
{
  unsigned short LSImarker;//holds EB status and LSI value retrieved from flash metadata
  unsigned short PageAddr;//holds flash Physical Page Address to read metadata from
  unsigned char  i;//used in a for() loop
  unsigned char  SplitSize = 32;//last written page address is in the range from (PageAddr - SplitSize) to (PageAddr + SplitSize - 1)
  
  if(EBI > 1023) return 0x3FFF;//if invalid EBI was specified, do nothing
  PageAddr = (64 * EBI) + SplitSize;//start search at PPO = 32
  
  //find address of the last page that was written to and store it in PageAddr
  for(i=0; i<6; i++)
    {
      read_PP(PageAddr);//fill internal Data Buffer with target page data      
      read_buffer((unsigned char*) &LSImarker, 2048 + 2, 2);//read LSImarker from flash to check if requested page was used already
      
      if((LSImarker & 0x3FFF) < 880) PageAddr = PageAddr + ((SplitSize + 0) / 2);//if a page at PageAddr was already written to, search pages at higher addresses
      else                           PageAddr = PageAddr - ((SplitSize + 1) / 2);//if a page at PageAddr was not written to yet, search pages at lower  addresses
      SplitSize = SplitSize / 2;//halve SplitSize for next iteration
    }
  
  //make sure internal W25N01GVZEIG Data Buffer has the last written page inside
  if(DiskInfo.BufferPageAddr != PageAddr)
    {
      read_PP(PageAddr);//store the last page into the Data Buffer
      read_buffer((unsigned char*) &LSImarker, 2048 + 2, 2);//get the latest LSImarker
    }
  
  read_buffer(PPOmap, 2048 + 4, 56);//read the latest PPOmap[] from flash metadata
  DiskInfo.PPOmapValidEBI = EBI;//save EBI for which current PPOmap[] is valid    
  if((LSImarker & 0xC000) == 0xC000) DiskInfo.PPOmapLastPPO = 0xFF;//if LSImarker status is erased, there is no last used PPO
  else                               DiskInfo.PPOmapLastPPO = PageAddr % 64;//if status is not erased, remember last used PPO
  
  return LSImarker;
}

//find and erase the first invalid EB, with search starting from specified EB index; then return newly erased EB index
static unsigned short makefree_EB(unsigned short startEBI)
{
  unsigned short i;//used in a for() loop 
  
  if(startEBI > 1023) return 0x3FFF;//if invalid EBI is specified, do nothing
  
  //if not a single free EB was found, erase some old invalid EB
  for(i=0; i<1024; i++)
    {
      //if some invalid EB is found, stop searching and return it's index
      if((EBImap[startEBI] & 0xC000) == 0x0000 )
      {
	erase_EB(startEBI);//erase the EB; this also writes new correct flash metadata
	EBImap[startEBI] |= 0xC000;//set newly found EB status to erased in EBImap[]	
	return startEBI;
      }

      startEBI = (startEBI + 1) % 1024;//move on to the next EB
    }

  return 0x3FFF;//if no invalid block was found to erase, return 0x3FFF
}

//search through EBImap[] and return index of a first free EB, with search starting from specified EB index
static unsigned short findfree_EB(unsigned short startEBI)
{
  unsigned short i;//used in a for() loop 
  
  if(startEBI > 1023) return 0x3FFF;//if invalid EBI is specified, do nothing
  
  //keep searching for new free EB
  for(i=0; i<1024; i++)
    {
      //if free EB is found, stop searching and return it's index
      if((EBImap[startEBI] & 0xC000) == 0xC000 ) return startEBI;
            
      startEBI = (startEBI + 1) % 1024;//move on to the next EB
    }
  
  return 0x3FFF;//if free EB was not found, return 0x3FFF
}

//erase specified EB based on it's index
static void erase_EB(unsigned short EBindex)
{
  unsigned short PageAddress;//holds Physical Page Address of an EB to erase
  
  if(EBindex  > 1023) return;//if invalid EBI is specified, do nothing
  PageAddress = EBindex * 64;//convert EBI into Physical Page Address

  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0xD8);//128 KiB Block Erase command
  spi_transfer(0x00);//send 8 dummy clocks
  spi_transfer( PageAddress >> 8 );//send Physical Page Address
  spi_transfer( PageAddress >> 0 );//send Physical Page Address
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high

  DiskInfo.LastErasedEB = EBindex;//remember index of the last EB that was erased
  
  return;
}

//read a page into internal Data Buffer in W25N01GVZEIG from a specified page address
static void read_PP(unsigned short PageAddress)
{
  if(PageAddress > 65535) return;//if invalid page address is specified, do nothing
  
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x13);//Page Data Read command
  spi_transfer(0x00);//send 8 dummy clocks
  spi_transfer( PageAddress >> 8 );//send target page address
  spi_transfer( PageAddress >> 0 );//send target page address
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  DiskInfo.BufferPageAddr = PageAddress;//remember which page is now loaded in internal Data Buffer
  GPIOB->BSRR = (1<<7);//turn on the status LED
  restart_tim7(100);//prevent background erasing for the next 100ms
  
  return;
}

//save current contents of internal Data Buffer in W25N01GVZEIG to flash at specified page address
static void write_PP(unsigned short PageAddress)
{
  if(PageAddress > 65535) return;//if invalid page address is specified, do nothing
  
  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x10);//Program Execute command
  spi_transfer(0x00);//send 8 dummy clocks
  spi_transfer( PageAddress >> 8 );//send target page address
  spi_transfer( PageAddress >> 0 );//send target page address
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  DiskInfo.BufferPageAddr = PageAddress;//remember which page is now loaded in internal Data Buffer
  GPIOB->BSRR = (1<<7);//turn on the status LED
  restart_tim7(100);//prevent background erasing for the next 100ms
  
  return;
}

//read data from internal Data Buffer of W25N01GVZEIG at ColumnAddress and of size dataSize; store it into a RAM buffer at targetAddress
static void read_buffer(unsigned char* targetAddress, unsigned short ColumnAddress, unsigned short dataSize)
{
  unsigned short k;//used in a for() loop
  
  if((ColumnAddress + dataSize) > 2112) return;//if access out of range is requested, do nothing
  if(dataSize == 0) return;//if read of 0 bytes is requested, do nothing
  
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer( ColumnAddress >> 8 );//send target column address
  spi_transfer( ColumnAddress >> 0 );//send target column address
  spi_transfer(0x00);//send 8 dummy clocks
  for(k=0; k<dataSize; k++) targetAddress[k] = spi_transfer(0x00);
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//read data from a RAM buffer at sourceAddress of size dataSize; store it in internal Data Buffer of W25N01GVZEIG at ColumnAddress
static void write_buffer(unsigned char* sourceAddress, unsigned short ColumnAddress, unsigned short dataSize)
{
  unsigned short k;//used in a for() loop
  
  if((ColumnAddress + dataSize) > 2112) return;//if access out of range is requested, do nothing
  if(dataSize == 0) return;//if write of 0 bytes is requested, do nothing
  
  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x84);//Random Load Program Data command
  spi_transfer( ColumnAddress >> 8 );//send target column address
  spi_transfer( ColumnAddress >> 0 );//send target column address
  for(k=0; k<dataSize; k++) spi_transfer( sourceAddress[k] );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//different implementation of read_buffer() which uses DMA for transfers
static void dmaread_buffer(unsigned char* targetAddress, unsigned short ColumnAddress, unsigned short dataSize)
{
  if((ColumnAddress + dataSize) > 2112) return;//if access out of range is requested, do nothing
  if(dataSize == 0) return;//if read of 0 bytes is requested, do nothing
  
  wait_notbusy();//make sure that a new command can be accepted
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer( ColumnAddress >> 8 );//send target column address
  spi_transfer( ColumnAddress >> 0 );//send target column address
  spi_transfer(0x00);//send 8 dummy clocks
  
  //enable DMA channel 2, get data from the medium
  DMA1_Channel2->CNDTR = dataSize;//transfer dataSize number of bytes
  DMA1_Channel2->CPAR = (unsigned int) &(SPI1->DR);//receive data from SPI1 RXFIFO 
  DMA1_Channel2->CMAR = (unsigned int) targetAddress;//fill specified buffer with data
  DMA1_Channel2->CCR = (1<<7)|(1<<1)|(1<<0);//byte access, memory increment mode, enable TC interrupt
  
  //enable DMA channel 3, send all 0x00 values to the memory chip
  DiskInfo.BusyFlag = 1;
  DiskInfo.TransferByte = 0x00;//prepare 0x00 value as DMA data source
  DMA1_Channel3->CNDTR = dataSize;//transfer dataSize number of bytes
  DMA1_Channel3->CPAR = (unsigned int) &(SPI1->DR);//send data to SPI1 TXFIFO
  DMA1_Channel3->CMAR = (unsigned int) &(DiskInfo.TransferByte);//send all 0x00 values
  DMA1_Channel3->CCR = (1<<4)|(1<<0);//byte access, transfer from memory to peripheral
  
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  SPI1->CR1 &= ~(1<<6);//disable SPI1
  SPI1->CR2 |= (1<<1)|(1<<0);//enable DMA requests for SPI1
  SPI1->CR1 |= (1<<6);//enable SPI1 with new configuration  
  
  return;
}

//different implementation of write_buffer() which uses DMA for transfers
static void dmawrite_buffer(unsigned char* sourceAddress, unsigned short ColumnAddress, unsigned short dataSize)
{
  if((ColumnAddress + dataSize) > 2112) return;//if access out of range is requested, do nothing
  if(dataSize == 0) return;//if write of 0 bytes is requested, do nothing
  
  //make sure that a new command can be accepted
  wait_notbusy();
  write_enable();
  
  CS_LOW;//pull CS pin low
  spi_transfer(0x84);//Random Load Program Data command
  spi_transfer( ColumnAddress >> 8 );//send target column address
  spi_transfer( ColumnAddress >> 0 );//send target column address
  
  //enable DMA channel 2, get data from the medium
  DMA1_Channel2->CNDTR = dataSize;//transfer dataSize number of bytes
  DMA1_Channel2->CPAR = (unsigned int) &(SPI1->DR);//receive data from RXFIFO
  DMA1_Channel2->CMAR = (unsigned int) &(DiskInfo.TransferByte);//ignore any data sent back
  DMA1_Channel2->CCR = (1<<1)|(1<<0);//byte access, enable TC interrupt
  
  //enable DMA channel 3, send data to the memory chip
  DiskInfo.BusyFlag = 1;
  DMA1_Channel3->CNDTR = dataSize;//transfer dataSize number of bytes
  DMA1_Channel3->CPAR = (unsigned int) &(SPI1->DR);//send data to TXFIFO
  DMA1_Channel3->CMAR = (unsigned int) sourceAddress;//send the data from specified buffer
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
  spi_transfer(0x05);//Read Status Register command
  spi_transfer(0xC0);//Select Status Register 3
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  while( spi_transfer(0x00) & (1<<0) );//keep sending clock until flash memory busy flag is reset back to 0
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  CS_HIGH;//pull CS pin high
  
  return;
}

//only the least significant byte will be sent; CS signal has to be asserted already
static unsigned char spi_transfer(unsigned int txdata)
{
  while( !(SPI1->SR & (1<<1)) );//wait until TX FIFO has enough space
  *( (unsigned char*) &(SPI1->DR) ) = txdata;//send txdata to the FIFO
  while( !(SPI1->SR & (1<<0)) );//wait until new packet is received
  
  return (unsigned char) SPI1->DR;
}

//start the TIM7 timer, run for specified amount of milliseconds (max argument = 1365)
static void restart_tim7(unsigned short time)
{
  //TIM7 configuration: ARR is buffered, one pulse mode, only overflow generates interrupt, start upcounting
  NVIC_DisableIRQ(18);//disable TIM7 interrupt
  TIM7->CR1 = (1<<7)|(1<<3)|(1<<2);//disable TIM7 (in case it was running)
  TIM7->DIER = (1<<0);//enable TIM7 overflow interrupt
  TIM7->ARR = time * 48 - 1;//run for specified amount of milliseconds
  TIM7->PSC = 999;//TIM7 prescaler = 1000
  TIM7->EGR = (1<<0);//generate update event
  TIM7->SR = 0;//clear overflow flag
  TIM7->CR1 = (1<<7)|(1<<3)|(1<<2)|(1<<0);//enable TIM7
  NVIC_EnableIRQ(18);//enable TIM7 interrupt
  
  return;
}
