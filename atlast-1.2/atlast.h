/*

			      A T L A S T

	  Autodesk Threaded Language Application System Toolkit

		     Program Linkage Definitions

     Designed and implemented in January of 1990 by John Walker.

     This  module  contains  the  definitions  needed by programs that
     invoke the ATLAST system.	It does contain the  definitions  used
     internally   within  ATLAST  (which  might  create  conflicts  if
     included in calling programs).

		This program is in the public domain.

*/

#ifdef UCLINUX
#define atl_int	long	      /* Stack integer type */
#define atl_real double	      /* Real number type */
#else
typedef long atl_int;		      /* Stack integer type */
typedef double atl_real;	      /* Real number type */
#endif

/*  External symbols accessible by the calling program.  */

extern atl_int atl_stklen;	      /* Initial/current stack length */
extern atl_int atl_rstklen;	      /* Initial/current return stack length */
extern atl_int atl_heaplen;	      /* Initial/current heap length */
extern atl_int atl_ltempstr;	      /* Temporary string buffer length */
extern atl_int atl_ntempstr;	      /* Number of temporary string buffers */

extern atl_int atl_trace;	      /* Trace mode */
extern atl_int atl_walkback;	      /* Error walkback enabled mode */
extern atl_int atl_comment;	      /* Currently ignoring comment */
extern atl_int atl_redef;	      /* Allow redefinition of words without
                                         issuing the "not unique" warning. */
extern atl_int atl_errline;	      /* Line number where last atl_load()
					 errored or zero if no error. */

/*  ATL_EVAL return status codes  */

#define ATL_SNORM	0	      /* Normal evaluation */
#define ATL_STACKOVER	-1	      /* Stack overflow */
#define ATL_STACKUNDER	-2	      /* Stack underflow */
#define ATL_RSTACKOVER	-3	      /* Return stack overflow */
#define ATL_RSTACKUNDER -4	      /* Return stack underflow */
#define ATL_HEAPOVER	-5	      /* Heap overflow */
#define ATL_BADPOINTER	-6	      /* Pointer outside the heap */
#define ATL_UNDEFINED	-7	      /* Undefined word */
#define ATL_FORGETPROT	-8	      /* Attempt to forget protected word */
#define ATL_NOTINDEF	-9	      /* Compiler word outside definition */
#define ATL_RUNSTRING	-10	      /* Unterminated string */
#define ATL_RUNCOMM	-11	      /* Unterminated comment in file */
#define ATL_BREAK	-12	      /* Asynchronous break signal received */
#define ATL_DIVZERO	-13	      /* Attempt to divide by zero */
#define ATL_APPLICATION -14	      /* Application primitive atl_error() */

/*  Entry points  */

extern void atl_init(), atl_mark(), atl_unwind(), atl_break();
extern int atl_eval();
extern int atl_load();
extern void atl_memstat();

#ifdef EMBEDDED
#define OUTBUFF_SIZE 255
#endif

/**
 * @name ATH_fileDump
 * @brief [FDUMP] Hex dump a file.
 * @todo change yaffs to eclofs
 *
 * @param[in] Number of bytes.
 * @param[in] File name.
 *
 * @retval None.
 *
 * Display, in hex, the contents of the given file.
 *
 * @code
 *
 * E.g.
 *
 * "NAND/gauge.ini" 32 fdump
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void ATH_fileDump();

/**
 * @name ATH_dump
 * @brief [DUMP] Hex dump an area of memory.
 *
 * @param[in] Number of bytes.
 * @param[in] Start address
 *
 * @retval None.
 *
 * Display, in hex, the contents of the memory.
 *
 * @note If you attempt to access a non-existant region of memory the interpreter will probably crash.
 *
 * @code
 *
 * E.g.
 *
 * buffer 128 dump
 *
 * @endcode
 *
 * Where buffer is a pre-defined constant.
 *
 * ________________________________________________________________________
 */
void ATH_dump();
/**
 *
 * @name ATH_erase
 * @brief [ERASE] Set to 0 a region of memory.
 *
 * @param[in] Number of bytes.
 *
 * @retval None.
 *
 * Display, in hex, the contents of the memory.
 *
 * @note If you attempt to access a non-existant region of memory the interpreter will probably crash.
 *
 * E.g.
 *
 * buffer erase
 *
 * @endcode
 * ________________________________________________________________________
 */
void ATH_erase();

/**
 * @name ATH_fill
 * @brief [ERASE] Fill a region of RAM with the given 8 bit value.
 *
 * @param[in] uint8_t value.
 * @param[in] Count
 * @param[in] Address
 *
 * @retval None.
 *
 * Fill a region of RAM.
 *
 * @note If you attempt to access a non-existant region of memory the interpreter will probably crash.
 *
 * E.g.
 *
 * buffer 0xff 0xaa fill
 *
 * @endcode
 * ________________________________________________________________________
 */

void ATH_fill();
/**
 * @name INI_getSettingsDb
 * @brief [SETTING-DB@] 
 *
 *
 * @retval Global settings database instance.
 *
 * @note Using this can be dangerous, only for testing.
 *
 * E.g.
 *
 * ________________________________________________________________________
 */

void INI_getSettingsDb();

