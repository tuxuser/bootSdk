#ifndef _CFE_H
#define _CFE_H

typedef unsigned int uint32_t;
typedef unsigned long uintptr_t;

typedef unsigned int UINT32;

#define NULL ((void*)0)
#define FALSE (0)
#define TRUE (1)

#define PHYS_FLASH_BASE         0x18000000      /* Flash Memory     */

#define IMAGE_BASE      (0xA0000000 | PHYS_FLASH_BASE)
#define FLASH_BASE		(0x04000000)
#define BOOT_OFFSET     (FLASH_BASE - IMAGE_BASE)

#define IMAGE_OFFSET    (0x00000000)

#define CRC32_INIT_VALUE 0xffffffff /* Initial CRC32 checksum value */
#define CRC_LEN 4

#define BLPARMS_MAGIC               0x424c504d

#define BOOTED_IMAGE_ID_NAME        "boot_image"

#define BOOTED_NEW_IMAGE            1
#define BOOTED_OLD_IMAGE            2
#define BOOTED_ONLY_IMAGE           3
#define BOOTED_PART1_IMAGE          4
#define BOOTED_PART2_IMAGE          5

#define BOOT_SET_NEW_IMAGE          '0'
#define BOOT_SET_OLD_IMAGE          '1'
#define BOOT_SET_NEW_IMAGE_ONCE     '2'
#define BOOT_GET_IMAGE_VERSION      '3'
#define BOOT_GET_BOOTED_IMAGE_ID    '4'
#define BOOT_SET_PART1_IMAGE        '5'
#define BOOT_SET_PART2_IMAGE        '6'
#define BOOT_SET_PART1_IMAGE_ONCE   '7'
#define BOOT_SET_PART2_IMAGE_ONCE   '8'
#define BOOT_GET_BOOT_IMAGE_STATE   '9'

#define FLASH_PARTDEFAULT_REBOOT    0x00000000
#define FLASH_PARTDEFAULT_NO_REBOOT 0x00000001
#define FLASH_PART1_REBOOT          0x00010000
#define FLASH_PART1_NO_REBOOT       0x00010001
#define FLASH_PART2_REBOOT          0x00020000
#define FLASH_PART2_NO_REBOOT       0x00020001

#define FLASH_IS_NO_REBOOT(X)       ((X) & 0x0000ffff)
#define FLASH_GET_PARTITION(X)      ((unsigned long) (X) >> 16)

extern void main();

#define TRACE() \
    do { \
        register unsigned long r11 asm("r11"); \
        register unsigned long lr asm("lr"); \
        register unsigned long sp asm("sp"); \
        ((void(*)(char *, ...))0xF37D00)("%s: %x,%x,%x\n", __func__, r11, lr, sp); \
    } while(0)


// Helper
void enableIrqFiq();
void enableMMU();

void cfe_init_funcs();

// Own UI functions
int ui_tftpget(void *argv);
int ui_goex(void *argv);
int ui_autorun(void *argv);
int ui_bootc(void *argv);
int bootCompressedImage(uint32_t *puiCmpImage, int retry);

#define LA_DEFAULT_FLAGS    0x0
/* Original CFE definitions */
#define LOADFLG_NOISY		0x0001	/* print out noisy info */
#define LOADFLG_EXECUTE		0x0002	/* execute loaded program */
#define LOADFLG_SPECADDR	0x0004	/* Use a specific size & addr */
#define LOADFLG_NOBB		0x0008	/* don't look for a boot block */
#define LOADFLG_NOCLOSE		0x0010	/* don't close network */
#define LOADFLG_COMPRESSED	0x0020	/* file is compressed */
#define LOADFLG_BATCH		0x0040	/* batch file */
#define LOADFLG_64BITIMG	0x0080	/* 64 bit or 32 bit image */

typedef struct cfe_loadargs_s {
    char *la_filename;			/* name of file on I/O device */
    char *la_filesys;			/* file system name */
    char *la_device;			/* device name (ide0, etc.) */
    char *la_options;			/* args to pass to loaded prog */
    char *la_loader;			/* binary file loader to use */
    unsigned int la_flags;		/* various flags */
    long la_address;			/* used by SPECADDR only */
    unsigned long la_maxsize;		/* used by SPECADDR only */
    long la_entrypt;			/* returned entry point */
} cfe_loadargs_t;


typedef struct cfe_loader_s {
    char *name;				/* name of loader */
    int (*loader)(cfe_loadargs_t *);	/* access function */
    int flags;				/* flags */
} cfe_loader_t;

