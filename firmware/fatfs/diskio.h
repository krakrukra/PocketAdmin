#ifndef DISKIO
#define DISKIO

/* These types MUST be 16-bit or 32-bit */
typedef int		INT;
typedef unsigned int	UINT;

/* This type MUST be 8-bit */
typedef unsigned char	BYTE;

/* These types MUST be 16-bit */
typedef short		SHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types MUST be 32-bit */
typedef long		LONG;
typedef unsigned long	DWORD;

/* This type MUST be 64-bit (Remove this for ANSI C (C89) compatibility) */
typedef unsigned long long QWORD;


/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum
{
  RES_OK = 0,	/* 0: Successful */
  RES_ERROR,	/* 1: R/W Error */
  RES_WRPRT,	/* 2: Write Protected */
  RES_NOTRDY,	/* 3: Not Ready */
  RES_PARERR	/* 4: Invalid Parameter */
} DRESULT;

//this structure contains all necessary information for handling disk access operations
typedef struct
{  
  unsigned short LastErasedEB;//index of the last EB that was erased
  unsigned short BufferPageAddr;//holds physical page address of a page that is currently loaded in internal Data Buffer of W25N01GVZEIG
  unsigned short PPOmapValidEBI;//holds index of an Erase Block for which PPOmap[] is currently valid
  unsigned char  PPOmapLastPPO;//holds the biggest valid PPO in PPOmap[] (0 to 63); set to 0xFF if no LPO to PPO links are found at all
  unsigned char  TransferByte;//used for some DMA transfers as a source/destination (eg. to fill some buffer with 0xFF values)
  DSTATUS pdrv0_status;//status of physical drive 0
  volatile unsigned char WritePageFlag;//0 means disk_dmawrite() will only write to internal DataBuffer, nonzero value means write block and save the DataBuffer to flash
  volatile unsigned char BusyFlag;//0 means no DMA transfers between MCU and W25N01GVZEIG are ongoing, 1 means some transfer is ongoing
} DiskInfo_TypeDef;

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT	0x01  /* Drive not initialized */
#define STA_NODISK	0x02  /* No medium in the drive */
#define STA_PROTECT	0x04  /* Write protected */

/* Generic command (Used by FatFs) */
#define CTRL_SYNC          0  /* Complete pending write process (needed at FF_FS_READONLY == 0) */
#define GET_SECTOR_COUNT   1  /* Get media size (needed at FF_USE_MKFS == 1) */
#define GET_SECTOR_SIZE	   2  /* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
#define GET_BLOCK_SIZE	   3  /* Get erase block size (needed at FF_USE_MKFS == 1) */
#define CTRL_TRIM	   4  /* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */

/* Generic command (Not used by FatFs) */
#define CTRL_POWER	5  /* Get/Set power status */
#define CTRL_LOCK	6  /* Lock/Unlock media removal */
#define CTRL_EJECT	7  /* Eject media */
#define CTRL_FORMAT	8  /* Create physical format on the media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE	10  /* Get card type */
#define MMC_GET_CSD	11  /* Get CSD */
#define MMC_GET_CID	12  /* Get CID */
#define MMC_GET_OCR	13  /* Get OCR */
#define MMC_GET_SDSTAT	14  /* Get SD status */
#define ISDIO_READ	55  /* Read data form SD iSDIO register */
#define ISDIO_WRITE	56  /* Write data to SD iSDIO register */
#define ISDIO_MRITE	57  /* Masked write data to SD iSDIO register */

/* ATA/CF specific ioctl command */
#define ATA_GET_REV	20  /* Get F/W revision */
#define ATA_GET_MODEL	21  /* Get model name */
#define ATA_GET_SN	22  /* Get serial number */

//chan fatfs interface functions
DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);
DWORD   get_fattime (void);

//general disk access functions
void prepare_LB(unsigned int LBaddress, unsigned int LBcount);
void dmaread_LB (unsigned char* buff, unsigned int sector);
void dmawrite_LB(unsigned char* buff, unsigned int sector, unsigned char writePageFlag);
void dma_handler() __attribute__((interrupt));
void garbage_collect();
void mass_erase();

#endif //DISKIO
