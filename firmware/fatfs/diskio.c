#include "../cmsis/stm32f0xx.h"
#include "diskio.h"

unsigned char ReadBuffer[512] __attribute__(( aligned(2) ));
unsigned char WriteBuffer[4096] __attribute__(( aligned(2) ));
DSTATUS pdrv0_status = STA_NOINIT;//not yet initialized

static unsigned char spi_transfer(unsigned int txdata);
static void write_enable();
static void wait_notbusy();

//----------------------------------------------------------------------------------------------------------------------

DSTATUS disk_initialize (BYTE pdrv)
{
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available
    
  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0xB7);//Enter 4-Byte Address Mode command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  pdrv0_status = 0;//physical drive 0 is now initialized

  return 0;
}

DSTATUS disk_status (BYTE pdrv)
{  
  if(pdrv != 0) return STA_NOINIT;//only physical drive 0 is available

  return pdrv0_status;
}

DRESULT disk_read (
	BYTE pdrv,	/* Physical drive number to identify the drive */
	BYTE* buff,	/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count	/* Number of sectors to read */
)
{
  unsigned int i;//used in a for() loop

  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  65536) return RES_PARERR;//last accessible LBA is 65535
  if(count == 0) return RES_OK;//read 0 sectors request immediately returns success

  sector = sector * 512;//variable sector now contains byte address of requested sector
  count = count * 512;//variable count now contains number of bytes to read

  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0x03);//Read Data command (4 byte address)
  spi_transfer(sector >> 24);
  spi_transfer(sector >> 16);
  spi_transfer(sector >> 8);
  spi_transfer(sector >> 0);  
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  while(SPI1->SR & (1<<0)) pdrv = (unsigned char) SPI1->DR;//empty the RX FIFO
  
  //save received data to specified buffer
  for(i=0; i<count; i++) *(buff + i) = spi_transfer(0x00);
  
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  return RES_OK;
}