/**
 * @name INI_getBoolean()
 * @brief [INI-GET-BOOLEAN-DB@] 
 *
 *
 * @param[in] default value.
 * @param[in] char * key name.
 * @param[in] char * section name.
 * @param[in] db Instance
 *
 * @retval Global settings database instance.
 *
 * @note Using this can be dangerous, only for testing.
 *
 * @code 
 * E.g.
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void INI_getBoolean();

/**
 * @name FR_yaffsInstall()
 * @brief [YAFFS-INSTALL]  Install a device into the yaffs system.
 *
 *
 * @param[in] char * NAND device name.
 *
 * @retval boolean fail flag.
 *
 * @note A fail flag is true, if the operation was a failure.
 *
 * @code 
 * E.g.
 *
 * "NAND1" yaffs-install
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void FR_yaffsInstall();

/**
 * @name FR_yaffsMount()
 * @brief [YAFFS-MOUNT] Make the filesystem(s) on the device accesible.
 *
 *
 * @param[in] char * NAND device name.
 *
 * @retval boolean fail flag.
 *
 * @note A fail flag is true, if the operation was a failure.
 *
 * @code 
 * E.g.
 *
 * "NAND" yaffs-mount
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void FR_yaffsMount();
/**
 * @name FR_yaffsUnmount()
 * @brief [YAFFS-UNMOUNT] Disconnect a filesystem.
 *
 * @param[in] char * NAND device name.
 *
 * @retval boolean fail flag.
 *
 * @note A fail flag is true, if the operation was a failure.
 *
 * @code 
 * E.g.
 *
 * "NAND" yaffs-unmount
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void FR_yaffsUnmount();

/**
 * @name FR_yaffsSync()
 * @brief [YAFFS-SYNC] Write and cached data to persistent storage.
 *
 * @param[in] void
 *
 * @retval void
 *
 * @todo Calls elcofs_sync, should call the yaffs function directly.
 * @note A fail flag is true, if the operation was a failure.
 *
 * @code 
 * E.g.
 *
 * yaffs-sync
 *
 * @endcode
 *
 * ________________________________________________________________________
 */
void FR_yaffsUnmount();

/**
 * @name FR_yaffsfsCreateDef()
 * @brief [O_CREAT] One of a number of constants used when opening files to set the mode
 *
 * @param[in] void
 *
 * @retval O_CREAT
 *
 * @note Conveniance funtions holding system constants.  I will not document each, they are listed below:
 *
 * | Atlast Word | Function     |
 * |-------------|--------------|
 * | O_RDONLY    | Read only    |
 * | O_RDWR      | Read write   |
 * | O_TRUNC     | Truncate file|
 * | O_WRONLY    | Write only   |
 * | O_APPEND    | Append to end|
 *
 * Example of usage
 *
 * @code
 * \ Create and open a file read/write and store the
 * \ file handle in 'dataFile'
 *
 * file dataFile
 *
 * "test.txt" O_CREATE O_RDWR or dataFile fopen
 * @endcode
 *
 * ________________________________________________________________________
 */
void FR_yaffsCreateDef();

/**
 * @name FR_yaffsSeekSet()
 * @brief [SEEK_SET] One of a number of constants used when to set the position in a file.
 *
 * @param[in] void
 *
 * @retval SEEK_SET
 *
 * @note Conveniance funtions holding system constants.  I will not document each, they are listed below:
 *
 * | Atlast Word | Function |
 * |-------------|----------|
 * | SEEK_SET    | Move to spec
 * | SEEK_CUR    | Read write|
 * | SEEK_END    | Truncate file|
 *
 * Example of usage
 *
 * @code
 * \ Move the file position 32 bytes from the start of
 * \ the file.
 *
 * 32 SEEK_SET dataFile fseek
 *
 * @endcode
 * ________________________________________________________________________
 *
 */
void FR_yaffs_yaffsSeekSet();

/**
 * @name FR_dataSize()
 *
 * @brief [DATA-SIZE] Return size of a batch data record.
 *
 * @param[in] void
 *
 * @retval int size
 *
 * @note
 *
 * Example of usage
 *
 * @code
 * \ Allocate a region of memeory to hold the size
 * \ and save the pointer in a constant.
 *
 * data-size allocate constant /record
 *
 * @endcode
 *
 */
void FR_dataSize();

/**
 * @name FR_batchInit()
 * @brief [BATCH-INIT] Setup the files for a batching instance.
 *
 * @param[in] Basename for batch files.
 * @param[in] Max number of batches.
 * @param[in] Max number of readings.
 *
 * @retval Instance of class.
 *
 * @note Generally only done once when the gauge is 'out of the box'
 *
 * Example of usage
 *
 * @code
 * \ 
 * \ Setup for 2500 batches, 10 000 reading
 * \
 *
 * 10000 2500 batch batch-init constant batchData
 *
 * @endcode
 *
 */
void FR_batchInit();

/**
 * @name FR_getBatchClassInstance()
 * @brief [0GET-BATCH-INSTANCE] Get the currently active batch instance.
 *
 * @param[in] None
 *
 * @retval Instance of class
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */

/**
 * @name
 * @brief
 *
 * @param[in]
 *
 * @retval
 *
 * @note
 *
 * Example of usage
 *
 * @code
 *
 * @endcode
 *
 */