#define BCM_SIG_1   "Broadcom Corporation"
#define BCM_SIG_2   "ver. 2.0"          // was "firmware version 2.0" now it is split 6 char out for chip id.

#define BCM_TAG_VER         "6"

// file tag (head) structure all is in clear text except validationTokens (crc, md5, sha1, etc). Total: 128 unsigned chars
#define TAG_LEN         256
#define TAG_VER_LEN     4
#define SIG_LEN         20
#define SIG_LEN_2       14   // Original second SIG = 20 is now devided into 14 for SIG_LEN_2 and 6 for CHIP_ID
#define CHIP_ID_LEN		6	
#define IMAGE_LEN       10
#define ADDRESS_LEN     12
#define FLAG_LEN        2
#define TOKEN_LEN       20
#define BOARD_ID_LEN    16
#define IMAGE_VER_LEN   32
#define RESERVED_LEN    (TAG_LEN - TAG_VER_LEN - SIG_LEN - SIG_LEN_2 - CHIP_ID_LEN - BOARD_ID_LEN - \
                        (4*IMAGE_LEN) - (3*ADDRESS_LEN) - (3*FLAG_LEN) - (2*TOKEN_LEN) - IMAGE_VER_LEN)

// TAG for downloadable image (kernel plus file system)
typedef struct _FILE_TAG
{
    char tagVersion[TAG_VER_LEN];       // tag version.  Will be 2 here.
    char signiture_1[SIG_LEN];          // text line for company info
    char signiture_2[SIG_LEN_2];        // additional info (can be version number)
    char chipId[CHIP_ID_LEN];			 // chip id 
    char boardId[BOARD_ID_LEN];         // board id
    char bigEndian[FLAG_LEN];           // if = 1 - big, = 0 - little endia of the host
    char totalImageLen[IMAGE_LEN];      // the sum of all the following length
    char cfeAddress[ADDRESS_LEN];       // if non zero, cfe starting address
    char cfeLen[IMAGE_LEN];             // if non zero, cfe size in clear ASCII text.
    char rootfsAddress[ADDRESS_LEN];    // if non zero, filesystem starting address
    char rootfsLen[IMAGE_LEN];          // if non zero, filesystem size in clear ASCII text.
    char kernelAddress[ADDRESS_LEN];    // if non zero, kernel starting address
    char kernelLen[IMAGE_LEN];          // if non zero, kernel size in clear ASCII text.
    char imageSequence[FLAG_LEN * 2];   // incrments everytime an image is flashed
    char imageVersion[IMAGE_VER_LEN];   // image version
    char reserved[RESERVED_LEN];        // reserved for later use
    char imageValidationToken[TOKEN_LEN];// image validation token - can be crc, md5, sha;  for
                                                 // now will be 4 unsigned char crc
    char tagValidationToken[TOKEN_LEN]; // validation token for tag(from signiture_1 to end of // mageValidationToken)
} FILE_TAG, *PFILE_TAG;

typedef struct flashaddrinfo
{
    int flash_persistent_start_blk;   /**< primary psi, for config file */
    int flash_persistent_number_blk;
    int flash_persistent_length;      /**< in bytes */
    unsigned long flash_persistent_blk_offset;
    int flash_scratch_pad_start_blk;  /**< start of scratch pad */
    int flash_scratch_pad_number_blk;
    int flash_scratch_pad_length;     /**< in bytes */
    unsigned long flash_scratch_pad_blk_offset;
    unsigned long flash_rootfs_start_offset; /**< offset from start of flash to fs+kernel image */
    int flash_backup_psi_start_blk;  /**< starting block of backup psi. Length is
                                          the same as primary psi.  
                                          Start at begining of sector, so offset is always 0.
                                          No sharing sectors with anybody else. */
    int flash_backup_psi_number_blk;  /**< The number of sectors for primary and backup
                                       *   psi may be different due to the irregular
                                       *   sector sizes at the end of the flash. */
    int flash_syslog_start_blk;  /**< starting block of persistent syslog. */
    int flash_syslog_number_blk; /**< number of blocks */
    int flash_syslog_length;     /**< in bytes, set from CFE, note busybox syslogd uses 16KB buffer.
                                      Like backup_psi, always start at beginning of sector,
                                      so offset is 0, and no sharing of sectors. */
    int flash_meta_start_blk; /**< The first block which is used for meta info such
                                   as the psi, scratch pad, syslog, backup psi.
                                   The kernel can use everything above this sector. */
} FLASH_ADDR_INFO, *PFLASH_ADDR_INFO;

#endif