DRESULT disk_write (
                    BYTE pdrv,         /* Physical drive number to identify the drive */
                    const BYTE* buff,  /* Data to be written */
                    DWORD sector,      /* Start sector in LBA */
                    UINT count	       /* Number of sectors to write */
)
{
  unsigned int i;//used in for() loop
  unsigned int offset;//LBA offset (from 0 to 7) of start sector in 4096 byte block
  
  if(pdrv != 0) return RES_PARERR;//only physical drive 0 is available
  if(pdrv0_status) return RES_NOTRDY;//disk must be initialized
  if( (sector + count) >  65536) return RES_PARERR;//last accessible LBA is 65535
  if(count == 0) return RES_OK;//write 0 sectors request immediately returns success
  
  offset = sector % 8;
  //if necessary, fill the beginning of write buffer with data already saved on the medium
  if(offset) disk_read(0, (unsigned char*) &WriteBuffer, sector - offset, offset);
  
  sector = sector * 512;//variable sector now contains byte address of requested LBA
  count = count * 512;//variable count now contains number of bytes to write
  offset = offset * 512;//variable offset now contains byte offset of a start sector in 4096 byte block

  while(count)//keep overwriting 4096 byte blocks until all requested data is saved on the medium
    {
      //fill the requested portion of WriteBuffer with new data
      for(i=0; i < count; i++)
	{
	  if( (offset + i) == 4096 ) break;//exit if WriteBuffer is full
	  
	  //overwrite previous data from the medium with requested data
	  *( (unsigned char*) ((unsigned int) &WriteBuffer + offset + i) ) = *buff;
	  buff++;
	}

      //if necessary, fill the end of write buffer with data already saved on the medium
      if((offset + i) < 4096) disk_read(0, (unsigned char*) ((unsigned int) &WriteBuffer + (sector + i) % 4096), (sector + i) / 512, (4096 - (sector + i) % 4096) / 512);
      
      block_erase_4k(sector - offset);
      write_pages(sector - offset);
      
      sector = sector + i;//if (i < count) is still true, sector now points to the start of next 4096 byte block
      offset = 0;//next 4096 block will not have offset (it will always be written with new data from the beginning)
      count = count - i;//i number of bytes is already written, count is now equal to the amount of bytes left
    }

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
      *((DWORD*) buff) = 65536;
      return RES_OK;
      break;

    case GET_SECTOR_SIZE:
      *((WORD*) buff) = 512;
      return RES_OK;
      break;

    case GET_BLOCK_SIZE:
      *((DWORD*) buff) = 8;
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

//saves data from WriteBuffer to medium, starting from byte address startAddr. medium must be pre-erased from startAddr to startAddr + 4095
void write_pages(unsigned int startAddr)
{
  unsigned int j;//used in for() loop
  unsigned int k;//used in for() loop
  
  //write entire 4096 byte block to the medium
  for(j=0; j<16; j++)
    { 
      write_enable();
      
      //Page Program command (4 byte address)
      GPIOA->BSRR = (1<<31);//pull CS pin low
      spi_transfer(0x02);
      spi_transfer( (startAddr + j * 256) >> 24 );
      spi_transfer( (startAddr + j * 256) >> 16 );
      spi_transfer( (startAddr + j * 256) >> 8 );
      spi_transfer( (startAddr + j * 256) >> 0 );
      for(k=0; k<256; k++) spi_transfer( WriteBuffer[j * 256 + k] );
      while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
      GPIOA->BSRR = (1<<15);//pull CS pin high
      
      wait_notbusy();
    }
  
  return;
}

//erases 4 KiB block located at startAddr
void block_erase_4k(unsigned int startAddr)
{
  if( startAddr % 4096 ) return;//if startAddr does not point to the very first byte of a block, do nothing
  
  write_enable();
  
  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0x20);//4 KiB Block Erase command (4 byte address)
  spi_transfer( startAddr >> 24 );
  spi_transfer( startAddr >> 16 );
  spi_transfer( startAddr >> 8 );
  spi_transfer( startAddr >> 0 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  wait_notbusy();

  return;
}

//erases 32 KiB block located at startAddr
void block_erase_32k(unsigned int startAddr)
{
  if( startAddr % (4096 * 8) ) return;//if startAddr does not point to the very first byte of a block, do nothing
  
  write_enable();
   
  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0x52);//32 KiB Block Erase command (4 byte address)
  spi_transfer( startAddr >> 24 );
  spi_transfer( startAddr >> 16 );
  spi_transfer( startAddr >> 8 );
  spi_transfer( startAddr >> 0 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  wait_notbusy();

  return;
}

//erases 64 KiB block located at startAddr
void block_erase_64k(unsigned int startAddr)
{
  if( startAddr % (4096 * 16) ) return;//if startAddr does not point to the very first byte of a block, do nothing
  
  write_enable();
  
  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0xD8);//64 KiB Block Erase command (4 byte address)
  spi_transfer( startAddr >> 24 );
  spi_transfer( startAddr >> 16 );
  spi_transfer( startAddr >> 8 );
  spi_transfer( startAddr >> 0 );
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  wait_notbusy();
  
  return;
}

void device_reset()
{
  GPIOA->BSRR = (1<<31);//pull CS pin low  
  spi_transfer(0x66);//Enable Reset command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  GPIOA->BSRR = (1<<15);//pull CS pin high

  GPIOA->BSRR = (1<<31);//pull CS pin low  
  spi_transfer(0x99);//Reset Device command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

//only the least significant byte will be sent
static unsigned char spi_transfer(unsigned int txdata)
{
  while( !(SPI1->SR & (1<<1)) );//wait until TX FIFO has enough space
  *( (unsigned char*) &(SPI1->DR) ) = txdata;//send txdata to the FIFO
  while( !(SPI1->SR & (1<<0)) );//wait until new packet is received
  
  return (unsigned char) SPI1->DR;
}

static void write_enable()
{
  GPIOA->BSRR = (1<<31);//pull CS pin low  
  spi_transfer(0x06);//Write Enable command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  return;
}

static void wait_notbusy()
{    
  GPIOA->BSRR = (1<<31);//pull CS pin low
  spi_transfer(0x05);//Read Status Register 1 command
  while(SPI1->SR & (1<<7));//wait until SPI1 is no longer busy
  while( spi_transfer(0x00) & (1<<0) );//keep sending clock until flash memory busy flag is reset back to 0
  while(SPI1->SR & (1<<7));//wait until SPI1 is not busy
  GPIOA->BSRR = (1<<15);//pull CS pin high
  
  return;
}
