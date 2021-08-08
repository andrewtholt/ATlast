/******************************************************************************
* @file     atlast..c
* @date    March 2017
* @author   Modified by Andrew Holt.
* @brief    Forth like debug command line.
*
* @details  Based on the public domain program ATLAST (see below).
*
* @copyright <center><h2>COPYRIGHT Elcometer Limited 2010-2018.
* 			<br>All rights reserved.</h2>
*           This document is the property of Elcometer Limited.<br>
*           Reproduction, replication of storage in any medium by third
*           parties is forbidden except with written authority.</center><br><br>
*****************************************************************************/

// Original Header.
/*

   A T L A S T

   Autodesk Threaded Language Application System Toolkit

   Main Interpreter and Compiler

   Designed and implemented in January of 1990 by John Walker.

   This program is in the public domain.

*/
#include <malloc.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <gdisphw.h>
#include "ProbeUtils.h"

#include "BostonGauge.h"
#include "BatchBoston.h"
#include "ReadingBoston.h"
#include "VariantHW.h"
#include "ATH_Serial.h"
#include "btUtils.h"

// #include <iostream>

bool inFlash(void *ptr) ;


#ifdef ELCOFS
#include "ElcoFS.h"
#endif

#ifdef FREERTOS

#include "tim.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
//#include "NANDFlash.h"
#include "stm32f4xx_hal_qspi.h"
#include "rxRingBufferUtils.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "atlcfig.h"
#include "atlast.h"
#include "logger.h"
#include "crc32.h"

#ifdef PUBSUB

#include "Small.h"
#include "iniUtils.h"
#include "iniParser.h"

#include "cmdParser.h"
#include "freeRTOSParser.h"

#ifdef LINUX
#include <mqueue.h>
#endif
#endif

//#include "atldef.h"

#ifdef LINUX
#include <unistd.h>
#ifdef PTHREAD
#include <pthread.h>
#endif
#endif

#ifdef FILEIO
#ifdef FREERTOS
#ifdef YAFFS
#include "yaffs_utils.h"
#include "yaffs_install.h"
#include "yportenv.h"
#endif
#endif
#endif

// #define MEMSTAT

/*
#ifdef ALIGNMENT
#ifdef __TURBOC__
#include <mem.h>
#else
#include <memory.h>
#endif
#endif
*/

#ifdef EMBEDDED
bool semihosting=false;

#ifdef FREERTOS
#include "usart.h"
//#include "ATH_serial.h"
//#include "tasks.h"
#include "cmsis_os.h"
//#include "elco.h"

#ifdef INIPARSER
extern Small *settingsDb;
#endif

using namespace std;

// extern osPoolId mpool_id;

extern UART_HandleTypeDef *console;
char *outBuffer;

#endif
#endif

#undef BATCHING
// #undef ELCOFS
static int token( char **);

/*  Custom configuration.  If the tag CUSTOM has been defined (usually on
    the compiler call line), we include the file "atlcfig.h", which may
    then define INDIVIDUALLY and select the subpackages needed for its
    application.  */

#ifdef CUSTOM
#include "atlcfig.h"
#endif

/*  Subpackage configuration.  If INDIVIDUALLY is defined, the inclusion
    of subpackages is based on whether their compile-time tags are
    defined.  Otherwise, we automatically enable all the subpackages.  */

#ifndef INDIVIDUALLY
#define ARRAY			      /* Array subscripting words */
#define BREAK			      /* Asynchronous break facility */
#define COMPILERW		      /* Compiler-writing words */
#define CONIO			      /* Interactive console I/O */
#define DEFFIELDS		      /* Definition field access for words */
#define DOUBLE			      /* Double word primitives (2DUP) */
#define EVALUATE		      /* The EVALUATE primitive */
#define FILEIO			      /* File I/O primitives */
#define MATH			      /* Math functions */
#define MEMMESSAGE		      /* Print message for stack/heap errors */
#define PROLOGUE		      /* Prologue processing and auto-init */
#define REAL			      /* Floating point numbers */
#define SHORTCUTA		      /* Shortcut integer arithmetic words */
#define SHORTCUTC		      /* Shortcut integer comparison */
#define STRING			      /* String functions */
#define SYSTEM			      /* System command function */
#ifndef NOMEMCHECK
#define TRACE			      /* Execution tracing */
#define WALKBACK		      /* Walkback trace */
#define WORDSUSED		      /* Logging of words used and unused */
#endif /* NOMEMCHECK */
#endif /* !INDIVIDUALLY */


#include "atldef.h"

void atl_init();
void atl_mark(atl_statemark *mp);
void atl_unwind(atl_statemark *mp);
void atl_break();
int atl_eval(const char *sp);
int atl_load(int fp);
void atl_memstat();

#ifdef MATH
#include <math.h>
#endif

/* LINTLIBRARY */

/* Implicit functions (work for all numeric types). */

#ifdef abs
#undef abs
#endif
#define abs(x)	 ((x) < 0    ? -(x) : (x))
#define max(a,b) ((a) >  (b) ?	(a) : (b))
#define min(a,b) ((a) <= (b) ?	(a) : (b))

// ATH runflag, setup at the end of atl_init
// dictword *rf;

/*  Globals imported  */

/*  Data types	*/

/* typedef enum {False = 0, True = 1} Boolean;
 *
 */

#define EOS     '\0'                  /* End of string characters */

#define V	(void)		      /* Force result to void */

#define Truth	-1L		      /* Stack value for truth */
#define Falsity 0L		      /* Stack value for falsity */

/* Utility definition to get an  array's  element  count  (at  compile
   time).   For  example:

   int  arr[] = {1,2,3,4,5};
   ...
   printf("%d", ELEMENTS(arr));

   would print a five.  ELEMENTS("abc") can also be used to  tell  how
   many  bytes are in a string constant INCLUDING THE TRAILING NULL. */

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

/*  Globals visible to calling programs  */


atl_int atl_stklen = 100;	      /* Evaluation stack length */
atl_int atl_rstklen = 100;	      /* Return stack length */
// atl_int atl_heaplen = 1000;	      /* Heap length */
atl_int atl_heaplen = 4000;	      /* Heap length */
atl_int atl_ltempstr = 256;	      /* Temporary string buffer length */
atl_int atl_ntempstr = 4;	      /* Number of temporary string buffers */

atl_int atl_trace = Falsity;	      /* Tracing if true */
atl_int atl_walkback = Truth;	      /* Walkback enabled if true */
atl_int atl_comment = Falsity;	      /* Currently ignoring a comment */
atl_int atl_redef = Truth;	      /* Allow redefinition without issuing
                                     the "not unique" message. */
atl_int atl_errline = 0;	      /* Line where last atl_load failed */

#ifdef ATH
atl_int ath_safe_memory = Truth;
#endif

/*  Local variables  */

/* The evaluation stack */

Exported stackitem *stack = NULL;     /* Evaluation stack */
Exported stackitem *stk;	      /* Stack pointer */
Exported stackitem *stackbot;	      /* Stack bottom */
Exported stackitem *stacktop;	      /* Stack top */

/* The return stack */

Exported dictword ***rstack = NULL;   /* Return stack */
Exported dictword ***rstk;	      /* Return stack pointer */
Exported dictword ***rstackbot;       /* Return stack bottom */
Exported dictword ***rstacktop;       /* Return stack top */

/* The heap */

Exported stackitem *heap = NULL;      /* Allocation heap */
Exported stackitem *hptr;	      /* Heap allocation pointer */
Exported stackitem *heapbot;	      /* Bottom of heap (temp string buffer) */
Exported stackitem *heaptop;	      /* Top of heap */

/* The dictionary */

Exported dictword *dict = NULL;       /* Dictionary chain head */
Exported dictword *dictprot = NULL;   /* First protected item in dictionary */

/* The temporary string buffers */

Exported char **strbuf = NULL;	      /* Table of pointers to temp strings */
Exported int cstrbuf = 0;	      /* Current temp string */

/* The walkback trace stack */

#ifdef WALKBACK
static dictword **wback = NULL;       /* Walkback trace buffer */
static dictword **wbptr;	      /* Walkback trace pointer */
#endif /* WALKBACK */

#ifdef MEMSTAT
stackitem *stackmax;	      /* Stack maximum excursion */
dictword ***rstackmax;       /* Return stack maximum excursion */
stackitem *heapmax;	      /* Heap maximum excursion */
#endif


#ifdef FILEIO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifdef LINUX
static const char *fopenmodes[] = {
#ifdef FBmode
#define FMspecial
    /* Fopen() mode table for systems that require a "b" in the
       mode string for binary files. */
    "", "r",  "",   "r+",
    "", "rb", "",   "r+b",
    "", "",   "w",  "w+",
    "", "",   "wb", "w+b"
#endif
#ifndef FMspecial
        /* Default fopen() mode table for SVID-compatible systems not
           overridden by a special table above. */
        "", "r",  "",   "r+",
    "", "r",  "",   "r+",
    "", "",   "w",  "w+",
    "", "",   "w",  "w+"
#endif
};
#endif

#pragma GCC diagnostic pop

#endif /* FILEIO */

static char tokbuf[128];	      /* Token buffer */
static char *instream = NULL;	      /* Current input stream line */
static long tokint;		      /* Scanned integer */
#ifdef REAL
static atl_real tokreal;	      /* Scanned real number */
#ifdef ALIGNMENT
Exported atl_real rbuf0, rbuf1, rbuf2; /* Real temporary buffers */
#endif
#endif
static long base = 10;		      /* Number base */
Exported dictword **ip = NULL;	      /* Instruction pointer */
Exported dictword *curword = NULL;    /* Current word being executed */
static int evalstat = ATL_SNORM;      /* Evaluator status */
static Boolean defpend = False;       /* Token definition pending */
static Boolean forgetpend = False;    /* Forget pending */
static Boolean tickpend = False;      /* Take address of next word */
static Boolean ctickpend = False;     /* Compile-time tick ['] pending */
static Boolean cbrackpend = False;    /* [COMPILE] pending */
Exported dictword *createword = NULL; /* Address of word pending creation */
static Boolean stringlit = False;     /* String literal anticipated */
#ifdef BREAK
static Boolean broken = False;	      /* Asynchronous break received */
#endif
prim P_con();

#ifdef COPYRIGHT
#ifndef HIGHC
#ifndef lint
static
#endif
#endif
char copyright[] = "ATLAST: This program is in the public domain.";
#endif

/* The following static cells save the compile addresses of words
   generated by the compiler.  They are looked up immediately after
   the dictionary is created.  This makes the compiler much faster
   since it doesn't have to look up internally-reference words, and,
   far more importantly, keeps it from being spoofed if a user redefines
   one of the words generated by the compiler.	*/

static stackitem s_exit, s_lit, s_strlit, s_dotparen,
                 s_qbranch, s_branch, s_xdo, s_xqdo, s_xloop,
                 s_pxloop, s_abortq;
#ifdef REAL
static stackitem s_flit;
#endif
/*  Forward functions  */

STATIC void exword(dictword *wp), trouble(const char *kind);
#ifndef NOMEMCHECK
STATIC void notcomp(), divzero();
#endif
#ifdef WALKBACK
STATIC void pwalkback();
#endif
prim P_cr() ;
prim P_create();

#ifdef ATH
void ATH_Features() {

#ifdef ARRAY
    sprintf(outBuffer,"\n    ARRAY\r\n");
#else
    sprintf(outBuffer,"\nNOT ARRAY\r\n");
#endif

	 printf(outBuffer);

#ifdef EMBEDDED
    sprintf(outBuffer,"    EMBEDDED\r\n");
#else
    sprintf(outBuffer,"NOT EMBEDDED\r\n");
#endif

	 printf(outBuffer);

#ifdef BREAK
    sprintf(outBuffer,"    BREAK\r\n");
#else
    sprintf(outBuffer,"NOT BREAK\r\n");
#endif
	 printf(outBuffer);

//
#ifdef COMPILERW
    sprintf(outBuffer,"    COMPILERW\r\n");
#else
    sprintf(outBuffer,"NOT COMPILERW\r\n");
#endif

	 printf(outBuffer);
//
#ifdef PUBSUB
    sprintf(outBuffer,"    PUBSUB\r\n");
#else
    sprintf(outBuffer,"NOT PUBSUB\r\n");
#endif

	 printf(outBuffer);
//

#ifdef CONIO
    sprintf(outBuffer,"    CONIO\r\n");
#else
    sprintf(outBuffer,"NOT CONIO\r\n");
#endif
	 printf(outBuffer);

#ifdef DEFFIELDS
    sprintf(outBuffer,"    DEFFIELDS\r\n");
#else
    sprintf(outBuffer,"NOT DEFFIELDS\r\n");
#endif
	 printf(outBuffer);

#ifdef DOUBLE
    sprintf(outBuffer,"    DOUBLE\r\n");
#else
    sprintf(outBuffer,"NOT DOUBLE\r\n");
#endif
	 printf(outBuffer);

#ifdef EVALUATE
    sprintf(outBuffer,"    EVALUATE\r\n");
#else
    sprintf(outBuffer,"NOT EVALUATE\r\n");
#endif
	 printf(outBuffer);

#ifdef FILEIO
    sprintf(outBuffer,"    FILEIO\r\n");
#else
    sprintf(outBuffer,"NOT FILEIO\r\n");
#endif
	 printf(outBuffer);

#ifdef MATH
    sprintf(outBuffer,"    MATH\r\n");
#else
    sprintf(outBuffer,"NOT MATH\r\n");
#endif
	 printf(outBuffer);

#ifdef MEMMESSAGE
    sprintf(outBuffer,"    MEMMESSAGE\r\n");
#else
    sprintf(outBuffer,"NOT MEMMESSAGE\r\n");
#endif
	 printf(outBuffer);

#ifdef PROLOGUE
    sprintf(outBuffer,"    PROLOGUE\r\n");
#else
    sprintf(outBuffer,"NOT PROLOGUE\r\n");
#endif
	 printf(outBuffer);

#ifdef REAL
    sprintf(outBuffer,"    REAL\r\n");
#else
    sprintf(outBuffer,"NOT REAL\r\n");
#endif
	 printf(outBuffer);

#ifdef SHORTCUTA
    sprintf(outBuffer,"    SHORTCUTA\r\n");
#else
    sprintf(outBuffer,"NOT SHORTCUTA\r\n");
#endif
	 printf(outBuffer);

#ifdef SHORTCUTC
    sprintf(outBuffer,"    SHORTCUTC\r\n");
#else
    sprintf(outBuffer,"NOT SHORTCUTC\r\n");
#endif
	 printf(outBuffer);

#ifdef STRING
    sprintf(outBuffer,"    STRING\r\n");
#else
    sprintf(outBuffer,"NOT STRING\r\n");
#endif
	 printf(outBuffer);

#ifdef SYSTEM
    sprintf(outBuffer,"    SYSTEM\r\n");
#else
    sprintf(outBuffer,"NOT SYSTEM\r\n");
#endif
	 printf(outBuffer);

#ifdef TRACE
    sprintf(outBuffer,"    TRACE\r\n");
#else
    sprintf(outBuffer,"NOT TRACE\r\n");
#endif
	 printf(outBuffer);

#ifdef WALKBACK
    sprintf(outBuffer,"    WALKBACK\r\n");
#else
    sprintf(outBuffer,"NOT WALKBACK\r\n");
#endif
	 printf(outBuffer);

#ifdef WORDSUSED
    sprintf(outBuffer,"    WORDSUSED\r\n");
#else
    sprintf(outBuffer,"NOT WORDSUSED\r\n");
#endif
	 printf(outBuffer);

// ------------------
#ifdef ATH
    sprintf(outBuffer,"\r\n    ATH CUSTOM\r\n");
#else
    sprintf(outBuffer,"\r\nNOT ATH CUSTOM\r\n");
#endif
	 printf(outBuffer);
// ------------------
#ifdef PUBSUB
    sprintf(outBuffer,"    PUBSUB\r\n");
#else
    sprintf(outBuffer,"NOT PUBSUB\r\n");
#endif
	 printf(outBuffer);
// ------------------

#ifdef FREERTOS
    sprintf(outBuffer,"    FREERTOS\r\n");
#else
    sprintf(outBuffer,"NOT FREERTOS\r\n");
#endif
	 printf(outBuffer);

#ifdef ANSI
    sprintf(outBuffer,"    ANSI\r\n");
#else
    sprintf(outBuffer,"NOT ANSI\r\n");
#endif
	 printf(outBuffer);
}

prim ATH_Instream() {
    Push=(stackitem)instream;
}

prim ATH_Token() {
    V token(&instream);

    V strcpy(strbuf[cstrbuf], tokbuf);
    Push = (stackitem) strbuf[cstrbuf];
}

prim ATH_pwd() {
    Sl(2); // pointer to a memory area large enough to hold the biggest path
#ifdef LINUX
    char *p;

    p=getcwd( (char *)S1, (size_t)S0);
    Pop;

    if(!p) {
        S0=true;
    } else {
        S0=false;
    }
#else
    Pop;
    S0=true;

#endif
}

prim ATH_cd() {
    Sl(1);
#ifdef LINUX
    int rc=0;
    rc=chdir(S0);
    if(rc < 0) {
        S0=true;
    } else {
        S0=false;
    }
#else
    S0=true;
#endif
}
 prim ATH_errno() {
	 Sl(0);
	 So(1);
	 Push=errno;

	 errno=0;
}

prim ATH_help() {
	Sl(0);
	So(0);

	printf("\r\nHelp is not avaialable, yet\r\n") ;
	printf("\r\nHowever, set your terminal to local echo.\r\n") ;
}

prim ATH_banner() {
		uint8_t msgBuffer[80];
		memset(msgBuffer,0,80);

		printf("\r\nBased on ATLAST 1.2 (2007-10-07)\n");
		printf("\rThe original version of this program is in the public domain.\n");
		printf("\rModifications for FreeRTOS are the property of Elcometer Ltd.\r\n");

		strcpy((char *)msgBuffer,"\r\nCompiled: " );
		strcat((char *)msgBuffer, VariantHW::GetGaugeBuildDate());
		strcat((char *)msgBuffer," ");
		strcat((char *)msgBuffer, VariantHW::GetGaugeBuildTime());
		strcat((char *)msgBuffer,"\r\n");

		printf("%s",msgBuffer);
}

#ifdef FREERTOS
prim ATH_perror() {

	Sl(1);
	So(0);

	printf("%s:%s",(char*)S0,strerror(errno));
	P_cr();
}
#endif

prim ATH_fsSize() {
    Sl(1);
    So(1);
#ifdef YAFFS
    char *path;
    int size;

    path=(char *)S0;

    size = yaffs_totalspace(path);

    S0=(stackitem)size;
#else
    Pop;
#endif

}

prim ATH_fsFree() {
    Sl(1);
    So(1);

#ifdef YAFFS
    char *path;
    int size;

    path=(char *)S0;

    size=yaffs_freespace(path);
    S0=(stackitem)size;
#else
    Pop;
#endif

}

int32_t directory(const char *dname, bool showHeader = true)
{
	uint32_t return_value = 0;
	void *d;
	elcofs_entry_t *de;
	char full_path[MAX_FILENAME_SIZE+16];	// Linux compiler warning fix.

	// TODO Fix directory
	d = elcofs_opendir(dname);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
	if(!d) {
		printf("opendir failed\n");
		return_value = -1;
	} else {
		if (showHeader) {
			printf("+----------------------------------------+------------+----------------------+\r\n");
			printf("| File                                   |     Length | Type                 |\r\n");
			printf("+----------------------------------------+------------+----------------------+\r\n");
		}

		while((de = elcofs_readdir(d)) != NULL) {
			// skip over system indicators

//			if (de->filename[0] == '.' || de->filename[0] == '..') {
			if (de->filename[0] == '.' ) {
				continue;
			}

			sprintf(full_path, "%s/%s", dname, de->filename);

			printf("| %-38s | %10d | %-20s |\r\n", full_path, (int)de->size, de->isDirectory ? "directory" : "file");

			if(de->isDirectory) {
				directory(full_path, false);
			}
		}

		if (showHeader) {
			printf("+----------------------------------------+------------+----------------------+\r\n");
		}
#pragma GCC diagnostic pop

		elcofs_closedir(d);
	}

	return return_value;
}

prim RT_dir() {
    Sl(0);
    So(0);

    directory("");

    S0 =  false;
}

prim RT_touch() {
    Sl(1);										// we expect 1 item on the stack (full file name)
    So(0);										// we aren't adding anything to the stack (we are replacing name with success/fail

    char *name = (char*) S0;

    int fd = elcofs_open(name, ELCOFS_CREAT | ELCOFS_TRUNC | ELCOFS_RDWR);

    if (fd >= 0)
    {
    	elcofs_close(fd);
    }

	S0 =  fd >= 0;
}

prim RT_mkfile() {
    Sl(2);										// we expect 2 item on the stack (full file name and length)
    So(0);										// we aren't adding anything to the stack (we are replacing 2 items with just 1 - success/fail)

    char *name = (char*)S1;
    int length = S0;

    Pop;										// trash 1 item

	int fd = elcofs_open(name, ELCOFS_CREAT | ELCOFS_TRUNC | ELCOFS_RDWR);

	if (fd >= 0)
	{
		uint8_t dummy = 0;

		for (int i = 0; i < length; i++)
		{
			elcofs_write(fd, &dummy, 1);
		}

		elcofs_close(fd);
	}

	S0 =  fd >= 0;
}

int32_t crc_file(char *name, uint32_t *crc)
{
	int32_t return_value = 0;
	int32_t file_handle;
	uint8_t buffer[1024];
	uint32_t result;
	uint32_t calc_crc;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
	printf("crc_file(%s)\r\n", name);

	file_handle = elcofs_open(name, ELCOFS_RDONLY);

	printf("elcofs_open(%s) %d\r\n", name, (int)file_handle);

	if (file_handle < 0)
	{
		return_value = -1;
	}
	else
	{
		calc_crc = -1;

		do
		{
			result = elcofs_read(file_handle, &buffer, 1024);

			printf("elcofs_read() %d\r\n", (int)result);

			calc_crc = crc32(calc_crc, buffer, result);

		} while (result > 0);

		result = elcofs_close(file_handle);

		printf("elcofs_close() %d\r\n", (int)result);
#pragma GCC diagnostic pop

		if (result != 0)
		{
			return_value = -1;
		}
		else
		{
			*crc = ~calc_crc;
		}
	}

	return (return_value);
}

prim RT_crcfile() {
    Sl(1);										// we expect 1 item on the stack (full file name)
    So(1);										// we are possibly adding 1 item to the stack (we are replacing 1 item with 1 or 2)

    char *name = (char*) S0;
    int crc;
    int rc;

    Pop;										// trash the name

    rc = crc_file(name, (uint32_t *)&crc);		// crc returns -1 for fail, 0 for success

    if (rc < 0)									// failed
    {
    	Push = true;							// return fail
    }
    else
    {
    	Push = crc;								// return crc ...
    	Push = false;							// ... and success
    }
}

prim ATH_ms() {
    Sl(1);
#ifdef LINUX
    usleep((useconds_t)S0 * 1000);
#endif

#ifdef FREERTOS
    vTaskDelay(pdMS_TO_TICKS((uint32_t)S0));

#endif
    Pop;
}
prim ATH_qlinux() {
#ifdef LINUX
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qfreertos() {
#ifdef FREERTOS
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qdebug() {
	Push=!elcofs_is_mounted();
}

prim ATH_qyaffs() {
    Sl(0);
    So(1);
#ifdef YAFFS
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qelcofs() {
    Sl(0);
    So(1);
#ifdef ELCOFS
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qbatching() {
    Sl(0);
    So(1);
#ifdef BATCHING
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qfileio() {
    So(1);

#ifdef FILEIO
    Push=-1;
#else
    Push=0;
#endif
}
prim ATH_memsafe() {
    Sl(1);
    ath_safe_memory = (S0 == 0) ? Falsity : Truth;
    Pop;
}

prim ATH_qmemsafe() {
    So(1);
    Push = ath_safe_memory;
}
// TODO Tidy this up.  Make non embedded version still copy to outBuffer,
// but change output method.
//
void displayLineHex(uint8_t *a) {
	int i;
	char localBuffer[8];

	outBuffer[0]='\0';

	for(i=0;i<16;i++) {
		sprintf(localBuffer," %02x",*(a++));
		strcat( outBuffer, localBuffer);
	}

	printf(outBuffer);
}

void displayLineAscii(uint8_t *a) {
    int i;

     printf(":");

    for(i=0;i<16;i++) {
        if( (*a < 0x20 ) || (*a >= 0x80 )) {
            printf(".");
            a++;
        } else {
            printf("%c",*(a++));
        }
    }
    printf("\r\n");
}

// Stack: filename --
//
prim ATH_fileDump() {
	Sl(2);
	So(0);
	off_t loc=0;

	bool runFlag=false;

	char *fileName = (char *)S1;
	int count = (int) S0;

#ifndef LINUX
	uint8_t buffer[16];
	volatile int len;

    printf("\n");

	int fd = elcofs_open(fileName, O_RDONLY);

	if(fd >= 0) {
		do {
			loc=elcofs_lseek(fd,0,SEEK_CUR);

			printf("%08ld:",loc);

			len = elcofs_read(fd, &buffer, 16);

			count -= len;
			displayLineHex((uint8_t *)&buffer);
			displayLineAscii((uint8_t* ) &buffer );

			if( len > 0 && count >0) {
				runFlag = true;
			} else {
				runFlag = false;
			}
		} while( runFlag );
	}
	elcofs_close(fd);
#endif
	Pop2;
}

prim ATH_dump() {
    Sl(2); // address len

    int len = S0;
    uint8_t *address = (uint8_t *) S1;
    int lines=len/16;

#ifndef DEBUG
    bool flag = inFlash( (void *)address);
    flag |= inFlash( (void *) ( address + len ));

    if(flag) {
        badpointer();
    }
#endif

    if(lines ==0 ) {
        lines=1;
    }
    outBuffer[0]='\0';
    printf("\r\n") ;

    int i=0;
    for( i = 0; i<len;i+=16) {
        printf("%08x:", (uintptr_t)address);
        displayLineHex( address );
        displayLineAscii( address );
        address +=16;
    }

    Pop2;
}

prim ATH_erase() {
    Sl(2);

    int length = S0;
    uint8_t *address = (uint8_t *) S1;

    memset(address,0,length);

    Pop2;
}
//
// Stack: address len val
//
prim ATH_fill() {
    Sl(3);

    uint8_t d = S0;
    int length = S1;
    uint8_t *address = (uint8_t *) S2 ;

    memset(address,d,length);

    Pop2;
    Pop;
}

prim ATH_hex() {
    base = 16;
}

prim ATH_dec() {
    base = 10;
}

prim ATH_bye() {
//    *((int *) atl_body(rf)) = 0;
#ifndef LINUX
	atl_eval("0 DBG_RUN !");
#else
	exit(0);
#endif
}

int8_t readLineFromArray(uint8_t *src, uint8_t *dest) {
    uint8_t ch;
    int i;
    int8_t len=0;
    static uint32_t offset=0;
    
    for(i=0;i<MAX_LINE;i++) {
        ch=*(src+(offset++));
        if ( ch == 0 ) {
            len=-1;
            *dest=0;
            break;
        } else if( ch == '\n' || ch == '\r' ) {
            *dest=0;
            break;
        } else {
            *(dest++)=ch;
            len=i;
        }
    }
    return len;
}
#endif // ATH
#ifdef ANSI
prim ANSI_cell() {
    So(1);
    Push = sizeof(int);
}

prim ANSI_cells() {
    Sl(1);
    S0 =  S0 * sizeof(int);
}

prim ANSI_cellplus() {
    Sl(1);
    S0 =  S0 + sizeof(int);
}

prim ANSI_chars() {
	Sl(1);

	S0 =  sizeof(uint8_t) * S0;

}

prim ANSI_allocate() {
	void *ptr=NULL;
	Sl(1);
	So(2);

	ptr=pvPortMalloc(S0);
//	ptr=malloc(S0);
	if(ptr != NULL) {
//		memset(ptr,0,S0);
		Pop;
		Push=(stackitem)ptr;
		Push=0;
	} else {
		Push=0;
		Push=-1;
	}
}

prim ANSI_free() {
	Sl(1);

	if(S0 != 0 ) {
		free((void *)S0);
	}
	Pop;
}

prim ATH_sbrk() {
	extern char _estack;
	Push =  (stackitem) sbrk(0);
	Push =  (stackitem) &_estack ;
}

prim ATH_mallinfo() {
	Sl(1);
	So(2);

	struct mallinfo tmp;

	tmp=mallinfo();

	extern char end;

	memcpy((struct mallinfo *)S0, (void *)&tmp, sizeof(struct mallinfo));
	Push = sizeof(struct mallinfo);
	Push = end;
}

prim ATH_mallinfoArena() {

	struct mallinfo *tmp;
	tmp=(struct mallinfo *)S0;
	S0=tmp->arena;
}

prim ATH_mallinfoUsed() {
	struct mallinfo *tmp;
	tmp=(struct mallinfo *)S0;
	S0=tmp->uordblks;
}

prim ATH_mallinfoFree() {
	struct mallinfo *tmp;
	tmp=(struct mallinfo *)S0;
	S0=tmp->fordblks;
}
#endif // ANSI

prim FR_setConsole() {
	Sl(1);

	VariantHW::DebugCommsChannelType cons = (VariantHW::DebugCommsChannelType)S0;

	VariantHW::SetDebugChannel(cons);

}
#ifdef INIPARSER

prim INI_getSettingsDb() {
	Sl(0);
	So(1);

	Push = (stackitem) settingsDb;
}

prim INI_loadIniFile() {
	Sl(2);
	So(1);

	struct Small *db = (struct Small *)S1;
	char *fname = (char *)S0;

	bool failFlag=true;

	iniAdd(db, fname);

	Pop2;
//	S0=(stackitem)failFlag;
}

// Stack: db section key default -- value

prim INI_getBoolean() {
	Sl(4);
	So(1);


	int defaultValue=(int)S0;

	char *key = (char *)S1;
	char *section = (char *)S2;

	int len = strlen(key) + strlen(section) +2;

	char buffer[len];
	strcpy(buffer,section);
	strcat(buffer,".");
	strcat(buffer,key);

	Small *db = (Small *)S3;

	const char *res= db->dbLookup(buffer);

	Npop(4);

	if( res == NULL) {
		Push=defaultValue;
	}

}

#endif

#ifdef FREERTOS
#ifdef NAND
prim FR_mkScmd() {
	Sl(0);

	P_create();
	int sz= sizeof(QSPI_CommandTypeDef);

	Ho((sz + 1 + sizeof(stackitem)) / sizeof(stackitem));
	hptr += (sz + 1 + sizeof(stackitem)) / sizeof(stackitem);
//	Hstore = 0;
	Push=sz;
}

prim FR_NANDSetup() {
	QSPI_CommandTypeDef *scommand;
	Sl(1);
	So(1);

	scommand = (QSPI_CommandTypeDef *)S0;

	S0=(uint8_t)NAND_Setup(scommand);

}

prim FR_NANDWrite() {
	Sl(5);
	So(1);

	uint32_t datalen = S0;
	uint8_t *databuff = (uint8_t*)S1;
	uint16_t bpaddress=S2;
	uint16_t coladdress=S3;

	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef *)S4;

	uint8_t rc=NAND_Write(scommand,coladdress, bpaddress, databuff, datalen);

	Npop(4);
	S0=rc;
}

prim FR_NANDRead() {
	Sl(5);
	So(1);

	uint32_t datalen = S0;
	uint8_t *databuff = (uint8_t*)S1;
	uint16_t bpaddress=S2;
	uint16_t coladdress=S3;

	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef *)S4;

	uint8_t rc=NAND_Read(scommand,coladdress, bpaddress, databuff, datalen);

	Npop(4);
	S0=rc;
}

prim FR_NANDBlocks() {
	Push=NO_BLOCKS;
}

prim FR_badBlockMap() {
	Sl(2);
	So(1);

	uint8_t *bbt;
	uint8_t rc=0;
	QSPI_CommandTypeDef *scommand;

	scommand = (QSPI_CommandTypeDef *)S1;
	bbt = (uint8_t *)S0;
	rc=NAND_Bad_Block_Map(scommand, bbt);

	Pop2;
	Push=rc;
}

prim FR_NANDUnlock() {

	Sl(1);
	So(0);

	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef*) S0;

	NAND_Unlock(scommand);
}

prim FR_NANDLock() {
	Sl(1);
	So(0);

	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef*) S0;

	NAND_Lock(scommand);
}

prim FR_NANDErase() {
	Sl(2);
	So(1);

	uint16_t block = S0;
	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef*) S1;

	uint8_t rc = NAND_Erase_Block(scommand, (block << 6));

	Pop;
	S0=rc;
}

prim FR_NANDGetParams() {

	uint8_t rc=0;

	uint8_t *param= (uint8_t*) S0;
	QSPI_CommandTypeDef *scommand = (QSPI_CommandTypeDef*) S1;

	rc=NAND_Get_Parameters(scommand, param);

	Pop;
	S0=rc;


}
#endif

#ifdef YAFFS
prim FR_yaffsInstall() {
	Sl(1);
	So(1);

	char *name;
	int rc;

	name=(char *)S0;

	rc = yaffs_install(name);
	S0=rc;
}
#endif

#ifdef ELCOFS

prim FR_elcofsFormat() {
	Sl(0);
	So(1);

	int rc;

	// TODO pass in the full set of params.

	rc = elcofs_format();

	S0 =  rc;
}


prim FR_elcofsMount() {
	Sl(0);
	So(1);

	int rc;

	rc = elcofs_mount();

	S0=rc;
}

prim FR_elcofsUnmount() {
	Sl(0);
	So(1);

	int rc;

	rc = elcofs_unmount();

	S0=rc;
}
#endif

prim FR_fsSync() {
	Sl(0);
	So(0);

#if defined(ELCOFS)
	elcofs_sync();
#endif
}

prim FR_fsCreatDef() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_CREAT;
#else
	Push = O_CREAT;
#endif
}

prim FR_fsTruncDef() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_TRUNC;
#else
	Push = O_TRUNC;
#endif
}

prim FR_fsRdwrDef() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_RDWR;
#else
	Push = O_RDWR;
#endif
}

prim FR_fsRdOnly() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_RDONLY;
#else
	Push = O_RDONLY;
#endif
}

prim FR_fsWrOnly() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_WRONLY;
#else
	Push = O_WRONLY;
#endif
}

prim FR_fsAppend() {
	Sl(0);
	So(1);

#if defined(ELCOFS)
	Push = ELCOFS_O_APPEND;
#else
	Push = O_APPEND;
#endif
}

prim FR_yaffsSeekSet() {
	Sl(0);
	So(1);

	Push = SEEK_SET;
}

prim FR_yaffsSeekCur() {
	Sl(0);
	So(1);

	Push = SEEK_CUR;
}

prim FR_yaffsSeekEnd() {
	Sl(0);
	So(1);

	Push = SEEK_END;
}

prim FR_fixedOpen() {
	Sl(2);
	So(1);

	char *fileName=NULL;
	uint32_t blockSize=0;
	FixedFile *batchFile=NULL;

	fileName = (char *)S1;
	blockSize = (uint32_t) S0;

	batchFile = new FixedFile(fileName, blockSize);

	Pop;
	S0=(stackitem) batchFile;

}

prim FR_fixedClose() {
	Sl(1);
	So(0);

	FixedFile *batchFile=NULL;

	batchFile=(FixedFile *)S0;
	delete batchFile;

	Pop;

}

prim FR_fixedRead() {
	Sl(3);
	Sl(1);

	FixedFile *batchFile=NULL;
	char *buffer=NULL;
	uint32_t idx=0;

	batchFile=(FixedFile *)S2;
	buffer=(char *)S1;
	idx=(uint32_t)S0;

	Npop(2);
	S0=batchFile->readFixedBlock(buffer,idx);

}

prim FR_batchBucketSize() {
	Sl(0);
	So(1);

	Push=BATCH_BUCKETS ;

}

// Stack: <instance> <batch name> -- <index>
prim FR_batchCreate() {
	Sl(1);
	So(1);

	char *batchName = (char *)S0;

	batchid_t id = BostonGauge::batching().getNextNewId();

	if (id != BATCH_INVALID_ID)
	{
		BatchBase* batch = BostonGauge::batching().newStorageBatch(id);
		batch->setBatchName(batchName);
		BostonGauge::batching().addBatch(*batch);
		delete batch;
	}

	S0=(stackitem) id;
}

/*
void BatchBase::getBatchFileName(batchid_t id, char* name)
{
    if (id != BATCHMASTER_IMMEDIATE_BATCH_ID)
    {
        char folder[MAX_FILENAME_SIZE];
        sprintf(folder, "%s/batch%04d", BATCHING_ROOT, (uint32_t) id % BATCH_BUCKETS);
        sprintf(name, "%s/%04d.batch", folder, (uint32_t) id);
    }
    else
    {
    	// immediate mode batch is placed in root
    	sprintf(name, "%04d.batch", (uint32_t) id);
    }
}
*/

prim FR_mkBatchFilename() {
	Sl(2);
	So(0);

	char *name = (char *)S0;
	uint32_t id=(uint32_t)S1;

    char folder[MAX_FILENAME_SIZE];
    sprintf(folder, "%s/batch%04lu", BATCHING_ROOT, (unsigned long) id % BATCH_BUCKETS);
    sprintf(name, "%s/%04lu.batch", folder, (unsigned long) id);
}

prim FR_batchOpen() {
	Sl(1);
	So(1);

    int batchId = (int )S0;
	BatchBoston *inst = BostonGauge::batching().openBatch(batchId);

	S0 = (stackitem)inst;
}

prim FR_getFirstBatch() {
	Sl(0);
	So(1);

	batchid_t batchId = BostonGauge::batching().getFirstBatch();

	Push=(stackitem) batchId;
}

prim FR_getNextBatch() {
	Sl(1);
	So(1);

	batchid_t current=S0;
	batchid_t batchId = BostonGauge::batching().getNextBatch(current);

	S0=batchId;

}

prim FR_getLastBatch() {
	Sl(0);
	So(1);

	batchid_t batchId = BostonGauge::batching().getLastBatch();

	Push=(stackitem) batchId;
}

// Stack: <batchClass instance> --

prim FR_getNumberOfBatches() {
	Sl(0);
	So(1);

	uint32_t batchCount = BostonGauge::batching().numberOfBatches();

	Push=(stackitem) batchCount;
}

prim FR_batchClose() {
	Sl(1);
	So(0);

	BatchBase* batch = (BatchBase*) S0;

	BostonGauge::batching().closeBatch(batch);
	Pop;
}

prim FR_batchRename() {
	Sl(2);
	So(1);

	bool failFlag = true;
	char *newName = (char *)S0;
	batchid_t batchId = S1;

	BatchBase* batch = BostonGauge::batching().openBatch(batchId);

	if (batch)
	{
		batch->setBatchName(newName);
		BostonGauge::batching().closeBatch(batch);
		failFlag = false;
	}

	Pop;
	S0 = failFlag;
}

prim FR_batchCopy() {
	Sl(1);
	So(1);

	batchid_t batchId = (batchid_t) S0;

	batchid_t newId = BostonGauge::batching().copyBatch(batchId);

	S0=newId;
}

// batch-rm
// <batch Master> idx --
prim FR_batchRm() {
	Sl(1);
	So(0);

	batchid_t batchId = (batchid_t)S0;

    BostonGauge::batching().removeBatch(batchId) ;
    Pop;
}

prim FR_batchRmAll() {
	Sl(0);
	So(0);

    BostonGauge::batching().removeAllBatches();
}

prim FR_batchRmReading() {
	Sl(1);
	So(1);

	bool failFlag=true;

    batchid_t batchId = (batchid_t) S0;

    BatchBase* batch = BostonGauge::batching().openBatch(batchId);

	if (batch) {
		batch->removeLastReading();
		BostonGauge::batching().closeBatch(batch);
		failFlag = false;
	}

    S0=(stackitem)failFlag;
}

prim FR_batchClear() {
	Sl(1);
	Sl(0);

	batchid_t batchId = (batchid_t)S0;

    BatchBase* batch = BostonGauge::batching().openBatch(batchId);

    if (batch) {
    	batch->removeBatchEntries();
    	BostonGauge::batching().closeBatch(batch);
    }
    Pop;
}

// TODO make this into:
// S3       S2            S1           S0
// Batch_ID Reading_idx   column       value
prim FR_batchAddReading() {
	Sl(2);
	So(0);

    /*
	BatchBase* batch = (BatchBase*)S1;
	ReadingBoston* reading = (ReadingBoston*) S0;

	batch->addReading(*reading);
    */
	Pop2;
}

/*
prim FR_batchGetReadingCount() {
	Sl(0);
	So(1);

	Push = BostonGauge::batching().getTotalReadingFileSize() / GAUGE_READING_SIZE;
}
*/

ReadingBoston fred;

prim FR_batchGetReadingSize() {
	Sl(0);
	So(1);

	Push = GAUGE_READING_SIZE;
}

// S3        S1      S0
// batch-id rdg-idx column  -- data

prim FR_batchGetReading() {
	Sl(2);

    int batchId =(int)S2;
    int idx = (int)S1;
    int column = (int)S0;
    
    Npop(3);
    
    BatchBase* batch = BostonGauge::batching().openBatch(batchId);
    if (batch) {
        batch->getReading(idx, fred);
        
        Push = fred.getMeasurement((ReadingColumn) column );
    }
}

prim FR_getBatchID() {
	Sl(1);
	So(1);

	uint32_t idx = (uint32_t) S0;

	batchid_t batchID = BostonGauge::batching().getBatchForSlotIndex(idx);

	S0 = batchID;
}

// batch_id buffer --
//
prim FR_getBatchName() {
	Sl(2);
	So(0);

	char *buffer = (char *)S0;

	batchid_t idx = (batchid_t)S1;

	std::string batchName = BostonGauge::batching().getBatchName(idx) ;

	strcpy(buffer, batchName.c_str());
	Pop2;
}

prim FR_getFileSize() {
	Sl(1);
	So(1);

	uint32_t size = 0;
	int32_t pfd=S0;

	size = elcofs_size(pfd);

	S0=size;
}

prim FR_getFileSizeByName() {

	uint32_t size = 0;
	int32_t pfd=0;

	char *filePath=NULL;

	filePath=(char *)S0;

	pfd=elcofs_open(filePath, O_RDONLY);

	if(pfd>=0) {
		size = elcofs_size(pfd);
	}

	elcofs_close(pfd);
	S0=size;

}

prim FR_getReadingCount() {
	Sl(1);
	So(1);

	int32_t count = 0;

	batchid_t batchId = (batchid_t)S0;
	BatchBase* batch = BostonGauge::batching().openBatch(batchId);
	if (batch) {
		count=batch->getReadingCount(false);
		BostonGauge::batching().closeBatch(batch);
	}

	S0=count;
}

prim FR_getBatchSize() {
	Sl(1);
	So(1);

	batchid_t batchId = (batchid_t)S0;
	BatchBase* batch = BostonGauge::batching().openBatch(batchId);
	int32_t size = 0;

	if (batch) {
		size = batch->getBatchSize();
		BostonGauge::batching().closeBatch(batch);
	}

	S0=size;
}

prim FR_setBatchSize() {
	Sl(2);
	So(0);

	uint32_t size = (uint32_t)S0 ;
	batchid_t batchId = (batchid_t)S1 ;

	BatchBase* batch = BostonGauge::batching().openBatch(batchId);


	if (batch) {
		batch->setBatchSize(size);
		BostonGauge::batching().closeBatch(batch);
	}

	Npop(3);
}

extern struct ringBuffer *uart4RxBuffer;

prim FR_getEvent() {
	Sl(1);
	So(1);

	EventBits_t mask = xEventGroupGetBits( (EventGroupHandle_t) S0);

	S0=(stackitem) mask ;
}
//
// Given a UART structure return the  ring buffer data pointer.
// Of the data.
//
prim FR_ringBufferPtr() {
    struct ringBuffer *ptr=NULL;
    UART_HandleTypeDef *uart;

    Sl(1);
    So(1);

    uart = (UART_HandleTypeDef *)S0;

    if(uart->Instance == UART4) {
        ptr = uart4RxBuffer;
    } else {
        ptr=NULL;
    }

    S0=(stackitem)ptr->buffer;
}

// Stack: <ring buffer ptr> -- <semaphore handle>
prim FR_getRingBufferLock() {
    Sl(1);
    So(1);
    
    struct ringBuffer *ptr;
    SemaphoreHandle_t sema;
    UART_HandleTypeDef *uart;
    
    uart = (UART_HandleTypeDef *)S0;

    if(uart->Instance == UART4) {
        ptr = uart4RxBuffer;
    } else {
        ptr=NULL;
    }
    
    sema = ptr->lock;
    
    S0=(stackitem) sema;
}

prim FR_ringBufferErase() {
    struct ringBuffer *ptr=NULL;
    UART_HandleTypeDef *uart;

    Sl(1);

    uart = (UART_HandleTypeDef *)S0;

    if(uart->Instance == UART4) {
        ptr = uart4RxBuffer;
    } else {
        ptr=NULL;
    }
    if( ptr != NULL) {
        ringBufferReset(ptr);
    }
    Pop;

}

prim FR_devReset() {
	elcofs_sync();
	HAL_NVIC_SystemReset();
}

/*
 *	?UART-RX
 *
 *	<uart> -- <true|false>
 */
prim FR_uartRxReady() {

	Sl(1);
	So(1);

	UART_HandleTypeDef *uart;
	bool failFlag = true;

	// If RX is interrupt driven then use
	// The ringBuffer struct.
	// NAMING CONVENTION: uartRxBuffer, e.g.
	// uart4RxBuffer

	uart = (UART_HandleTypeDef *)S0;
	if (uart->Instance == UART4) {
		failFlag = (uart4RxBuffer->inIdx != uart4RxBuffer->outIdx);
	} else {
		failFlag = rxReady(uart);
	}

	S0=failFlag;
}

/*
 *	UART-TYPE
 *	<uart> <address> --
 */
prim FR_uartTxBuffer() {
	Sl(2);

	uint8_t *buffer=(uint8_t *)S0;
	UART_HandleTypeDef *uart=(UART_HandleTypeDef *)S1;

	txBuffer(uart, buffer);

	Pop2;
}

prim FR_uartRxByte() {

	Sl(1);
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)S0;
	uint8_t data;
	if (huart->Instance == UART4) {
	    data=ringBufferRxByte( uart4RxBuffer, portMAX_DELAY );
	} else {
	    data = rxByteWait(huart,-1) ;
	}

	S0=(stackitem)data;
}

/*
 * UART-READLINE
 * uart buffer len -- count
 */
prim FR_uartReadLine() {
	Sl(3);
	So(1);
	uint8_t byteCount=0;

	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)S2;
	void *buffer;
	buffer =(void *) S1 ;
	uint8_t len = (uint8_t)S0;
    extern struct ringBuffer *uart5RxBuffer;

	memset(buffer,0,len);

	if( huart->Instance == UART4) {
	    byteCount = ringBufferSimpleRxLine(uart4RxBuffer, (uint8_t*)buffer, len, '\r');
	} else if( huart->Instance == UART5) {
	    byteCount = ringBufferSimpleRxLine(uart5RxBuffer, (uint8_t*)buffer, len, '\r');
	} else {
	    byteCount = simpleRxLine(huart, (uint8_t*)buffer, len,'\r') ;
	}
	Pop2;
	S0=byteCount;
}

prim FR_uartEmit() {

	Sl(2);
	UART_HandleTypeDef *huart;
	uint8_t data;

	huart=(UART_HandleTypeDef *)S1;
	data=(uint8_t)S0;

//	txByteWait(UART_HandleTypeDef *huart,uint8_t data, int timeout)
	txByteWait(huart,data, -1) ;
	Pop2;
}

prim FR_qconsoleKey() {
    extern struct ringBuffer *dataIn;
    Push=ringBufferRxReady(dataIn);
}

prim FR_consoleKey() {
    extern struct ringBuffer *dataIn;
	Push = ringBufferRxByte( dataIn, portMAX_DELAY);
}

prim FR_consoleExpect() {
	Sl(2);
	So(0);

	extern OSQueueHandle_t dbgIn;

	char *buffer = (char*)S1;

	xQueueReceive(dbgIn, buffer, portMAX_DELAY);

	Pop2;
}

prim FR_lcdReset() {
	ghw_init();
	HAL_GPIO_WritePin(nLCD_RESET_GPIO_Port,nLCD_RESET_Pin,GPIO_PIN_RESET); // Set LCD reset low.
	vTaskDelay( pdMS_TO_TICKS(500));
	HAL_GPIO_WritePin(nLCD_RESET_GPIO_Port,nLCD_RESET_Pin,GPIO_PIN_SET); // Set LCD reset high.
	vTaskDelay( pdMS_TO_TICKS(500));
}

prim FR_lcdRegSet() {
	HAL_GPIO_WritePin(nLCD_RESET_GPIO_Port,nLCD_RESET_Pin,GPIO_PIN_RESET);
	vTaskDelay( pdMS_TO_TICKS(10));
	HAL_GPIO_WritePin(nLCD_RESET_GPIO_Port,nLCD_RESET_Pin,GPIO_PIN_SET);
	vTaskDelay( pdMS_TO_TICKS(200));
}
prim FR_getSysTick() {
    uint32_t tick;

    Sl(0);
    So(1);

    tick = osKernelSysTick();

    Push=(stackitem)tick;
}

prim FR_cmdParse() {
	Sl(2);

	char *res=NULL;

	freertosParser *parser=(freertosParser *)S1;
	MsgCmdPacketType *msg=(MsgCmdPacketType *)S0;

	Pop;

	postParseAction_t act = PP_NULL;
	parser->parse(msg,  &act);

	S0=(stackitem)res;
}


prim FR_getPoolId() {
//	Push=(stackitem)mpool_id;
	Push=0;
}

prim FR_poolAllocate() {
	osPoolId pool;
	Sl(1);

	pool=(osPoolId)S0;

	MsgCmdPacketType *msg= (MsgCmdPacketType *) osPoolAlloc(pool);
	S0=(stackitem) msg;
	Push=sizeof(MsgCmdPacketType);
}

prim FR_poolFree() {

	osStatus rc=osOK;;
	osPoolId pool;

	Sl(2);
	MsgCmdPacketType *msg=(MsgCmdPacketType *)S0;
	pool=(osPoolId)S1;

	rc=osPoolFree(pool, msg);
	Pop2;
	Push=rc;
}

bool validTask(uint8_t n) {
    return (bool) ( (n > APPTASK_NONE) && (n < LAST_TASK));
}

prim FR_semGet() {
    SemaphoreHandle_t sema;
    sema=task[S0]->m_Lock;
    S0 =  (stackitem)sema;
}

prim FR_semGetCount() {
    BaseType_t counter;
    Sl(1);
    So(1);

    counter=uxQueueMessagesWaiting( (SemaphoreHandle_t )S0 );

    S0=counter;
}
prim FR_semGive() {
    SemaphoreHandle_t sema;
    BaseType_t rc;
    BaseType_t counter;
    bool failFlag=true;

    Sl(1);
    So(1);

    sema=(SemaphoreHandle_t)S0;

    counter=uxQueueMessagesWaiting( sema );

    if ( counter >= 0 ) {
        rc = xSemaphoreGive( sema);
        failFlag=(rc != pdPASS ) ? true : false ;
    } else {
        failFlag=true;
    }

    S0=failFlag ;
}

prim FR_semTake() {
    SemaphoreHandle_t sema;
    BaseType_t rc;
    bool failFlag=true;
    TickType_t delay;

    Sl(2);
    So(1);

    sema=(SemaphoreHandle_t)S1;
    delay=(TickType_t)S0;

    rc=xSemaphoreTake( sema, pdMS_TO_TICKS(delay)) ;
    Pop;

    failFlag=(rc != pdPASS ) ? true : false ;
    S0=(stackitem)failFlag;
}

prim FR_getQid() {
    TaskDataType *t;

    Sl(1);
    
    int taskNumber = S0;
    
//    if (validTask( S0 )) {
    if (validTask( taskNumber )) {
        // We have a valid task id ...
        t=task[S0];
        
        if( t != NULL) {
            xSemaphoreTake(task[taskNumber]->m_Lock, portMAX_DELAY );
            S0=(stackitem)t->m_Iam;
            xSemaphoreGive(task[taskNumber]->m_Lock);
        } else {
            S0=-1;
        }
    } else {
        S0=-1;
    }
}

prim FR_getTaskDb() {
    Sl(1);

    bool failFlag=true;

    if (validTask( S0 )) {
        xSemaphoreTake(task[S0]->m_Lock, portMAX_DELAY );
        S0 =  (stackitem)task[S0]->m_Db;
        xSemaphoreGive(task[USB_TASK]->m_Lock);
        failFlag=false;
    }
    Push= failFlag ;
}

prim FR_setTaskDb() {
	Sl(1);

    xSemaphoreTake(task[S0]->m_Lock, portMAX_DELAY );
	task[S0]->m_Db = (Small *)S0;
    xSemaphoreGive(task[USB_TASK]->m_Lock);
    Pop;
}

prim FR_getTaskBatching() {
	Sl(1);

	bool failFlag=true;
	uint32_t delay=100 ; // Milliseconds
	uint32_t taskId=0;
	void *bi=NULL;

	taskId=S0;
	Pop;

	if (validTask( taskId )) {
		if( task[taskId] != NULL) {
			if (xSemaphoreTake(task[taskId]->m_Lock, pdMS_TO_TICKS(delay)) == pdTRUE) {
				xSemaphoreGive(task[taskId]->m_Lock);
				Push=(stackitem)bi;
				failFlag=false;
			}
		}
	}
	Push= failFlag ;
}

prim FR_getTaskReady() {
	Sl(1);
	So(1);

	bool tr = taskReady((appTask_t) S0);

	S0= tr;

}

prim FR_writePin() {
	Sl(3);

	GPIO_PinState pinState= (GPIO_PinState) ((uint8_t)S0 & 0x01);
	uint16_t portPin = (uint16_t)S1;
	GPIO_TypeDef *portAddr = (GPIO_TypeDef*)S2;

	HAL_GPIO_WritePin(portAddr,portPin,pinState);
	Pop2;
	Pop;
}

prim FR_readPin() {
	Sl(2);
	So(1);

	uint16_t portPin = (uint16_t)S0;
	uint32_t portAddr = (uint32_t)S1;

	Pop;
	S0=(stackitem)HAL_GPIO_ReadPin((GPIO_TypeDef*) portAddr,portPin);
}

prim FR_setBacklight() {
	TIM_OC_InitTypeDef sConfigOC;

	Sl(1);

	sConfigOC.OCMode = TIM_OCMODE_PWM1;

	sConfigOC.Pulse = (uint32_t)S0;

	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4);

	S0=HAL_TIM_PWM_Start(&htim5,TIM_CHANNEL_4);
	Pop;
}

prim FR_stackHighWaterMark() {
    TaskHandle_t h;

    Sl(1);
    So(1);

    h=(TaskHandle_t)S0;

    S0 =  uxTaskGetStackHighWaterMark( h );

}

prim FR_qdepth() {
	Sl(1);
	Sl(1);

	QueueSetHandle_t q;
	UBaseType_t depth;

	uint8_t taskId = (uint8_t)S0;

	q = task[taskId]->m_Iam;

	depth = uxQueueMessagesWaiting( q );

	S0=depth;
}

prim FR_qspace() {
	Sl(1);
	Sl(1);

	QueueSetHandle_t q;
	UBaseType_t depth;

	uint8_t taskId = (uint8_t)S0;

	q = task[taskId]->m_Iam;

	depth = uxQueueSpacesAvailable( q );

	S0=depth;
}

prim FR_nice() {
	Sl(2);
	So(1);

	uint8_t taskId = (uint8_t)S1;
	int8_t priorityInc = (int8_t)S0;

	if (taskId < LAST_TASK) {

		int8_t cPriority= task[taskId]->m_Priority;

		cPriority += priorityInc;

		if( cPriority < 1) {
			cPriority = 1;
		} else if (cPriority >= (configMAX_PRIORITIES -1)) {
			cPriority = configMAX_PRIORITIES -1;
		}

		vTaskPrioritySet(task[taskId]->m_Handle, cPriority);
		task[taskId]->m_Priority = cPriority;
		Pop;
		S0=cPriority;
	} else {
		Pop;
		S0=-1;
	}

}

prim FR_doverPs() {

	int i=0;

	printf("\r\n");
    
	for(i = 0;i<(int) LAST_TASK; i++) {
		if(task[i] == 0) {
			printf("Task %3d: Not defined\r\n",i);
		} else {
//            UBaseType_t qFreeSpace = uxQueueMessagesWaiting(task[i]->m_Iam );
            UBaseType_t qFreeSpace = uxQueueSpacesAvailable(task[i]->m_Iam );
            
            printf("Task     : %02d\n",i);
            printf("Task ID  : 0x%08x\n",(unsigned int) task[i]->m_Handle);
            printf("\tQueue  : 0x%08x\n" ,(unsigned int)task[i]->m_Iam);
            printf("\tFree   : %2d\n",(int)qFreeSpace);
            printf("\tDb     : 0x%08x\n",(unsigned int)task[i]->m_Db);
            
            
            /*
			printf("Task %3d: Queue :0x%08x\n\tFree %d\n\tTask ID 0x%08x\n\tDB 0x%08x\n",
                   i, 
                   (unsigned int)task[i]->m_Iam,
                   (unsigned int)task[i]->m_Handle,
                   (unsigned int)qFreeSpace,
                   (unsigned int)task[i]->m_Db);
                   */
        }
    }
}

prim FR_ps() {
	char taskList[1024];
	int taskCount=0;
	extern uint32_t _eheap;

	volatile uint32_t top = (uint32_t) &_eheap;

	struct mallinfo memInfo;

	memInfo = mallinfo();

	taskCount = uxTaskGetNumberOfTasks();

	printf("\n\rNumber of Task:%6d\r\n",taskCount);

    printf("Free Space    :%6d\r\n", (int)(top-(int)sbrk(0)));

    printf("Arena         :%6d\r\n",memInfo.arena );

    printf("Free Heap     :%6d\r\n",memInfo.fordblks );

    printf("Used Heap     :%6d\r\n",memInfo.uordblks );

    printf("+--------------+-------+-------+-------|----+\r\n");
    printf("|Name          |State  |Pri    |HWM    |ID  |\r\n");
    printf("+--------------+-------+-------+-------|----+\r\n");

	VariantOS::TaskList(taskList);

	char *ptr;

	ptr=strtok(taskList,"\r");

	while(ptr) {

		printf("%s\r",ptr) ;

		ptr=strtok(NULL,"\r");
	}

}

prim FR_kill() {
	Sl(1);
	So(0);

	uint8_t taskId = S0;
	TaskHandle_t t;
	if (taskId < LAST_TASK) {
		t=task[taskId]->m_Handle;
		if(t != 0) {
			vTaskDelete(t);
		}
	}
}

prim FR_suspend() {
	Sl(1);
	So(0);

	uint8_t taskId = S0;
	TaskHandle_t t;
	if (taskId < LAST_TASK) {
		t=task[taskId]->m_Handle;
		if(t != 0) {
			vTaskSuspend(t);
		}
	}
}

prim FR_resume() {
	Sl(1);
	So(0);

	uint8_t taskId = S0;
	TaskHandle_t t;
	if (taskId < LAST_TASK) {
		t=task[taskId]->m_Handle;
		if(t != 0) {
			vTaskResume(t);
		}
	}
}

prim FR_getTaskHandle() {

    uint8_t idx;

    idx=S0;

    S0=(stackitem)task[idx]->m_Handle ;
}

prim FR_getTaskState() {
    TaskHandle_t h;

    Sl(1);
    So(1);

    h=(TaskHandle_t)S0;

    S0 =  eTaskGetState( h );

}

prim FR_rxXmodem() {
    //xmodemReceive(BKPSRAM_BASE,2048); // ANT EDIT - Have removed xmodem code
}

#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"

prim FR_getTime() {
	Sl(0);
	So(0);

	//volatile HAL_StatusTypeDef rc;


	RTC_TimeTypeDef now;
	RTC_DateTypeDef dateNow;

	//volatile uint32_t data;

	HAL_RTC_GetTime(&hrtc,&now,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc,&dateNow, RTC_FORMAT_BIN );

	now.DayLightSaving = 0;

//	HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format
	Push= now.Hours;
	Push= now.Minutes;
	Push= now.Seconds;
}

prim FR_getBR() {
	Sl(1);
	Sl(0);

	uint32_t reg=S0;
	uint32_t v = HAL_RTCEx_BKUPRead(&hrtc, reg);

	S0=v;
}


prim FR_setBR() {
	Sl(2);
	Sl(0);

	uint32_t reg=S0;
	uint32_t v=S1;

	HAL_RTCEx_BKUPWrite(&hrtc,reg,v);

}

prim FR_setTime() {
	Sl(3);
	So(0);
	RTC_TimeTypeDef now;
	RTC_DateTypeDef dateNow;

	HAL_RTC_GetTime(&hrtc,&now,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc,&dateNow, RTC_FORMAT_BIN );

	now.Seconds = (uint8_t)S0;
	now.Minutes = (uint8_t)S1;
	now.Hours   = (uint8_t)S2;

//	extern RTC_HandleTypeDef hrtc;

	now.DayLightSaving = 0;

	HAL_RTC_SetTime(&hrtc, &now, RTC_FORMAT_BIN);
	Npop(3);

}
#endif // FREERTOS

#ifdef PUBSUB
/*
 * Name: alloc-mesg
 * Stack: -- <message ptr>
 * 
 * Description:
 *   Allocate space, properly aligned for a message.
 */
prim FR_allocMsg() {
    MsgCmdPacketType *out;
    int sze = sizeof(MsgCmdPacketType);
    
    
    out = (MsgCmdPacketType *) calloc(1, sze);
    
    Push=(stackitem)out;
}
/*
 * Name: MESSAGE@
 * Stack: <from> <timeout> <message ptr>
 *
 * Description:
 *  Receive a message, waiting for a specified number of milli seconds, and
 *  place the recieved message at ptr.
 */
prim FR_getMessage() {
#ifdef FREERTOS
//	extern struct taskData *task[LAST_TASK];
	BaseType_t rc;
	uint32_t timeout;
	volatile QueueHandle_t qh;
    MsgCmdPacketType*out;
    bool failFlag=true;

	Sl(3);
	So(1);

    out = (MsgCmdPacketType *)S0;
	timeout=S1;
    qh=(QueueHandle_t)S2;

    rc = xQueueReceive( qh, out, pdMS_TO_TICKS(timeout));

	if( rc != pdPASS ) {
		memset( out, 0, sizeof(MsgCmdPacketType) );
	    failFlag=true;
	} else {
	    failFlag=false;
	}
    Pop2;

	S0 =  (stackitem)failFlag;
#endif

#ifdef LINUX
	uint32_t timeout;
	char *from;
	struct mq_attr attr;
	int rc=0;

	MsgCmdPacketType *out;
	Sl(3);
	So(1);

	out = S0;
	timeout=S1;
	from = S2;

	mqd_t mq=mq_open(from, O_RDONLY);
	if((mqd_t)-1 == mq) {
		perror("MESSAGE@");
	}
	rc = mq_getattr(mq,&attr);

	int len = mq_receive(mq,out,attr.mq_msgsize, NULL);
	Npop(3);

	if( len <0)  {
		Push=-1;
	} else {
		Push=0;
	}

#endif

}
// Set fields in a message.
//
// <address> <value> -- <address>
prim FR_setSender() {
    Sl(2);
    MsgCmdPacketType *msg;

    QueueHandle_t value;
    value=(QueueHandle_t)S0;

    msg = (MsgCmdPacketType *)S1;
    msg->m_Sender = value;

    Pop2;
}

// Get sender
// <address> -- <sender>
prim FR_getSender() {
    MsgCmdPacketType *msg;

    msg = (MsgCmdPacketType *)S0;
    S0 =  (stackitem)msg->m_Sender;

}

// <struct address> <string> -- <struct address>
//
prim FR_setCmd() {
	Sl(2);
	So(1);

    MsgCmdPacketType *msg;
    MsgCmdType cmd;

    cmd = (MsgCmdType)S0;
    msg = (MsgCmdPacketType *)S1;

    msg->m_Payload.m_Message.m_Cmd=cmd;

    Pop2;
}

prim FR_getCmd() {
	Sl(1);
	So(1);

    MsgCmdPacketType *msg;
//    char *cmd;

    msg = (MsgCmdPacketType *)S0;

    S0 =  (stackitem)msg->m_Payload.m_Message.m_Cmd;
    Pop;
}

prim FR_setKey() {
	Sl(2);
	So(0);

    MsgCmdPacketType*msg;
    char *key;

    key = (char *)S0;
    msg = (MsgCmdPacketType *)S1;

    Pop2;

    strncpy(msg->m_Payload.m_Message.m_Key, key, TASK_MSG_MAX_KEY);
}

prim FR_getKey() {
	Sl(1);
	So(1);

    MsgCmdPacketType *msg;

    msg = (MsgCmdPacketType *)S0;

    S0=(stackitem)msg->m_Payload.m_Message.m_Key;
    
    Pop;
}

prim FR_setValue() {
	Sl(2);
	So(0);

    MsgCmdPacketType *msg;
    char *value;

    value = (char *)S0;
    msg = (MsgCmdPacketType *)S1;

    strncpy(msg->m_Payload.m_Message.m_Value, value, TASK_MSG_MAX_VALUE);

    Pop2;
}

// GET-VALUE

prim FR_getValue() {
	Sl(1);
	So(1);

    MsgCmdPacketType *msg;

    msg = (MsgCmdPacketType *)S0;
    S0=(stackitem)msg->m_Payload.m_Message.m_Value;
}


prim FR_setFieldCnt() {
	Sl(2);
	So(0);

	MsgCmdPacketType *msg;
	uint8_t fields;

	fields = (uint8_t)S0;
	msg = (MsgCmdPacketType *)S1;


	msg->m_Payload.m_Message.m_Fields = fields;

	Pop2;

}

prim FR_setChanges() {
	Sl(2);
	So(0);

	MsgCmdPacketType *msg;
    bool flag;
    
    flag = (bool)S0;
	msg = (MsgCmdPacketType *)S1;
    
	msg->m_Payload.m_Message.m_ChangesOnly = flag;
    
    Pop2;
}

prim FR_getFieldCnt() {
	Sl(1);
	So(1);

	MsgCmdPacketType *msg;

	msg=(MsgCmdPacketType *)S0;

	S0=(stackitem)msg->m_Payload.m_Message.m_Fields;
}
// <msg pointer> <sender>

prim FR_mkmsgDone() {
    MsgCmdPacketType *msg;
    Sl(2);

#ifdef FREERTOS
    appTask_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif
    msg=(MsgCmdPacketType *)S1;

#ifdef FREERTOS
    sender=(appTask_t) S0;
#endif
    mkMsg(sender, msg, EMsgCmdDone, NULL, NULL);

    Pop2;
}

prim FR_displayMsg() {
    Sl(1);
    So(0);
    
    MsgCmdPacketType *msg=(MsgCmdPacketType *)S0;
    
    printf("Sender      : %08x\n", (unsigned int)msg->m_Sender);
    printf("Task Id     : %08x\n", (unsigned int)msg->m_TaskId);
	printf("Field Count : %02d\n", msg->m_Payload.m_Message.m_Fields);
	printf("Command     : %02d\n", msg->m_Payload.m_Message.m_Cmd);
	printf("Key         : %s\n", msg->m_Payload.m_Message.m_Key);
	printf("Value       : %s\n", msg->m_Payload.m_Message.m_Value);
	printf("Changes Only: %d\n", msg->m_Payload.m_Message.m_ChangesOnly);
    
    Pop;
}

prim FR_msgErase() {
    Sl(1);
    So(0);
    
//    MsgCmdPacketType *msg=(MsgCmdPacketType *)S0;
    
    memset((void *)S0,0,sizeof(MsgCmdPacketType));
    Pop;
}

//
// populate a GET message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgGet() {
    MsgCmdPacketType *msg;
    char *key;

    Sl(3);
#ifdef FREERTOS
    appTask_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
    sender=(appTask_t) S1;
    msg=(MsgCmdPacketType *)S2;

    mkMsg(sender, msg, EMsgCmdGet, key, NULL);
    Pop;
    Pop2;
}
//
// populate a SET message
// Stack <msg pointer> <key> <value> --
//
prim FR_mkmsgSet() {
    MsgCmdPacketType *msg;
    char *key;
    char *value;

    Sl(3);

#ifdef FREERTOS
    appTask_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    value=(char *)S0;
    key=(char *)S1;
    msg=(MsgCmdPacketType *)S2;

#ifdef FREERTOS
    sender=APPTASK_NONE;
#endif

    mkMsg(sender, msg, EMsgCmdSet, key, value);
    Pop;
    Pop2;
}


//
// populate a SUB message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgSub() {
    MsgCmdPacketType *msg;
    char *key;

#ifdef FREERTOS
    appTask_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
#ifdef LINUX
    strncpy(msg->sender,(char *)S1, SENDER_SIZE);
#endif

#ifdef FREERTOS
    sender=(appTask_t) S1;
#endif
    msg=(MsgCmdPacketType *)S2;

    mkMsg(sender, msg, EMsgCmdSub, key, NULL);
    Pop;
    Pop2;
}
//
// populate an UNSUB message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgUnsub() {
    MsgCmdPacketType *msg;
    char *key;

#ifdef FREERTOS
    appTask_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
#ifdef LINUX
    strncpy(msg->sender,(char *)S1, SENDER_SIZE);
#endif

#ifdef FREERTOS
    sender=(appTask_t) S1;
#endif
    msg=(MsgCmdPacketType *)S2;

    mkMsg(sender, msg, EMsgCmdUnsub, key, NULL);
    Pop;
    Pop2;
}

// TODO FreeRTOS Only and temporary at that
// Ultimately these will become open close etc
//
/*
#ifdef FREERTOS
// <msg ptr> <sender> filename mode --
//
prim FR_mkmsgOpen() {
	Sl(4);
	So(0);

	if(elcofs_is_mounted()) {
	MsgCmdPacketType *out = (MsgCmdPacketType *) S3;
    QueueHandle_t sender = (QueueHandle_t) S2;
	char *fname = (char*) S1;
	char *mode = (char*) S0;

		memset(out,0,sizeof(MsgCmdPacketType));

		strcpy(out->payload.message.cmd, "OPEN");
		strcpy(out->payload.message.key, fname);
		strcpy(out->payload.message.value, mode);

		out->payload.message.fields = 3;
		out->sender = sender;
	}
	Npop(4);
}
#endif
*/

// Stack: errnum <error message>
//
prim FR_logMessage() {
	Sl(2);
	So(0);

	//BaseType_t status = errQUEUE_FULL;
	QueueHandle_t dest;
	//char *note;

	if(elcofs_is_mounted()) {
		loggerMessage outMessage;
		char *msg = (char *)S0;
		uint8_t err = (uint8_t)S1;

		dest = task[LOGGER_TASK]->m_Iam;
		outMessage.error = err;
		outMessage.cmd = LOG_ENTRY;

		memset(outMessage.note,0,sizeof(outMessage.note ));

		int len = min(strlen(msg), sizeof(outMessage.note));
		strncpy(outMessage.note,msg, len);

		if( dest != 0) {
			xQueueSendToBack(dest,&outMessage, portMAX_DELAY);
		}
	}
	Pop2;
}

prim FR_logClear() {
	loggerMessage outMessage;

	if(elcofs_is_mounted()) {
		memset(&outMessage,0,sizeof(loggerMessage));

		outMessage.cmd = LOG_CLEAR;

		QueueHandle_t dest = task[LOGGER_TASK]->m_Iam;

		xQueueSendToBack(dest,&outMessage, portMAX_DELAY);
	}
}

prim FR_logDump() {
	loggerMessage outMessage;

	if(elcofs_is_mounted()) {
		memset(&outMessage,0,sizeof(loggerMessage));

		outMessage.cmd = LOG_PP;

		QueueHandle_t dest = task[LOGGER_TASK]->m_Iam;

		xQueueSendToBack(dest,&outMessage, portMAX_DELAY);
	}
}

prim FR_putMessage() {
	MsgCmdPacketType *out;
	Sl(2);
	So(1);
#ifdef FREERTOS
	volatile QueueHandle_t dest;
	//BaseType_t status = errQUEUE_FULL;
	bool rc=true;;


	dest = (QueueHandle_t ) S1;
	out = (MsgCmdPacketType *)S0;

	if ( (dest != NULL) && (out != NULL)) {
		xQueueSendToBack(dest,out, portMAX_DELAY);

		if( rc == pdPASS) {
			rc=false;
		} else {
			rc=true;
		}
	}
	Pop2;
//	S0=rc;

#endif
#ifdef LINUX
	char *dest = (char *)S1;
    int rc=0;

	out = (MsgCmdPacketType *)S0;

	mqd_t mq=mq_open(dest,O_WRONLY);
	if ((mqd_t) -1 == mq) {
		perror("MESSAGE! mq_open");
		exit(2);
	}
    rc = mq_send(mq,out,sizeof(MsgCmdPacketType),(size_t)NULL);

    mq_close( mq ) ;
    Pop2;
#endif
	Push=(stackitem)rc;
}

#ifdef LINUX
#ifdef PTHREAD
extern pthread_mutex_t lock;
prim PS_comms() {

	pthread_mutex_unlock(&lock);
    pthread_yield();
    sleep(1);
}
#endif
#endif

prim FR_mkdb() {
	So(1);

	Small *db = new Small(APPTASK_NONE);

	Push = (stackitem) db;

}

prim FR_dumpDb() {
	So(1);
	Sl(0);

	Small *db=(Small*)S0;
	if( db != NULL) {
		db->printDb();
	}

	Pop;
}

prim FR_dumpDbIni() {
	Sl(2);
	So(0);

	char *path = (char *)S0;
	Small *db=(Small*)S1;

	if( db != NULL) {
//		dbIniDump(db, path);
		db->dumpIni(path);
	}

	Pop2;
}

prim FR_publish() {
	char *name;
	Small *db;
	bool rc;

	Sl(2);

	name=(char *)S0;
	db=(Small *)S1;

	rc=db->publish(name);

	Pop2;

	Push=rc;
}

prim FR_subCount() {
    int32_t cnt;
    char *key;
    Small *db;

    Sl(2);
    So(1);

    key=(char *)S0;
    db=(Small *)S1;

    cnt=(int32_t)db->getKeySubCount(key);
    Pop;
    S0=(int32_t)cnt;
}



prim FR_displayRecord() {
    nlist *rec;
    Sl(1);
    char localBuffer[80];

    rec=(nlist *)S0;
    printf("Name      : %s\n", rec->getName());
    printf("Value     : %s\n", (char *)rec->getDef());

    sprintf(localBuffer, "Published :" );

    if(rec->amIPublished()) {
        strcat(localBuffer,"True\n");
    } else {
        strcat(localBuffer,"False\n");
    }
    printf(localBuffer);
    Pop;
}

prim FR_addRecord() {
	Small *db;
	char *n;
	char *v;
	bool rc;

	Sl(3);
	So(1);

	v=(char *)S0;
	n=(char *)S1;
	db=(Small *)S2;

	rc = db->dbInstall(n,v);

	Npop(3);

	Push=rc;

}

prim FR_setReadOnly() {
	Small *db;
	char *n;
	bool flag;
	bool rc;

	Sl(3);
	So(1);

	flag=(bool)S0;
	n=(char *)S1;
	db=(Small *)S2;

	rc = db->readOnly(n,flag);

	Npop(3);

	Push=rc;
}

prim FR_addIniSection() {
	char *section;
	Small *db;
	bool failFlag=true;

	section=(char *)S0;
	db=(Small *)S1;

	failFlag = db->dbInstallSection(section);
	Pop;

	S0=(stackitem)failFlag;
}
prim FR_addIniRecord() {
	Small *db;
	char *name;
	char *value;
	char *section;
	bool rc;

	Sl(4);
	So(1);

	value=(char *)S0;
	name=(char *)S1;
	section=(char *)S2;
	db=(Small *)S3;

	rc = db->dbInstallIni(section, name, value);
	Npop(3);

	S0=rc;
}

prim FR_lookupIniRecordBool() {
	Small *db;
	char *name;

	bool defaultState;
	bool rc;

	defaultState=(bool)S0;
	name=(char *)S1;
	db=(Small *)S2;

	rc = db->dbLookupBoolIni(name,defaultState);

	Pop2;

	S0=rc;
}


//    char *dbLookup(struct Small *db, const char *n);
// Stack: db <key> -- <nlist>
prim FR_lookupRecord() {
	Small *db;
	char *key;
	nlist *rec;

	Sl(2);
	So(1);

	key=(char *)S0;
	db=(Small *)S1;
    Pop2;

	rec = db->dbLookupRec(key);

    Push = (stackitem)rec;

    if( rec == NULL ) {
        Push = true;
    } else {
        Push = false;
    }
}

prim FR_lookup() {

	Small *db;
	char *key;
	const char *value;

	Sl(2);
	So(1);

	key=(char *)S0;
	db=(Small *)S1;

	value = db->dbLookup(key);
	Pop2;

	Push = (stackitem)value;

	if( value == NULL) {
		Push = true;
	} else {
		Push = false;
	}

}

prim FR_findFirst() {
	Sl(2);
	So(1);

	Small *db;
	char *key;
//	char *value;

	key=(char *)S0;
	db=(Small *)S1;

	nlist *ptr = db->findFirst(key);

	Pop;
	S0=(stackitem) ptr;
}

prim FR_findNext() {
	Sl(2);
	So(1);

	nlist *ptr;
	nlist *out;

	char *name=(char *) S0;
	ptr=(nlist *)S1;
	Small *db=(Small *)S2;

	out = db->findNext(ptr, name);
	Pop;

	S0 = (stackitem)out;
}
#endif

/*  ALLOC  --  Allocate memory and error upon exhaustion.  */

static char *alloc(unsigned int size) {
    char *cp = (char *)malloc(size);

    if (cp == NULL) {
        
        printf("\n\nOut of memory!  %u bytes requested.\n", size);
        
        abort();
    }
    return cp;
}

/*  UCASE  --  Force letters in string to upper case.  */

static void ucase(char *c) {
    char ch;

    while ((ch = *c) != EOS) {
        if (islower(ch))
            *c = toupper(ch);
        c++;
    }
}

/*  TOKEN  --  Scan a token and return its type.  */

static int token( char **cp) {
    char *sp = *cp;

    while (True) {
        char *tp = tokbuf;
        int tl = 0;
        Boolean istring = False, rstring = False;

        if (atl_comment) {
            while (*sp != ')') {
                if (*sp == EOS) {
                    *cp = sp;
                    return TokNull;
                }
                sp++;
            }
            sp++;
            atl_comment = Falsity;
        }

        while (isspace(*sp))		  /* Skip leading blanks */
            sp++;

        if (*sp == '"') {                 /* Is this a string ? */

            /* Assemble string token. */

            sp++;
            while (True) {
                char c = *sp++;

                if (c == '"') {
                    sp++;
                    *tp++ = EOS;
                    break;
                } else if (c == EOS) {
                    rstring = True;
                    *tp++ = EOS;
                    break;
                }
                if (c == '\\') {
                    c = *sp++;
                    if (c == EOS) {
                        rstring = True;
                        break;
                    }
                    switch (c) {
                        case 'b':
                            c = '\b';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        default:
                            break;
                    }
                }
                if (tl < (int) (sizeof tokbuf) - 1) {
                    *tp++ = c;
                    tl++;
                } else {
                    rstring = True;
                }
            }
            istring = True;
        } else {

            /* Scan the next raw token */

            while (True) {
                char c = *sp++;

                if (c == EOS || isspace(c)) {
                    *tp++ = EOS;
                    break;
                }
                if (tl < (int) (sizeof tokbuf) - 1) {
                    *tp++ = c;
                    tl++;
                }
            }
        }
        *cp = --sp;			  /* Store end of scan pointer */

        if (istring) {
            if (rstring) {
                printf("\nRunaway string: %s\n", tokbuf);
                evalstat = ATL_RUNSTRING;
                return TokNull;
            }
            return TokString;
        }

        if (tokbuf[0] == EOS)
            return TokNull;

        /* See if token is a comment to end of line character.	If so, discard
           the rest of the line and return null for this token request. */

        if (strcmp(tokbuf, "\\") == 0) {
            while (*sp != EOS)
                sp++;
            *cp = sp;
            return TokNull;
        }

        /* See if this token is a comment open delimiter.  If so, set to
           ignore all characters until the matching comment close delimiter. */

        if (strcmp(tokbuf, "(") == 0) {
            while (*sp != EOS) {
                if (*sp == ')')
                    break;
                sp++;
            }
            if (*sp == ')') {
                sp++;
                continue;
            }
            atl_comment = Truth;
            *cp = sp;
            return TokNull;
        }

        /* See if the token is a number. */

        if (isdigit(tokbuf[0]) || tokbuf[0] == '-') {
            //char tc;
            char *tcp;

#ifdef USE_SSCANF
            if (sscanf(tokbuf, "%li%c", &tokint, &tc) == 1)
                return TokInt;
#else
            tokint = strtoul(tokbuf, &tcp, 0);
            if (*tcp == 0) {
                return TokInt;
            }
#endif
#ifdef REAL
            if (sscanf(tokbuf, "%lf%c", &tokreal, &tc) == 1)
                return TokReal;
#endif
        }
        return TokWord;
    }
}

/*  LOOKUP  --	Look up token in the dictionary.  */

static dictword *lookup(char *tkname)
{
    dictword *dw = dict;

    ucase(tkname);		      /* Force name to upper case */
    while (dw != NULL) {
        if (!(dw->wname[0] & WORDHIDDEN) &&
                (strcmp(dw->wname + 1, tkname) == 0)) {
#ifdef WORDSUSED
            *(dw->wname) |= WORDUSED; /* Mark this word used */
#endif
            break;
        }
        dw = dw->wnext;
    }
    return dw;
}

/* Gag me with a spoon!  Does no compiler but Turbo support
#if defined(x) || defined(y) ?? */
#ifdef EXPORT
#define FgetspNeeded
#endif
#ifdef FILEIO
#ifndef FgetspNeeded
#define FgetspNeeded
#endif
#endif
#include <unistd.h>

#ifdef FgetspNeeded

/*  ATL_FGETSP	--  Portable database version of FGETS.  This reads the
    next line into a buffer a la fgets().  A line is
    delimited by either a carriage return or a line
    feed, optionally followed by the other character
    of the pair.  The string is always null
    terminated, and limited to the length specified - 1
    (excess characters on the line are discarded.
    The string is returned, or NULL if end of file is
    encountered and no characters were stored.	No end
    of line character is stored in the string buffer.
    */
// #if defined(FREERTOS) && defined(ELCOFS)
Exported char *atl_fgetsp(char *s, int n, int stream) {

	int rc=0;
	int ch=0;

	int idx=0;

    bool debug=false;

//    debug=(CoreDebug->DHCSR & 1) ? true : false ;
//    debug = debug && semihosting;

	while(True) {
		if(debug) {
			rc = read(stream,&ch,1);
		} else {
			rc = elcofs_read(stream,&ch,1);
		}

		if(rc < 0) {
			return NULL;
		}
		if(rc == 0) {
			if(idx == 0) {
				return NULL;
			}
			break;
		}
		if( ch == '\r') {
			break;
		}
		if( ch == '\n') {
			break;
		}
		if(idx < (n-1)) {
			s[idx++] = (ch & 0xff);
		}
	}
	s[idx]='\0';
	return s;
}
// #endif

#ifdef LINUX
Exported char *atl_fgetsp(s, n, stream)
    char *s;
    int n;
    FILE *stream;
{
    int i = 0, ch;

    while (True) {
        ch = getc(stream);
        if (ch == EOF) {
            if (i == 0)
                return NULL;
            break;
        }
        if (ch == '\r') {
            ch = getc(stream);
            if (ch != '\n')
                V ungetc(ch, stream);
            break;
        }
        if (ch == '\n') {
            ch = getc(stream);
            if (ch != '\r')
                V ungetc(ch, stream);
            break;
        }
        if (i < (n - 1))
            s[i++] = ch;
    }
    s[i] = EOS;
    return s;
}
#endif // LINUX
#endif /* FgetspNeeded */

/*  ATL_MEMSTAT  --  Print memory usage summary.  */

#ifdef MEMSTAT
// TODO Comments reduce size to below 255 bytes, now make it print into
// outBuffer
// #warning MEMSTAT
void atl_memstat() {
    static char fmt[] = "   %-12s %6ld    %6ld    %6ld       %3ld %%\r\n";
     printf("  Memory Area     usage     used    allocated   in use\r\n");

     printf(fmt, "Stack",
            ((long) (stk - stack)),
            ((long) (stackmax - stack)),
            atl_stklen,
            (100L * (stk - stack)) / atl_stklen);

	 printf(fmt, "Return stack",
            ((long) (rstk - rstack)),
            ((long) (rstackmax - rstack)),
            atl_rstklen,
            (100L * (rstk - rstack)) / atl_rstklen);
	 printf(fmt, "Heap",
            ((long) (hptr - heap)),
            ((long) (heapmax - heap)),
            atl_heaplen,
            (100L * (hptr - heap)) / atl_heaplen);
}
#endif /* MEMSTAT */

/*  Primitive implementing functions.  */

/*  ENTER  --  Enter word in dictionary.  Given token for word's
    name and initial values for its attributes, returns
    the newly-allocated dictionary item. */

static void enter(char *tkname)
{
    /* Allocate name buffer */
    createword->wname = alloc(((unsigned int) strlen(tkname) + 2));
    createword->wname[0] = 0;	      /* Clear flags */
    V strcpy(createword->wname + 1, tkname); /* Copy token to name buffer */
    createword->wnext = dict;	      /* Chain rest of dictionary to word */
    dict = createword;		      /* Put word at head of dictionary */
}

#ifdef Keyhit

/*  KBQUIT  --	If this system allows detecting key presses, handle
    the pause, resume, and quit protocol for the word
    listing facilities.  */

// TODO, not sure what to do here
static Boolean kbquit() {

	Boolean rc=False;
/*
	if(rxReady(console)) {
		(void) rxByte(console );
		rc=True;
	}
	*/

	return rc;
	/*
    int key;

    if ((key = Keyhit()) != 0) {
        V printf("\nPress RETURN to stop, any other key to continue: ");
        while ((key = Keyhit()) == 0) ;
        if (key == '\r' || (key == '\n'))
            return True;
    }
    return False;
    */
}
#endif /* Keyhit */

/*  Primitive word definitions.  */

#ifdef NOMEMCHECK
#define Compiling
#else
#define Compiling if (state == Falsity) {notcomp(); return;}
#endif
#define Compconst(x) Ho(1); Hstore = (stackitem) (x)
#define Skipstring ip += *((char *) ip)

prim P_plus()			      /* Add two numbers */
{
    Sl(2);
    /* printf("PLUS %lx + %lx = %lx\n", S1, S0, (S1 + S0)); */
    S1 += S0;
    Pop;
}

prim P_minus()			      /* Subtract two numbers */
{
    Sl(2);
    S1 -= S0;
    Pop;
}

prim P_times()			      /* Multiply two numbers */
{
    Sl(2);
    S1 *= S0;
    Pop;
}

prim P_div()			      /* Divide two numbers */
{
    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    S1 /= S0;
    Pop;
}

prim P_mod()			      /* Take remainder */
{
    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    S1 %= S0;
    Pop;
}

prim P_divmod() 		      /* Compute quotient and remainder */
{
    stackitem quot;

    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    quot = S1 / S0;
    S1 %= S0;
    S0 =  quot;
}

prim P_min()			      /* Take minimum of stack top */
{
    Sl(2);
    S1 =  min(S1, S0);
    Pop;
}

prim P_max()			      /* Take maximum of stack top */
{
    Sl(2);
    S1 =  max(S1, S0);
    Pop;
}

prim P_neg()			      /* Negate top of stack */
{
    Sl(1);
    S0 =  - S0;
}

prim P_abs()			      /* Take absolute value of top of stack */
{
    Sl(1);
    S0 =  abs(S0);
}

prim P_equal()			      /* Test equality */
{
    Sl(2);
    S1 =  (S1 == S0) ? Truth : Falsity;
    Pop;
}

prim P_unequal()		      /* Test inequality */
{
    Sl(2);
    S1 =  (S1 != S0) ? Truth : Falsity;
    Pop;
}

prim P_gtr()			      /* Test greater than */
{
    Sl(2);
    S1 =  (S1 > S0) ? Truth : Falsity;
    Pop;
}

/* Test less than */
prim P_lss() {
    Sl(2);
    S1 =  (S1 < S0) ? Truth : Falsity;
    Pop;
}

prim P_geq()			      /* Test greater than or equal */
{
    Sl(2);
    S1 =  (S1 >= S0) ? Truth : Falsity;
    Pop;
}

prim P_leq()			      /* Test less than or equal */
{
    Sl(2);
    S1 =  (S1 <= S0) ? Truth : Falsity;
    Pop;
}

prim P_and()			      /* Logical and */
{
    Sl(2);
    /* printf("AND %lx & %lx = %lx\n", S1, S0, (S1 & S0)); */
    S1 &= S0;
    Pop;
}

prim P_or()			      /* Logical or */
{
    Sl(2);
    S1 |= S0;
    Pop;
}

prim P_xor()			      /* Logical xor */
{
    Sl(2);
    S1 ^= S0;
    Pop;
}

prim P_not()			      /* Logical negation */
{
    Sl(1);
    S0 =  ~S0;
}

prim P_shift()			      /* Shift:  value nbits -- value */
{
    Sl(1);
    S1 =  (S0 < 0) ? (((unsigned long) S1) >> (-S0)) :
        (((unsigned long) S1) <<   S0);
    Pop;
}

#ifdef SHORTCUTA

prim P_1plus()			      /* Add one */
{
    Sl(1);
    S0++;
}

prim P_2plus()			      /* Add two */
{
    Sl(1);
    S0 += 2;
}

prim P_1minus() 		      /* Subtract one */
{
    Sl(1);
    S0--;
}

prim P_2minus() 		      /* Subtract two */
{
    Sl(1);
    S0 -= 2;
}

prim P_2times() 		      /* Multiply by two */
{
    Sl(1);
    S0 *= 2;
}

prim P_2div()			      /* Divide by two */
{
    Sl(1);
    S0 /= 2;
}

#endif /* SHORTCUTA */

#ifdef SHORTCUTC

prim P_0equal() 		      /* Equal to zero ? */
{
    Sl(1);
    S0 =  (S0 == 0) ? Truth : Falsity;
}

prim P_0notequal()		      /* Not equal to zero ? */
{
    Sl(1);
    S0 =  (S0 != 0) ? Truth : Falsity;
}

prim P_0gtr()			      /* Greater than zero ? */
{
    Sl(1);
    S0 =  (S0 > 0) ? Truth : Falsity;
}

prim P_0lss()			      /* Less than zero ? */
{
    Sl(1);
    S0 =  (S0 < 0) ? Truth : Falsity;
}

#endif /* SHORTCUTC */

/*  Storage allocation (heap) primitives  */

prim P_here()			      /* Push current heap address */
{
    So(1);
    Push = (stackitem) hptr;
}

bool inFlash(void *ptr) {

    bool flag = ( ptr >= (void *)0x08000000 && ptr < (void *)0x080f0000) ? true : false ;

    return flag;
}

prim P_bang()			      /* Store value into address */
{
    Sl(2);


    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((stackitem *) S0) = S1;
    Pop2;
}

prim P_at()			      /* Fetch value from address */
{
    Sl(1);

#ifndef DEBUG
    bool flag = inFlash( (void *)S0);

    if(flag) {
        badpointer();
    }
#endif
    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 =  *((stackitem *) S0);
}

prim P_plusbang()		      /* Add value at specified address */
{
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((stackitem *) S0) += S1;
    Pop2;
}

prim P_allot()			      /* Allocate heap bytes */
{
    stackitem n;

    Sl(1);
    n = (S0 + (sizeof(stackitem) - 1)) / sizeof(stackitem);
    Pop;
    Ho(n);
    hptr += n;
}

prim P_comma()			      /* Store one item on heap */
{
    Sl(1);

//    if ( ath_safe_memory == Truth ) {
//        Hpc(S0);
//    }
    Hstore = S0;
    /*
    t1=S0;
    *hptr = t1;
    hptr++;
    */
    Pop;
}

prim P_cbang()			      /* Store byte value into address */
{
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((unsigned char *) S0) = S1;
    Pop2;
}
#ifdef ATH

prim ATH_mkBuffer() {
    /* Declare constant */
    uint8_t *hp = (uint8_t *) hptr;
    int size;
    uint8_t *t;

    Sl(1);

    size=S0;
    Pop;

    P_create(); 		      /* Create dictionary item */

    createword->wcode = P_con;	      /* Set code to constant push */

    Ho(1)

    size = ((size + (sizeof(stackitem) - 1 )) / sizeof(stackitem));  // no of items in units of stacksize
    size = size * sizeof(stackitem);        // Size in bytes

    t = hp+(4*sizeof(stackitem));		      /* Store constant value in body */

    Hstore = (stackitem)t;
    hptr+=size;

    memset(t,0,size);
}

prim ATH_wbang() {
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }

    uint16_t *ptr=(uint16_t *)S0;
    uint16_t data=S1;

    *ptr=(uint16_t)(data & 0xffff);

    Pop2;
}

/* Fetch byte value from address */
prim ATH_wat() {

    Sl(1);
    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 =  *((uint16_t *) S0);
}

prim ATH_16toCell() {
	stackitem v;

	v=(stackitem)(int16_t)S0 ;
	S0=v;


}
#endif

prim P_cat()			      /* Fetch byte value from address */
{
    Sl(1);
#ifndef DEBUG
    bool flag = inFlash( (void *)S0);

    if(flag) {
        badpointer();
    }
#endif

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 =  *((unsigned char *) S0);
}

prim P_ccomma() 		      /* Store one byte on heap */
{
    unsigned char *chp;

    Sl(1);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    chp = ((unsigned char *) hptr);
    *chp++ = S0;
    hptr = (stackitem *) chp;
    Pop;
}

prim P_cequal() 		      /* Align heap pointer after storing */
{				      /* a series of bytes. */
    stackitem n = (((stackitem) hptr) - ((stackitem) heap)) %
        (sizeof(stackitem));

    if (n != 0) {
        char *chp = ((char *) hptr);

        chp += sizeof(stackitem) - n;
        hptr = ((stackitem *) chp);
    }
}

/*  Variable and constant primitives  */

prim P_var()			      /* Push body address of current word */
{
    So(1);
    Push = (stackitem) (((stackitem *) curword) + Dictwordl);
}

prim P_create()	      /* Create new word */
{
    defpend = True;		      /* Set definition pending */
    Ho(Dictwordl);
    createword = (dictword *) hptr;   /* Develop address of word */
    createword->wname = NULL;	      /* Clear pointer to name string */
    createword->wcode = P_var;	      /* Store default code */
    hptr += Dictwordl;		      /* Allocate heap space for word */
}

prim P_forget() 		      /* Forget word */
{
    forgetpend = True;		      /* Mark forget pending */
}

prim P_variable()		      /* Declare variable */
{
    P_create(); 		      /* Create dictionary item */
    Ho(1);
    Hstore = 0; 		      /* Initial value = 0 */
}

prim P_con()			      /* Push value in body */
{
    So(1);
    Push = *(((stackitem *) curword) + Dictwordl);
}

prim P_constant()		      /* Declare constant */
{
    Sl(1);
    P_create(); 		      /* Create dictionary item */
    createword->wcode = P_con;	      /* Set code to constant push */
    Ho(1);
    Hstore = S0;		      /* Store constant value in body */
    Pop;
}

/*  Array primitives  */

#ifdef ARRAY
prim P_arraysub()		      /* Array subscript calculation */
{				      /* sub1 sub2 ... subn -- addr */
    int i, offset, esize, nsubs;
    stackitem *array;
    stackitem *isp;

    Sl(1);
    array = (((stackitem *) curword) + Dictwordl);
    Hpc(array);
    nsubs = *array++;		      /* Load number of subscripts */
    esize = *array++;		      /* Load element size */
#ifndef NOMEMCHECK
    isp = &S0;
    for (i = 0; i < nsubs; i++) {
        stackitem subn = *isp--;

        if (subn < 0 || subn >= array[i])
            trouble("Subscript out of range");
    }
#endif /* NOMEMCHECK */
    isp = &S0;
    offset = *isp;		      /* Load initial offset */
    for (i = 1; i < nsubs; i++)
        offset = (offset * (*(++array))) + *(--isp);
    Npop(nsubs - 1);
    /* Calculate subscripted address.  We start at the current word,
       advance to the body, skip two more words for the subscript count
       and the fundamental element size, then skip the subscript bounds
       words (as many as there are subscripts).  Then, finally, we
       can add the calculated offset into the array. */
    S0 =  (stackitem) (((char *) (((stackitem *) curword) +
                    Dictwordl + 2 + nsubs)) + (esize * offset));
}

prim P_array()			      /* Declare array */
{				      /* sub1 sub2 ... subn n esize -- array */
    int i, nsubs, asize = 1;
    stackitem *isp;

    Sl(2);
#ifndef NOMEMCHECK
    if (S0 <= 0)
        trouble("Bad array element size");
    if (S1 <= 0)
        trouble("Bad array subscript count");
#endif /* NOMEMCHECK */

    nsubs = S1; 		      /* Number of subscripts */
    Sl(nsubs + 2);		      /* Verify that dimensions are present */

    /* Calculate size of array as the product of the subscripts */

    asize = S0; 		      /* Fundamental element size */
    isp = &S2;
    for (i = 0; i < nsubs; i++) {
#ifndef NOMEMCHECK
        if (*isp <= 0)
            trouble("Bad array dimension");
#endif /* NOMEMCHECK */
        asize *= *isp--;
    }

    asize = (asize + (sizeof(stackitem) - 1)) / sizeof(stackitem);
    Ho(asize + nsubs + 2);	      /* Reserve space for array and header */
    P_create(); 		      /* Create variable */
    createword->wcode = P_arraysub;   /* Set method to subscript calculate */
    Hstore = nsubs;		      /* Header <- Number of subscripts */
    Hstore = S0;		      /* Header <- Fundamental element size */
    isp = &S2;
    for (i = 0; i < nsubs; i++) {     /* Header <- Store subscripts */
        Hstore = *isp--;
    }
    while (asize-- > 0) 	      /* Clear the array to zero */
        Hstore = 0;
    Npop(nsubs + 2);
}
#endif /* ARRAY */

/*  String primitives  */

#ifdef STRING

prim P_strlit() 		      /* Push address of string literal */
{
    So(1);
    Push = (stackitem) (((char *) ip) + 1);
#ifdef TRACE
    if (atl_trace) {
        printf("\"%s\" ", (((char *) ip) + 1)); // EMBEDDED
    }
#endif /* TRACE */
    Skipstring; 		      /* Advance IP past it */
}

prim P_string() 		      /* Create string buffer */
{
    Sl(1);
    Ho((S0 + 1 + sizeof(stackitem)) / sizeof(stackitem));
    P_create(); 		      /* Create variable */
    /* Allocate storage for string */
    hptr += (S0 + 1 + sizeof(stackitem)) / sizeof(stackitem);
    Pop;
}

// ATH, raw memory move.
//
prim P_move() {
    Sl(3);

    uint32_t len = S0 ;
    char *dest=(char *)S1;
    char *src =(char *)S2;

    memcpy( dest, src, len);
}

char* strsep( char** stringp, char* delim )
{
	char* result;

	if ((stringp == NULL) || (*stringp == NULL)) return NULL;

	result = *stringp;

	while (**stringp && !strchr( delim, **stringp )) ++*stringp;

	if (**stringp) *(*stringp)++ = '\0';
	else             *stringp    = NULL;

	return result;
}

// ATH find token
//
prim P_strsep() {
    uint8_t *buffer;
    uint8_t *res=NULL;
    uint8_t delim;

    Sl(2);
    So(1);

    delim=(uint8_t) S0;
    buffer=(uint8_t *)S1;

    res= (uint8_t*) strsep((char**)&buffer, (char*)&delim);
    Pop;
    S0=(stackitem)res;
}

prim P_strcpy() 		      /* Copy string to address on stack */
{
    Sl(2);
    // Hpc checks that the pointer is an address inside the heap.
    // This prevents you writing to random memory addresses.
    // However sometimes you want to write to memory that has been
    // allocated by malloc, or a hardware address.
    // NOTE Use with caution
    // TODO Should I set ath_safe_memory to Truth after use ?  this means 
    // every acces to memory outside of heap would have to be prefixed with
    // 0 memsafe
    //
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    V strcpy((char *) S0, (char *) S1);
    Pop2;
}

prim P_strcat() 		      /* Append string to address on stack */
{
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    V strcat((char *) S0, (char *) S1);
    Pop2;
}

prim P_strlen() 		      /* Take length of string on stack top */
{
    Sl(1);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
    }
    S0 =  strlen((char *) S0);
}

prim P_strcmp() 		      /* Compare top two strings on stack */
{
    int i;

    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    i = strcmp((char *) S1, (char *) S0);
    S1 =  (i == 0) ? 0L : ((i > 0) ? 1L : -1L);
    Pop;
}

prim P_strchar()		      /* Find character in string */
{
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    S1 =  (stackitem) strchr((char *) S1, *((char *) S0));
    Pop;
}

prim P_substr() 		      /* Extract and store substring */
{				      /* source start length/-1 dest -- */
    long sl, sn;
    char *ss, *sp, *se, *ds;

    Sl(4);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S3);
    }
    sl = strlen(ss = ((char *) S3));
    se = ss + sl;
    sp = ((char *) S3) + S2;
    if ((sn = S1) < 0)
        sn = 999999L;
    ds = (char *) S0;
    while (sn-- && (sp < se))
        *ds++ = *sp++;
    *ds++ = EOS;
    Npop(4);
}

prim P_strform()		      /* Format integer using sprintf() */
{                                     /* value "%ld" str -- */
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    
    V sprintf((char *) S0, (char *) S1, S2);  // NOT EMBEDDED
    
    Npop(3);
}

#ifdef REAL
prim P_fstrform()		      /* Format real using sprintf() */
{                                     /* rvalue "%6.2f" str -- */
    Sl(4);
    Hpc(S0);
    Hpc(S1);
    V sprintf((char *) S0, (char *) S1, REAL1);
    Npop(4);
}
#endif /* REAL */

prim P_strint() 		      /* String to integer */
{				      /* str -- endptr value */
    stackitem is;
    char *eptr;

    Sl(1);
    So(1);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
    }
    is = strtoul((char *) S0, &eptr, 0);
    S0 =  (stackitem) eptr;
    Push = is;
}

#ifdef REAL
prim P_strreal()		      /* String to real */
{				      /* str -- endptr value */
    int i;
    union {
        atl_real fs;
        stackitem fss[Realsize];
    } fsu;
    char *eptr;

    Sl(1);
    So(2);
    Hpc(S0);
    fsu.fs = strtod((char *) S0, &eptr);
    S0 =  (stackitem) eptr;
    for (i = 0; i < Realsize; i++) {
        Push = fsu.fss[i];
    }
}
#endif /* REAL */
#endif /* STRING */

/*  Floating point primitives  */

#ifdef REAL

prim P_flit()			      /* Push floating point literal */
{
    int i;

    So(Realsize);
#ifdef TRACE
    if (atl_trace) {
        atl_real tr;

        V memcpy((char *) &tr, (char *) ip, sizeof(atl_real));
        V printf("%g ", tr);
    }
#endif /* TRACE */
    for (i = 0; i < Realsize; i++) {
        Push = (stackitem) *ip++;
    }
}

prim P_fplus()			      /* Add floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 + REAL0);
    Realpop;
}

prim P_fminus() 		      /* Subtract floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 - REAL0);
    Realpop;
}

prim P_ftimes() 		      /* Multiply floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 * REAL0);
    Realpop;
}

prim P_fdiv()			      /* Divide floating point numbers */
{
    Sl(2 * Realsize);
#ifndef NOMEMCHECK
    if (REAL0 == 0.0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    SREAL1(REAL1 / REAL0);
    Realpop;
}

prim P_fmin()			      /* Minimum of top two floats */
{
    Sl(2 * Realsize);
    SREAL1(min(REAL1, REAL0));
    Realpop;
}

prim P_fmax()			      /* Maximum of top two floats */
{
    Sl(2 * Realsize);
    SREAL1(max(REAL1, REAL0));
    Realpop;
}

prim P_fneg()			      /* Negate top of stack */
{
    Sl(Realsize);
    SREAL0(- REAL0);
}

prim P_fabs()			      /* Absolute value of top of stack */
{
    Sl(Realsize);
    SREAL0(abs(REAL0));
}

prim P_fequal() 		      /* Test equality of top of stack */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 == REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_funequal()		      /* Test inequality of top of stack */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 != REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fgtr()			      /* Test greater than */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 > REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_flss()			      /* Test less than */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 < REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fgeq()			      /* Test greater than or equal */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 >= REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fleq()			      /* Test less than or equal */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 <= REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fdot()			      /* Print floating point top of stack */
{
    Sl(Realsize);
    
    V printf("%g ", REAL0);
    
    Realpop;
}

prim P_float()			      /* Convert integer to floating */
{
    atl_real r;

    Sl(1)
        So(Realsize - 1);
    r = S0;
    stk += Realsize - 1;
    SREAL0(r);
}

prim P_fix()			      /* Convert floating to integer */
{
    stackitem i;

    Sl(Realsize);
    i = (int) REAL0;
    Realpop;
    Push = i;
}

#ifdef MATH

#define Mathfunc(x) Sl(Realsize); SREAL0(x(REAL0))

prim P_acos()			      /* Arc cosine */
{
    Mathfunc(acos);
}

prim P_asin()			      /* Arc sine */
{
    Mathfunc(asin);
}

prim P_atan()			      /* Arc tangent */
{
    Mathfunc(atan);
}

prim P_atan2()			      /* Arc tangent:  y x -- atan */
{
    Sl(2 * Realsize);
    SREAL1(atan2(REAL1, REAL0));
    Realpop;
}

prim P_cos()			      /* Cosine */
{
    Mathfunc(cos);
}

prim P_exp()			      /* E ^ x */
{
    Mathfunc(exp);
}

prim P_log()			      /* Natural log */
{
    Mathfunc(log);
}

prim P_pow()			      /* X ^ Y */
{
    Sl(2 * Realsize);
    SREAL1(pow(REAL1, REAL0));
    Realpop;
}

prim P_sin()			      /* Sine */
{
    Mathfunc(sin);
}

prim P_sqrt()			      /* Square root */
{
    Mathfunc(sqrt);
}

prim P_tan()			      /* Tangent */
{
    Mathfunc(tan);
}
#undef Mathfunc
#endif /* MATH */
#endif /* REAL */

/*  Console I/O primitives  */

#ifdef CONIO

/* Print top of stack, pop it */
prim P_dot() {
    Sl(1);
//    stackitem top=S0;
    int top=S0;

#ifdef EMBEDDED
    switch(base) {
    case 10:
        printf("%d",top);
    	break;
    case 16:
        printf("%x",top);
    	break;
    default:
        printf("%d",top);
    	break;
    }
#endif

    Pop;
}

prim P_question()		      /* Print value at address */
{
    Sl(1);
    Hpc(S0);
    
//    sprintf(outBuffer,(base == 16 ? "%lX" : "%ld "), *((stackitem *) S0)); // EMBEDDED
    printf("Hello\n");
    Pop;
}

/* Carriage return */
prim P_cr() {
        printf("\r\n");
}

/* Print entire contents of stack */
prim P_dots() {
    stackitem *tsp;

//    printf(outBuffer,"Stack: ");  // EMBEDDED
    printf("Stack: ");  // EMBEDDED
        
    if (stk == stackbot) {
        printf("Empty.");  // EMBEDDED
    } else {
        for (tsp = stack; tsp < stk; tsp++) {
        	// TODO If you change the stack size change this
            printf((base == 16 ? "%lX " : "%ld "), *tsp); //  EMBEDDED
        }
}
}

prim P_dotquote()		      /* Print literal string that follows */
{
    Compiling;
    stringlit = True;		      /* Set string literal expected */
    Compconst(s_dotparen);	      /* Compile .( word */
}

prim P_dotparen()		      /* Print literal string that follows */
{
    if (ip == NULL) {		      /* If interpreting */
        stringlit = True;	      /* Set to print next string constant */
    } else {			      /* Otherwise, */
        /* print string literal in in-line code. */
    	memset(outBuffer,0,OUTBUFF_SIZE);
        printf("%s", ((char *) ip) + 1);  // EMBEDDED
        Skipstring;		      /* And advance IP past it */
    }
}
#ifdef EMBEDDED
prim ATH_semihosting() {
	So(1);
	Sl(0);

	uint32_t flag=(uint32_t)S0;
	Pop;

	semihosting=(flag != 0 ) ? true : false ;
}
#endif

// send the byte at TOS.
prim P_emit() {
	Sl(1);

#ifdef FREERTOS
//	usbTxByte(S0);
	printf("%c", (char)S0);
#endif
#ifdef LINUX
    putchar(S0);
#endif
	Pop;
}
/* Print string pointed to by stack */

prim P_type() {
    Sl(1);
    
    if( ath_safe_memory == Truth) {
    	Hpc(S0);
    }

    printf("%s", (char *) S0); // EMBEDDED
    
    Pop;
}

prim P_words()			      /* List words */
{
#ifndef Keyhit
    int key = 0;
#endif
    dictword *dw = dict;

    while (dw != NULL) {

#ifdef EMBEDDED
    	strcpy(outBuffer,"\r\n");
    	strcat(outBuffer,dw->wname+1);
//    	strcat(outBuffer,"\r\n");

//        sprintf(outBuffer,"\r\n%s", dw->wname + 1); // EMBEDDED
#endif
    printf(outBuffer);
        
        dw = dw->wnext;
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#else
        /* If this system can't trap keystrokes, just stop the WORDS
     NOT       listing after 20 words. */
        if (++key >= 200)
            break;
#endif
    }
    printf(outBuffer,"\n"); // EMBEDDED
}
#endif /* CONIO */

#ifdef FILEIO

prim P_file()			      /* Declare file */
{
    Ho(2);
    P_create(); 		      /* Create variable */
    Hstore = FileSent;		      /* Store file sentinel */
    Hstore = 0; 		      /* Mark file not open */
}

prim P_mkdir() {
	Sl(1);
	So(0);
	char *name;
	mode_t mode=0666; // By default make it globally accesible.

	name=(char *)S0;
	if( strlen(name) > 0) {
#ifdef LINUX
	S0=-1;	// Failed, unimplemented.
#endif

	S0 =  elcofs_mkdir((char *)name, mode);
	}
	Pop;
}

prim P_rmdir() {
	Sl(1);
	So(0);
	char *name;

	name=(char *)S0;

	if(strlen(name) > 0) {
#ifdef LINUX
	S0=-1;	// Failed, unimplemented.
#endif

	S0 =  elcofs_unlink((char *)name);
	}
	Pop;
}

prim P_fdAt() {
	Sl(1);
	So(1);

	int fd=FileD(S0);

	S0=fd;
}

prim ELCOFS_lsof() {
	elcofs_lsof();
}


/* Open file: fname fmodes fd -- flag */
prim P_fopen() {
    stackitem stat=-1;

    Sl(3);
    Hpc(S2);
    Hpc(S0);
    int flags=0;

    char *fname;

    fname=(char *)S2;

    Isfile(S0);
#ifdef LINUX
    FILE *fd = fopen((char *) S2, fopenmodes[S1]);
    if (fd == NULL) {
        stat = Falsity;
    } else {
        *(((stackitem *) S0) + 1) = (stackitem) fd;
        stat = Truth;
    }
#endif

#if defined(FREERTOS)
#if defined(ELCOFS)
    flags=(int)S1;

    int fd = elcofs_open(fname, flags);
    if( fd < 0) {
        stat = Truth;
    } else {
        *(((stackitem *) S0) + 1) = (stackitem) fd;
        stat = Falsity;
    }
#endif
#endif

    Pop2;
    S0 =  stat;
}

prim P_fclose() 		      /* Close file: fd -- */
{
    Sl(1);
    Hpc(S0);
#if 0
    Isfile(S0);
    Isopen(S0);
#ifdef LINUX
    V fclose(FileD(S0));
#endif

#if defined(FREERTOS)
#if defined(ELCOFS)
    int fd = *(((stackitem *) S0) + 1);
    V elcofs_close(fd);
#endif
#endif
    *(((stackitem *) S0) + 1) = (stackitem) NULL;
#endif

    int fd = *(((stackitem *) S0) + 1);
    V elcofs_close(fd);
    Pop;
}

prim P_fdelete()		      /* Delete file: fname -- flag */
{
    Sl(1);
    Hpc(S0);
#ifdef LINUX
    S0 =  (unlink((char *) S0) == 0) ? Truth : Falsity;
#endif

#ifndef LINUX
    S0 =  (elcofs_unlink((char *) S0) == 0) ? Truth : Falsity;
#endif
#endif
}

// Delete folder and everything below it.
// <filename pointer> -- failFalg
prim P_recursiveDelete() {
	Sl(1);
	So(1);

	char *name=NULL;
	bool failFlag=true;

	name =(char *) S0;

	if(!strcmp(name,"/")) {
		// Protect from deleting everything.
		failFlag = true;
	} else {
		elcofs_unlink(name);
		elcofs_mkdir(name, 066);
		failFlag = false;
	}

	S0 = failFlag;
}

prim P_frename() {
	Sl(2);
	So(0);

	char *oldPath;
	char *newPath;

	oldPath=(char *)S1;
	newPath = (char *)S0;

#ifndef LINUX
	V elcofs_rename(oldPath, newPath);
#endif
	Pop2;

}

prim P_fgetline()		      /* Get line: fd string -- flag */
{
    Sl(2);
    Hpc(S0);
    Isfile(S1);
    Isopen(S1);
    if (atl_fgetsp((char *) S0, 132, FileD(S1)) == NULL) {
        S1 =  Falsity;
    } else {
        S1 =  Truth;
    }
    Pop;
}

prim P_fputline()		      /* Put line: string fd -- flag */
{
    Sl(2);
    Hpc(S1);
    Isfile(S0);
    Isopen(S0);
#ifdef LINUX
    if (fputs((char *) S1, FileD(S0)) == EOF) {
        S1 =  Falsity;
    } else {
        S1 =  putc('\n', FileD(S0)) == EOF ? Falsity : Truth;
    }
#endif
    Pop;
}

/* File read: buf len fd -- length */
prim P_fread() {
    /* Was ------- File read: fd len buf -- length */
    Sl(3);
    Hpc(S0);

    int fd;
    char *buff;
    int len;

    fd = FileD(S0);
    buff = (char *)S2;
    len = (int)S1;
#ifdef LINUX
    Isfile(S0);
    Isopen(S0);
    S2 =  fread((char *) buff, 1, len, fd);
#endif
#if defined(FREERTOS)
#if defined(ELCOFS)
    S2 =  elcofs_read(fd, buff, len);
#endif
#endif
    Pop2;
}

/* File write: buf len fd -- length */
prim P_fwrite() {
    Sl(3);
    Hpc(S2);
    Isfile(S0);
#ifdef LINUX
    Isopen(S0);
    S2 =  fwrite((char *) S1, 1, ((int) S2), FileD(S0));
#endif

#if defined(FREERTOS)
#if defined(ELCOFS)
    S2 =  elcofs_write(FileD(S0), (char *)S2, (int)S1);
#endif
#endif
    Pop2;
}

#ifdef ELCOFS
prim ELCOFS_begin() {
	Sl(1);
	So(0);


	//int32_t fd = FileD(S0);

	Pop;

	// ATH 3 Jan 2018 Temporarily commented out.
	// elcofs_begin(fd);
}

prim ELCOFS_commit() {
	Sl(0);
	So(0);

	// ATH 3 Jan 2018 Temporarily commented out.
	// elcofs_commit();
}

prim ELCOFS_rollback() {

}

#endif

prim P_fgetc()			      /* File get character: fd -- char */
{
    Sl(1);
    Isfile(S0);
    Isopen(S0);
#ifdef LINUX
    S0 =  getc(FileD(S0));	      /* Returns -1 if EOF hit */
#endif
}

prim P_fputc()			      /* File put character: char fd -- stat */
{
    Sl(2);
    Isfile(S0);
    Isopen(S0);
#ifdef LINUX
    S1 =  putc((char) S1, FileD(S0));
#endif
    Pop;
}

prim P_ftell()			      /* Return file position:	fd -- pos */
{
    Sl(1);
    Isfile(S0);
#ifdef LINUX
    Isopen(S0);
    S0 =  (stackitem) ftell(FileD(S0));
#else
    S0 = (stackitem) elcofs_lseek(FileD(S0), 0, SEEK_CUR);
#endif
}

prim P_fseek()			      /* Seek file:  offset base fd -- */
{
    Sl(3);
    So(1);
    Isfile(S0);
    Isopen(S0);

    unsigned long roffset=0;
#ifdef LINUX
    V fseek(FileD(S0), (long) S2, (int) S1);
#else
    roffset = elcofs_lseek(FileD(S0), (long) S2, (int) S1);
#endif
    Npop(2);
    S0 =  roffset;
}

prim P_fsync() {
	Sl(1);
	So(1);

    Isfile(S0);
    Isopen(S0);

	S0=elcofs_fsync(FileD(S0));

}

prim P_access() {
	Sl(2);
	So(1);

	S1 =   elcofs_access((char *)S1, (int) S0);

	Pop;

}

prim FR_loadFile() {
	Sl(1);
	So(0);

	char *fname=(char*)S0;
	char buffer[132];
	bool runFlag=True;

	Pop;
	V elcofs_unlink(fname);

	int fd = elcofs_open(fname, ELCOFS_CREAT | ELCOFS_WRONLY);

	if (fd >= 0)
	{
		while(runFlag) {
			Push=(stackitem)buffer;
			Push=132;

			FR_consoleExpect();

			if(buffer[0] == '\\' ) {
				char *tmp = strtok(buffer, " ");
				tmp = strtok(NULL," \n");
				if(!strcmp(tmp,"EOF")) {
					runFlag=False;
				}
			} else {
				elcofs_write(fd, buffer, strlen(buffer));
			}
		}

		V elcofs_close(fd);
	}
}

prim FR_upgradeProbe() {
	Sl(1);

	ProbeUtils::Upgrade((char*)S0);

}

prim FR_BTgetSetting() {
    Sl(2);
    So(1);
    
    bool fail=true;
    
    char *buffer = (char *)S0;
    char *name = (char *)S1 ;
    
    fail=getBTSetting(name, buffer);
    
    Pop;
    S0 = fail;
}

prim FR_BTenterDataMode() {
    Sl(0);
    Sl(1);
    
    bool fail=enterDataMode( );
    
    Push=fail;
}

prim FR_BTexitDataMode() {
    Sl(1);
    Sl(1);
    
    int quietTime = (int) S0;
    
    bool fail=exitDataMode( quietTime);
    
    S0=(int)fail;
}



prim P_fload()			      /* Load source file:  fd -- evalstat */
{
    int estat;
    int fd;

    Sl(1);
    Isfile(S0);
    Isopen(S0);
    fd = FileD(S0);
    Pop;
    estat = atl_load(fd);
    So(1);
    Push = estat;
}

prim P_include() {
    //int estat;
    Sl(1);
#ifdef LINUX
    FILE *fd;

    fd = fopen((char *)S0, "r") ;
#endif
    char *fname = (char *)S0;
    int fd = elcofs_open(fname, ELCOFS_RDONLY);

    if(!fd) {
        perror("INCLUDE fopen");
        Pop;
        return;
    }
    atl_load(fd);
#ifdef LINUX
    fclose(fd);
#endif

    elcofs_close(fd);

    Pop;
}

prim P_qinclude() {
    Sl(1);
    So(0);
#ifdef LINUX
    FILE *fd;

    char* name=(char *)S0;
    fd = fopen((char *)S0, "r") ;
    if(access(name,F_OK) ==0 ) {
    	estat = atl_load(fd);
    	fclose(fd);
    }
#endif

    int fd=-1;
    char *fname = (char *)S0;

    if((CoreDebug->DHCSR & 1) && semihosting) {
    	char buffer[255];
    	char fileBuffer[4096];
    	int toFd=0;
    	sprintf(buffer,"./Src/platform/atlSource/%s", fname);
    	if(access(buffer,R_OK) == 0) {
    		fd = open(buffer, O_RDONLY);
    		if( fd ) {
    			int fLen = read(fd, fileBuffer,sizeof(fileBuffer));
    			if( fLen > 0) {
    				toFd = elcofs_open(fname, O_WRONLY);
    				if(toFd ){
    					elcofs_write(toFd,fileBuffer, fLen);
    					elcofs_close(toFd);
    				}
    			}
				close(fd);
				fd=-1;
    		}


    		fd = elcofs_open(fname, O_WRONLY);
    		atl_load(fd);
    		close(fd);
    	}

    } else {

    	if(elcofs_access(fname,0) == 0 ) {
    		fd = elcofs_open(fname,ELCOFS_O_RDONLY);
    		atl_load(fd);
    		elcofs_close(fd);
    	}
    }

    Pop;
}

#ifdef EVALUATE

prim P_evaluate()
{				      /* string -- status */
    int es = ATL_SNORM;
    atl_statemark mk;
    atl_int scomm = atl_comment;      /* Stack comment pending state */
    dictword **sip = ip;	      /* Stack instruction pointer */
    char *sinstr = instream;	      /* Stack input stream */
    char *estring;

    Sl(1);
    Hpc(S0);
    estring = (char *) S0;	      /* Get string to evaluate */
    Pop;			      /* Pop so it sees arguments below it */
    atl_mark(&mk);		      /* Mark in case of error */
    ip = NULL;			      /* Fool atl_eval into interp state */
    if ((es = atl_eval(estring)) != ATL_SNORM) {
        atl_unwind(&mk);
    }
    /* If there were no other errors, check for a runaway comment.  If
       we ended the file in comment-ignore mode, set the runaway comment
       error status and unwind the file.  */
    if ((es == ATL_SNORM) && (atl_comment != 0)) {
        es = ATL_RUNCOMM;
        atl_unwind(&mk);
    }
    atl_comment = scomm;	      /* Unstack comment pending status */
    ip = sip;			      /* Unstack instruction pointer */
    instream = sinstr;		      /* Unstack input stream */
    So(1);
    Push = es;			      /* Return eval status on top of stack */
}
#endif /* EVALUATE */

/*  Stack mechanics  */

prim P_depth()			      /* Push stack depth */
{
    stackitem s = stk - stack;

    So(1);
    Push = s;
}

prim P_clear()			      /* Clear stack */
{
    stk = stack;
}

prim P_dup()			      /* Duplicate top of stack */
{
    stackitem s;

    Sl(1);
    So(1);
    s = S0;
    Push = s;
}

prim P_drop()			      /* Drop top item on stack */
{
    Sl(1);
    Pop;
}

prim P_swap()			      /* Exchange two top items on stack */
{
    stackitem t;

    Sl(2);
    t = S1;
    S1 =  S0;
    S0 =  t;
}

prim P_nip() {
	S1=S0;
	Pop;
}

prim P_over()			      /* Push copy of next to top of stack */
{
    stackitem s;

    Sl(2);
    So(1);
    s = S1;
    Push = s;
}

prim P_pick()			      /* Copy indexed item from stack */
{
    Sl(2);
    S0 =  stk[-(2 + S0)];
}

prim P_rot()			      /* Rotate 3 top stack items */
{
    stackitem t;

    Sl(3);
    t = S0;
    S0 =  S2;
    S2 =  S1;
    S1 =  t;
}

prim P_minusrot()		      /* Reverse rotate 3 top stack items */
{
    stackitem t;

    Sl(3);
    t = S0;
    S0 =  S1;
    S1 =  S2;
    S2 =  t;
}

prim P_roll()			      /* Rotate N top stack items */
{
    stackitem i, j, t;

    Sl(1);
    i = S0;
    Pop;
    Sl(i + 1);
    t = stk[-(i + 1)];
    for (j = -(i + 1); j < -1; j++)
        stk[j] = stk[j + 1];
    S0 =  t;
}

prim P_tor()			      /* Transfer stack top to return stack */
{
    Rso(1);
    Sl(1);
    Rpush = (rstackitem) S0;
    Pop;
}

prim P_rfrom()			      /* Transfer return stack top to stack */
{
    Rsl(1);
    So(1);
    Push = (stackitem) R0;
    Rpop;
}

prim P_rfetch() 		      /* Fetch top item from return stack */
{
    Rsl(1);
    So(1);
    Push = (stackitem) R0;
}

#ifdef Macintosh
/* This file creates more than 32K of object code on the Mac, which causes
   MPW to barf.  So, we split it up into two code segments of <32K at this
   point. */
#pragma segment TOOLONG
#endif /* Macintosh */

/*  Double stack manipulation items  */

#ifdef DOUBLE

prim P_2dup()			      /* Duplicate stack top doubleword */
{
    stackitem s;

    Sl(2);
    So(2);
    s = S1;
    Push = s;
    s = S1;
    Push = s;
}

prim P_2drop()			      /* Drop top two items from stack */
{
    Sl(2);
    stk -= 2;
}

prim P_2swap()			      /* Swap top two double items on stack */
{
    stackitem t;

    Sl(4);
    t = S2;
    S2 =  S0;
    S0 =  t;
    t = S3;
    S3 = S1;
    S1 =  t;
}

prim P_2over()			      /* Extract second pair from stack */
{
    stackitem s;

    Sl(4);
    So(2);
    s = S3;
    Push = s;
    s = S3;
    Push = s;
}

prim P_2rot()			      /* Move third pair to top of stack */
{
    stackitem t1, t2;

    Sl(6);
    t2 = S5;
    t1 = S4;
    S5 = S3;
    S4 = S2;
    S3 = S1;
    S2 =  S0;
    S1 =  t2;
    S0 =  t1;
}

prim P_2variable()		      /* Declare double variable */
{
    P_create(); 		      /* Create dictionary item */
    Ho(2);
    Hstore = 0; 		      /* Initial value = 0... */
    Hstore = 0; 		      /* ...in both words */
}

prim P_2con()			      /* Push double value in body */
{
    So(2);
    Push = *(((stackitem *) curword) + Dictwordl);
    Push = *(((stackitem *) curword) + Dictwordl + 1);
}

prim P_2constant()		      /* Declare double word constant */
{
    Sl(1);
    P_create(); 		      /* Create dictionary item */
    createword->wcode = P_2con;       /* Set code to constant push */
    Ho(2);
    Hstore = S1;		      /* Store double word constant value */
    Hstore = S0;		      /* in the two words of body */
    Pop2;
}

prim P_2bang()			      /* Store double value into address */
{
    stackitem *sp;

    Sl(2);
    Hpc(S0);
    sp = (stackitem *) S0;
    *sp++ = S2;
    *sp = S1;
    Npop(3);
}

prim P_2at()			      /* Fetch double value from address */
{
    stackitem *sp;

    Sl(1);
    So(1);
    Hpc(S0);
    sp = (stackitem *) S0;
    S0 =  *sp++;
    Push = *sp;
}
#endif /* DOUBLE */

/*  Data transfer primitives  */

prim P_dolit()			      /* Push instruction stream literal */
{
    So(1);
#ifdef TRACE
    if (atl_trace) {
#ifdef EMBEDDED
        sprintf(outBuffer,"%ld ", (long) *ip); // NOT EMBEDDED
#endif
     printf("%s",outBuffer);
    }
#endif
    Push = (stackitem) *ip++;	      /* Push the next datum from the
                                         instruction stream. */
}

/*  Control flow primitives  */

prim P_nest()			      /* Invoke compiled word */
{
    Rso(1);
#ifdef WALKBACK
    *wbptr++ = curword; 	      /* Place word on walkback stack */
#endif
    Rpush = ip; 		      /* Push instruction pointer */
    ip = (((dictword **) curword) + Dictwordl);
}

prim P_exit()			      /* Return to top of return stack */
{
    Rsl(1);
#ifdef WALKBACK
    wbptr = (wbptr > wback) ? wbptr - 1 : wback;
#endif
    ip = R0;			      /* Set IP to top of return stack */
    Rpop;
}

prim P_branch() 		      /* Jump to in-line address */
{
    ip += (stackitem) *ip;	      /* Jump addresses are IP-relative */
}

prim P_qbranch()		      /* Conditional branch to in-line addr */
{
    Sl(1);
    if (S0 == 0)		      /* If flag is false */
        ip += (stackitem) *ip;	      /* then branch. */
    else			      /* Otherwise */
        ip++;			      /* skip the in-line address. */
    Pop;
}

prim P_if()			      /* Compile IF word */
{
    Compiling;
    Compconst(s_qbranch);	      /* Compile question branch */
    So(1);
    Push = (stackitem) hptr;	      /* Save backpatch address on stack */
    Compconst(0);		      /* Compile place-holder address cell */
}

prim P_else()			      /* Compile ELSE word */
{
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_branch);	      /* Compile branch around other clause */
    Compconst(0);		      /* Compile place-holder address cell */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get IF backpatch address */
    *bp = hptr - bp;
    S0 =  (stackitem) (hptr - 1);      /* Update backpatch for THEN */
}

prim P_then()			      /* Compile THEN word */
{
    stackitem *bp;

    Compiling;
    Sl(1);
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get IF/ELSE backpatch address */
    *bp = hptr - bp;
    Pop;
}

prim P_qdup()			      /* Duplicate if nonzero */
{
    Sl(1);
    if (S0 != 0) {
        stackitem s = S0;
        So(1);
        Push = s;
    }
}

prim P_begin()			      /* Compile BEGIN */
{
    Compiling;
    So(1);
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_until()			      /* Compile UNTIL */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_qbranch);	      /* Compile question branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    Pop;
}

prim P_again()			      /* Compile AGAIN */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Compconst(s_branch);	      /* Compile unconditional branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    Pop;
}

prim P_while()			      /* Compile WHILE */
{
    Compiling;
    So(1);
    Compconst(s_qbranch);	      /* Compile question branch */
    Compconst(0);		      /* Compile place-holder address cell */
    Push = (stackitem) (hptr - 1);    /* Queue backpatch for REPEAT */
}

prim P_repeat() 		      /* Compile REPEAT */
{
    stackitem off;
    stackitem *bp1, *bp;

    Compiling;
    Sl(2);
    Hpc(S0);
    bp1 = (stackitem *) S0;	      /* Get WHILE backpatch address */
    Pop;
    Compconst(s_branch);	      /* Compile unconditional branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *bp1 = hptr - bp1;                /* Backpatch REPEAT's jump out of loop */
    Pop;
}

prim P_do()			      /* Compile DO */
{
    Compiling;
    Compconst(s_xdo);		      /* Compile runtime DO word */
    So(1);
    Compconst(0);		      /* Reserve cell for LEAVE-taking */
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_xdo()			      /* Execute DO */
{
    Sl(2);
    Rso(3);
    Rpush = ip + ((stackitem) *ip);   /* Push exit address from loop */
    ip++;			      /* Increment past exit address word */
    Rpush = (rstackitem) S1;	      /* Push loop limit on return stack */
    Rpush = (rstackitem) S0;	      /* Iteration variable initial value to
                                         return stack */
    stk -= 2;
}

prim P_qdo()			      /* Compile ?DO */
{
    Compiling;
    Compconst(s_xqdo);		      /* Compile runtime ?DO word */
    So(1);
    Compconst(0);		      /* Reserve cell for LEAVE-taking */
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_xqdo()			      /* Execute ?DO */
{
    Sl(2);
    if (S0 == S1) {
        ip += (stackitem) *ip;
    } else {
        Rso(3);
        Rpush = ip + ((stackitem) *ip);/* Push exit address from loop */
        ip++;			      /* Increment past exit address word */
        Rpush = (rstackitem) S1;      /* Push loop limit on return stack */
        Rpush = (rstackitem) S0;      /* Iteration variable initial value to
                                         return stack */
    }
    stk -= 2;
}

prim P_loop()			      /* Compile LOOP */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_xloop); 	      /* Compile runtime loop */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get DO address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *(bp - 1) = (hptr - bp) + 1;      /* Backpatch exit address offset */
    Pop;
}

prim P_ploop()			      /* Compile +LOOP */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_pxloop);	      /* Compile runtime +loop */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get DO address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *(bp - 1) = (hptr - bp) + 1;      /* Backpatch exit address offset */
    Pop;
}

prim P_xloop()			      /* Execute LOOP */
{
    Rsl(3);
    R0 = (rstackitem) (((stackitem) R0) + 1);
    if (((stackitem) R0) == ((stackitem) R1)) {
        rstk -= 3;		      /* Pop iteration variable and limit */
        ip++;			      /* Skip the jump address */
    } else {
        ip += (stackitem) *ip;
    }
}

prim P_xploop() 		      /* Execute +LOOP */
{
    stackitem niter;

    Sl(1);
    Rsl(3);

    niter = ((stackitem) R0) + S0;
    Pop;
    if ((niter >= ((stackitem) R1)) &&
            (((stackitem) R0) < ((stackitem) R1))) {
        rstk -= 3;		      /* Pop iteration variable and limit */
        ip++;			      /* Skip the jump address */
    } else {
        ip += (stackitem) *ip;
        R0 = (rstackitem) niter;
    }
}

prim P_leave()			      /* Compile LEAVE */
{
    Rsl(3);
    ip = R2;
    rstk -= 3;
}

prim P_i()			      /* Obtain innermost loop index */
{
    Rsl(3);
    So(1);
    Push = (stackitem) R0;            /* It's the top item on return stack */
}

prim P_j()			      /* Obtain next-innermost loop index */
{
    Rsl(6);
    So(1);
    Push = (stackitem) rstk[-4];      /* It's the 4th item on return stack */
}

prim P_quit()			      /* Terminate execution */
{
    rstk = rstack;		      /* Clear return stack */
#ifdef WALKBACK
    wbptr = wback;
#endif
    ip = NULL;			      /* Stop execution of current word */
}

prim P_abort()			      /* Abort, clearing data stack */
{
    P_clear();			      /* Clear the data stack */
    P_quit();			      /* Shut down execution */
}

prim P_abortq() 		      /* Abort, printing message */
{
    if (state) {
        stringlit = True;	      /* Set string literal expected */
        Compconst(s_abortq);	      /* Compile ourselves */
    } else {
        /* Otherwise, print string literal in in-line code. */
#ifdef EMBEDDED
        //sprintf(outBuffer,"%s", (char *) ip);  // EMBEDDED
        (void) memset(outBuffer,0,OUTBUFF_SIZE) ;

        sprintf(outBuffer,"%s", ((char *) ip) + 1);  // EMBEDDED
#endif
     printf("%s",outBuffer);
        
#ifdef WALKBACK
        pwalkback();
#endif /* WALKBACK */
        P_abort();		      /* Abort */
        atl_comment = state = Falsity;/* Reset all interpretation state */
        forgetpend = defpend = stringlit =
            tickpend = ctickpend = False;
    }
}

/*  Compilation primitives  */

prim P_immediate()		      /* Mark most recent word immediate */
{
    dict->wname[0] |= IMMEDIATE;
}

prim P_lbrack() 		      /* Set interpret state */
{
    Compiling;
    state = Falsity;
}

prim P_rbrack() 		      /* Restore compile state */
{
    state = Truth;
}

Exported void P_dodoes()	      /* Execute indirect call on method */
{
    Rso(1);
    So(1);
    Rpush = ip; 		      /* Push instruction pointer */
#ifdef WALKBACK
    *wbptr++ = curword; 	      /* Place word on walkback stack */
#endif
    /* The compiler having craftily squirreled away the DOES> clause
       address before the word definition on the heap, we back up to
       the heap cell before the current word and load the pointer from
       there.  This is an ABSOLUTE heap address, not a relative offset. */
    ip = *((dictword ***) (((stackitem *) curword) - 1));

    /* Push the address of this word's body as the argument to the
       DOES> clause. */
    Push = (stackitem) (((stackitem *) curword) + Dictwordl);
}

prim P_does()			      /* Specify method for word */
{

    /* O.K., we were compiling our way through this definition and we've
       encountered the Dreaded and Dastardly Does.  Here's what we do
       about it.  The problem is that when we execute the word, we
       want to push its address on the stack and call the code for the
       DOES> clause by diverting the IP to that address.  But...how
       are we to know where the DOES> clause goes without adding a
       field to every word in the system just to remember it.  Recall
       that since this system is portable we can't cop-out through
       machine code.  Further, we can't compile something into the
       word because the defining code may have already allocated heap
       for the word's body.  Yukkkk.  Oh well, how about this?  Let's
       copy any and all heap allocated for the word down one stackitem
       and then jam the DOES> code address BEFORE the link field in
       the word we're defining.

       Then, when (DOES>) (P_dodoes) is called to execute the word, it
       will fetch that code address by backing up past the start of
       the word and seting IP to it.  Note that FORGET must recognise
       such words (by the presence of the pointer to P_dodoes() in
       their wcode field, in case you're wondering), and make sure to
       deallocate the heap word containing the link when a
       DOES>-defined word is deleted.  */

    if (createword != NULL) {
        stackitem *sp = ((stackitem *) createword), *hp;

        Rsl(1);
        Ho(1);

        /* Copy the word definition one word down in the heap to
           permit us to prefix it with the DOES clause address. */

        for (hp = hptr - 1; hp >= sp; hp--)
            *(hp + 1) = *hp;
        hptr++; 		      /* Expand allocated length of word */
        *sp++ = (stackitem) ip;       /* Store DOES> clause address before
                                         word's definition structure. */
        createword = (dictword *) sp; /* Move word definition down 1 item */
        createword->wcode = P_dodoes; /* Set code field to indirect jump */

        /* Now simulate an EXIT to bail out of the definition without
           executing the DOES> clause at definition time. */

        ip = R0;		      /* Set IP to top of return stack */
#ifdef WALKBACK
        wbptr = (wbptr > wback) ? wbptr - 1 : wback;
#endif
        Rpop;			      /* Pop the return stack */
    }
}

prim P_colon()			      /* Begin compilation */
{
    state = Truth;		      /* Set compilation underway */
    P_create(); 		      /* Create conventional word */
}

prim P_semicolon()		      /* End compilation */
{
    Compiling;
    Ho(1);
    Hstore = s_exit;
    state = Falsity;		      /* No longer compiling */
    /* We wait until now to plug the P_nest code so that it will be
       present only in completed definitions. */
    if (createword != NULL)
        createword->wcode = P_nest;   /* Use P_nest for code */
    createword = NULL;		      /* Flag no word being created */
}

prim ATH_char() {
    int i;
    char c;

    i=token(&instream);

    if( i != TokNull) {
        c=tokbuf[0];
        Push=c;
    }
}
/* Take address of next word */
prim P_tick() {
    int i;

    /* Try to get next symbol from the input stream.  If
       we can't, and we're executing a compiled word,
       report an error.  Since we can't call back to the
       calling program for more input, we're stuck. */

    i = token(&instream);	      /* Scan for next token */
    if (i != TokNull) {
        if (i == TokWord) {
            dictword *di;

            ucase(tokbuf);
            if ((di = lookup(tokbuf)) != NULL) {
                So(1);
                Push = (stackitem) di; /* Push word compile address */
            } else {
#ifdef EMBEDDED
                sprintf(outBuffer," '%s' undefined ", tokbuf); // EMBEDDED
#endif
     printf("%s",outBuffer);
            }
        } else {
#ifdef EMBEDDED
            sprintf(outBuffer,"\nWord not specified when expected.\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
            P_abort();
        }
    } else {
        /* O.K., there was nothing in the input stream.  Set the
           tickpend flag to cause the compilation address of the next
           token to be pushed when it's supplied on a subsequent input
           line. */
        if (ip == NULL) {
            tickpend = True;	      /* Set tick pending */
        } else {
#ifdef EMBEDDED
            sprintf(outBuffer,"\nWord requested by ` not on same input line.\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
            P_abort();
        }
    }
}

prim P_bracktick()		      /* Compile in-line code address */
{
    Compiling;
    ctickpend = True;		      /* Force literal treatment of next
                                     word in compile stream */
}

prim P_execute()		      /* Execute word pointed to by stack */
{
    dictword *wp;

    Sl(1);
    wp = (dictword *) S0;	      /* Load word address from stack */
    Pop;			      /* Pop data stack before execution */
    exword(wp); 		      /* Recursively call exword() to run
                                 the word. */
}

prim P_body()			      /* Get body address for word */
{
    Sl(1);
    S0 += Dictwordl * sizeof(stackitem);
}

prim P_state()			      /* Get state of system */
{
    So(1);
    Push = (stackitem) &state;
}

/*  Definition field access primitives	*/

#ifdef DEFFIELDS

prim P_find()			      /* Look up word in dictionary */
{
    dictword *dw;

    Sl(1);
    So(1);
    Hpc(S0);
    V strcpy(tokbuf, (char *) S0);    /* Use built-in token buffer... */
    dw = lookup(tokbuf);              /* So ucase() in lookup() doesn't wipe */
    /* the token on the stack */
    if (dw != NULL) {
        S0 =  (stackitem) dw;
        /* Push immediate flag */
        Push = (dw->wname[0] & IMMEDIATE) ? 1 : -1;
    } else {
        Push = 0;
    }
}

#define DfOff(fld)  (((char *) &(dict->fld)) - ((char *) dict))

prim P_toname() 		      /* Find name field from compile addr */
{
    Sl(1);
    S0 += DfOff(wname);
}

prim P_tolink() 		      /* Find link field from compile addr */
{
    if (DfOff(wnext) != 0) {
#ifdef EMBEDDED
        sprintf(outBuffer,"\n>LINK Foulup--wnext is not at zero!\n");  // EMBEDDED
#endif
     printf("%s",outBuffer);
    }
    /*  Sl(1);
        S0 += DfOff(wnext);  */	      /* Null operation.  Wnext is first */
}

prim P_frombody()		      /* Get compile address from body */
{
    Sl(1);
    S0 -= Dictwordl * sizeof(stackitem);
}

prim P_fromname()		      /* Get compile address from name */
{
    Sl(1);
    S0 -= DfOff(wname);
}

prim P_fromlink()		      /* Get compile address from link */
{
    if (DfOff(wnext) != 0) {
#ifdef EMBEDDED
        sprintf(outBuffer,"\nLINK> Foulup--wnext is not at zero!\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
    }
    /*  Sl(1);
        S0 -= DfOff(wnext);  */	      /* Null operation.  Wnext is first */
}

#undef DfOff

#define DfTran(from,to) (((char *) &(dict->to)) - ((char *) &(dict->from)))

prim P_nametolink()		      /* Get from name field to link */
{
    char *from, *to;

    Sl(1);
    /*
       S0 -= DfTran(wnext, wname);
       */
    from = (char *) &(dict->wnext);
    to = (char *) &(dict->wname);
    S0 -= (to - from);
}

prim P_linktoname()		      /* Get from link field to name */
{
    char *from, *to;

    Sl(1);
    /*
       S0 += DfTran(wnext, wname);
       */
    from = (char *) &(dict->wnext);
    to = (char *) &(dict->wname);
    S0 += (to - from);
}

prim P_fetchname()		      /* Copy word name to string buffer */
{
    Sl(2);			      /* nfa string -- */
    Hpc(S0);
    Hpc(S1);
    /* Since the name buffers aren't in the system heap, but
       rather are separately allocated with alloc(), we can't
       check the name pointer references.  But, hey, if the user's
       futzing with word dictionary items on the heap in the first
       place, there's a billion other ways to bring us down at
       his command. */
    V strcpy((char *) S0, *((char **) S1) + 1);
    Pop2;
}

prim P_storename()		      /* Store string buffer in word name */
{
    char tflags;
    char *cp;

    Sl(2);			      /* string nfa -- */
    Hpc(S0);			      /* See comments in P_fetchname above */
    Hpc(S1);			      /* checking name pointers */
    tflags = **((char **) S0);
    free(*((char **) S0));
    *((char **) S0) = cp = alloc((unsigned int) (strlen((char *) S1) + 2));
    V strcpy(cp + 1, (char *) S1);
    *cp = tflags;
    Pop2;
}

#endif /* DEFFIELDS */

#ifdef SYSTEM
prim P_system()
{				      /* string -- status */
    Sl(1);
    Hpc(S0);
    S0 =  system((char *) S0);
}

// Time to leave.
#endif /* SYSTEM */

#ifdef TRACE
prim P_trace()			      /* Set or clear tracing of execution */
{
    Sl(1);
    atl_trace = (S0 == 0) ? Falsity : Truth;
    Pop;
}
#endif /* TRACE */

#ifdef WALKBACK
prim P_walkback()		      /* Set or clear error walkback */
{
    Sl(1);
    atl_walkback = (S0 == 0) ? Falsity : Truth;
    Pop;
}
#endif /* WALKBACK */

#ifdef WORDSUSED

prim P_wordsused()		      /* List words used by program */
{
    dictword *dw = dict;

    while (dw != NULL) {
        if (*(dw->wname) & WORDUSED) {
#ifdef EMBEDDED
            sprintf(outBuffer,"\r\n%s", dw->wname + 1); // EMBEDDED
#endif
     printf("%s",outBuffer);
        }
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#endif
        dw = dw->wnext;
    }
#ifdef EMBEDDED
    sprintf(outBuffer,"\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
}

prim P_wordsunused()		      /* List words not used by program */
{
    dictword *dw = dict;

    while (dw != NULL) {
        if (!(*(dw->wname) & WORDUSED)) {
#ifdef EMBEDDED
            sprintf(outBuffer,"\r\n%s", dw->wname + 1); // EMBEDDED
#endif
     printf("%s",outBuffer);
        }
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#endif
        dw = dw->wnext;
    }
#ifdef EMBEDDED
    sprintf(outBuffer,"\n");  // EMBEDDED
#endif
     printf("%s",outBuffer);
}
#endif /* WORDSUSED */

#ifdef COMPILERW

prim P_brackcompile()		      /* Force compilation of immediate word */
{
    Compiling;
    cbrackpend = True;		      /* Set [COMPILE] pending */
}

prim P_literal()		      /* Compile top of stack as literal */
{
    Compiling;
    Sl(1);
    Ho(2);
    Hstore = s_lit;		      /* Compile load literal word */
    Hstore = S0;		      /* Compile top of stack in line */
    Pop;
}

prim P_compile()		      /* Compile address of next inline word */
{
    Compiling;
    Ho(1);
    Hstore = (stackitem) *ip++;       /* Compile the next datum from the
                                         instruction stream. */
}

prim P_backmark()		      /* Mark backward backpatch address */
{
    Compiling;
    So(1);
    Push = (stackitem) hptr;	      /* Push heap address onto stack */
}

prim P_backresolve()		      /* Emit backward jump offset */
{
    stackitem offset;

    Compiling;
    Sl(1);
    Ho(1);
    Hpc(S0);
    offset = -(hptr - (stackitem *) S0);
    Hstore = offset;
    Pop;
}

prim P_fwdmark()		      /* Mark forward backpatch address */
{
    Compiling;
    Ho(1);
    Push = (stackitem) hptr;	      /* Push heap address onto stack */
    Hstore = 0;
}

prim P_fwdresolve()		      /* Emit forward jump offset */
{
    stackitem offset;

    Compiling;
    Sl(1);
    Hpc(S0);
    offset = (hptr - (stackitem *) S0);
    *((stackitem *) S0) = offset;
    Pop;
}

#endif /* COMPILERW */

prim ATH_debuggerAttached() {
	Push=(CoreDebug->DHCSR &1) ? true : false ;
}

/*  Table of primitive words  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

static const struct primfcn primt[] = {
    {"0+", P_plus},
    {"0-", P_minus},
    {"0*", P_times},
    {"0/", P_div},
    {"0MOD", P_mod},
    {"0/MOD", P_divmod},
    {"0MIN", P_min},
    {"0MAX", P_max},
    {"0NEGATE", P_neg},
    {"0ABS", P_abs},
    {"0=", P_equal},
    {"0<>", P_unequal},
    {"0>", P_gtr},
    {"0<", P_lss},
    {"0>=", P_geq},
    {"0<=", P_leq},

    {"0AND", P_and},
    {"0OR", P_or},
    {"0XOR", P_xor},
    {"0NOT", P_not},
    {"0SHIFT", P_shift},

    {"0DEPTH", P_depth},
    {"0CLEAR", P_clear},
    {"0DUP", P_dup},
    {"0DROP", P_drop},
    {"0SWAP", P_swap},
    {"0NIP", P_nip},
    {"0OVER", P_over},
    {"0PICK", P_pick},
    {"0ROT", P_rot},
    {"0-ROT", P_minusrot},
    {"0>R", P_tor},
    {"0ROLL", P_roll},
    {"0R>", P_rfrom},
    {"0R@", P_rfetch},

#ifdef SHORTCUTA
    {"01+", P_1plus},
    {"02+", P_2plus},
    {"01-", P_1minus},
    {"02-", P_2minus},
    {"02*", P_2times},
    {"02/", P_2div},
#endif /* SHORTCUTA */

#ifdef SHORTCUTC
    {"00=", P_0equal},
    {"00<>", P_0notequal},
    {"00>", P_0gtr},
    {"00<", P_0lss},
#endif /* SHORTCUTC */

#ifdef DOUBLE
    {"02DUP", P_2dup},
    {"02DROP", P_2drop},
    {"02SWAP", P_2swap},
    {"02OVER", P_2over},
    {"02ROT", P_2rot},
    {"02VARIABLE", P_2variable},
    {"02CONSTANT", P_2constant},
    {"02!", P_2bang},
    {"02@", P_2at},
#endif /* DOUBLE */

    {"0VARIABLE", P_variable},
    {"0CONSTANT", P_constant},
    {"0!", P_bang},
    {"0@", P_at},
    {"0+!", P_plusbang},
    {"0ALLOT", P_allot},
    {"0,", P_comma},
    {"0C!", P_cbang},
    {"0C@", P_cat},
    {"0C,", P_ccomma},
    {"0C=", P_cequal},
    {"0HERE", P_here},

#ifdef ARRAY
    {"0ARRAY", P_array},
#endif

#ifdef STRING
    {"0(STRLIT)", P_strlit},
    {"0STRING", P_string},
    {"0STRCPY", P_strcpy},
    {"0S!", P_strcpy},
    {"0STRCAT", P_strcat},
    {"0S+", P_strcat},
    {"0STRLEN", P_strlen},
    {"0STRCMP", P_strcmp},
    {"0STRCHAR", P_strchar},
    {"0SUBSTR", P_substr},
    {"0COMPARE", P_strcmp},
    {"0STRFORM", P_strform},
    {"0MOVE", P_move},
    {"0STRSEP", P_strsep},
#ifdef REAL
    {"0FSTRFORM", P_fstrform},
    {"0STRREAL", P_strreal},
#endif
    {"0STRINT", P_strint},
#endif /* STRING */

#ifdef REAL
    {"0(FLIT)", P_flit},
    {"0F+", P_fplus},
    {"0F-", P_fminus},
    {"0F*", P_ftimes},
    {"0F/", P_fdiv},
    {"0FMIN", P_fmin},
    {"0FMAX", P_fmax},
    {"0FNEGATE", P_fneg},
    {"0FABS", P_fabs},
    {"0F=", P_fequal},
    {"0F<>", P_funequal},
    {"0F>", P_fgtr},
    {"0F<", P_flss},
    {"0F>=", P_fgeq},
    {"0F<=", P_fleq},
    {"0F.", P_fdot},
    {"0FLOAT", P_float},
    {"0FIX", P_fix},
#ifdef MATH
    {"0ACOS", P_acos},
    {"0ASIN", P_asin},
    {"0ATAN", P_atan},
    {"0ATAN2", P_atan2},
    {"0COS", P_cos},
    {"0EXP", P_exp},
    {"0LOG", P_log},
    {"0POW", P_pow},
    {"0SIN", P_sin},
    {"0SQRT", P_sqrt},
    {"0TAN", P_tan},
#endif /* MATH */
#endif /* REAL */

    {"0(NEST)", P_nest},
    {"0EXIT", P_exit},
    {"0(LIT)", P_dolit},
    {"0BRANCH", P_branch},
    {"0?BRANCH", P_qbranch},
    {"1IF", P_if},
    {"1ELSE", P_else},
    {"1THEN", P_then},
    {"0?DUP", P_qdup},
    {"1BEGIN", P_begin},
    {"1UNTIL", P_until},
    {"1AGAIN", P_again},
    {"1WHILE", P_while},
    {"1REPEAT", P_repeat},
    {"1DO", P_do},
    {"1?DO", P_qdo},
    {"1LOOP", P_loop},
    {"1+LOOP", P_ploop},
    {"0(XDO)", P_xdo},
    {"0(X?DO)", P_xqdo},
    {"0(XLOOP)", P_xloop},
    {"0(+XLOOP)", P_xploop},
    {"0LEAVE", P_leave},
    {"0I", P_i},
    {"0J", P_j},
    {"0QUIT", P_quit},
    {"0ABORT", P_abort},
    {"1ABORT\"", P_abortq},

#ifdef SYSTEM
    {"0SYSTEM", P_system},
    {"0BYE", ATH_bye},
#endif
#ifdef TRACE
    {"0TRACE", P_trace},
#endif
#ifdef WALKBACK
    {"0WALKBACK", P_walkback},
#endif

#ifdef WORDSUSED
    {"0WORDSUSED", P_wordsused},
    {"0WORDSUNUSED", P_wordsunused},
#endif

#ifdef MEMSTAT
    {"0MEMSTAT", atl_memstat},
#endif

    {"0:", P_colon},
    {"1;", P_semicolon},
    {"0IMMEDIATE", P_immediate},
    {"1[", P_lbrack},
    {"0]", P_rbrack},
    {"0CREATE", P_create},
    {"0FORGET", P_forget},
    {"0DOES>", P_does},
    {"0'", P_tick},
    {"1[']", P_bracktick},
    {"0EXECUTE", P_execute},
    {"0>BODY", P_body},
    {"0STATE", P_state},
    {"0CHAR", ATH_char},

#ifdef DEFFIELDS
    {"0FIND", P_find},
    {"0>NAME", P_toname},
    {"0>LINK", P_tolink},
    {"0BODY>", P_frombody},
    {"0NAME>", P_fromname},
    {"0LINK>", P_fromlink},
    {"0N>LINK", P_nametolink},
    {"0L>NAME", P_linktoname},
    {"0NAME>S!", P_fetchname},
    {"0S>NAME!", P_storename},
#endif /* DEFFIELDS */

#ifdef COMPILERW
    {"1[COMPILE]", P_brackcompile},
    {"1LITERAL", P_literal},
    {"0COMPILE", P_compile},
    {"0<MARK", P_backmark},
    {"0<RESOLVE", P_backresolve},
    {"0>MARK", P_fwdmark},
    {"0>RESOLVE", P_fwdresolve},
#endif /* COMPILERW */

#ifdef CONIO
    {"0.", P_dot},
    {"0?", P_question},
    {"0CR", P_cr},
    {"0.S", P_dots},
    {"1.\"", P_dotquote},
    {"1.(", P_dotparen},
    {"0TYPE", P_type},
    {"0WORDS", P_words},
	{"0EMIT", P_emit},
#endif /* CONIO */

#ifdef FILEIO
    {"0FILE", P_file},
    {"0(MKDIR)", P_mkdir},
    {"0(RMDIR)", P_rmdir},

    {"0FD@", P_fdAt},
//    {"0FACCESS", P_fopen},
    {"0FOPEN", P_fopen},
    {"0FSIZE", FR_getFileSize},
    {"0$FSIZE", FR_getFileSizeByName},

    {"0ACCESS", P_access},
    {"0FSYNC", P_fsync},
    {"0FCLOSE", P_fclose},
    {"0UNLINK", P_fdelete},
    {"0RM-R", P_recursiveDelete},
    {"0(MV)", P_frename},
    {"0FGETS", P_fgetline},
    {"0FPUTS", P_fputline},
    {"0FREAD", P_fread},
    {"0FWRITE", P_fwrite},
    {"0FGETC", P_fgetc},
    {"0FPUTC", P_fputc},
    {"0FTELL", P_ftell},
    {"0FSEEK", P_fseek},
    {"0FLOAD", P_fload},
    {"0INCLUDE", P_include},
    {"0?INCLUDE", P_qinclude},
    {"0>LOG", FR_logMessage},
    {"0LOG-CLEAR", FR_logClear},
    {"0LOG-DISPLAY", FR_logDump},
    {"0UPGRADE-PROBE", FR_upgradeProbe},

#ifdef ELCOFS
	{(char *)"0ELCOFS-BEGIN", ELCOFS_begin },
	{(char *)"0ELCOFS-COMMIT", ELCOFS_commit },
	{(char *)"0ELCOFS-ROLLBACK", ELCOFS_rollback },
#endif
	{(char *)"0LSOF", ELCOFS_lsof },

	{(char *)"0O_RDONLY", FR_fsRdOnly },
	{(char *)"0O_RDWR", FR_fsRdwrDef },
	{(char *)"0O_CREAT", FR_fsCreatDef },
	{(char *)"0O_TRUNC", FR_fsTruncDef },
	{(char *)"0O_WRONLY", FR_fsWrOnly },
	{(char *)"0O_APPEND", FR_fsAppend },
#endif /* FILEIO */

#ifdef EVALUATE
    {"0EVALUATE", P_evaluate},
#endif /* EVALUATE */

#ifdef ATH
	{(char *)"0MKBUFFER",ATH_mkBuffer},
	{(char *)"0MEMSAFE",ATH_memsafe},
	{(char *)"0?MEMSAFE",ATH_qmemsafe},
	{(char *)"0FDUMP",ATH_fileDump},
	{(char *)"0DUMP",ATH_dump},
	{(char *)"0FILL",ATH_fill},
	{(char *)"0ERASE",ATH_erase},
	{(char *)"0W@",ATH_wat},
	{(char *)"0W!",ATH_wbang},
	{(char *)"0W>CELL",ATH_16toCell},

	{(char *)"0HEX",ATH_hex},
	{(char *)"0DECIMAL",ATH_dec},
	{(char *)"0BYE",ATH_bye},
	{(char *)"0?FILEIO",ATH_qfileio},
    {(char *)"0.FEATURES", ATH_Features},
    {(char *)"0TIB", ATH_Instream},
    {(char *)"0TOKEN", ATH_Token},
    {(char *)"0?LINUX", ATH_qlinux},
    {(char *)"0?FREERTOS", ATH_qfreertos},
    {(char *)"0?DEBUG", ATH_qdebug},

    {(char *)"0?YAFFS", ATH_qyaffs},
    {(char *)"0?ELCOFS", ATH_qelcofs},
    {(char *)"0?BATCHING", ATH_qbatching},
    {(char *)"0BATCH@", FR_getTaskBatching},
    {(char *)"0?READY", FR_getTaskReady},

    {(char *)"0MS", ATH_ms},
    {(char *)"0PWD", ATH_pwd},
    {(char *)"0CD", ATH_cd},

    {(char *)"0DIR", RT_dir},
    {(char *)"0DS", ATH_fsSize },
    {(char *)"0DF", ATH_fsFree },

    {(char *)"0ERRNO", ATH_errno},
    {(char *)"0HELP", ATH_help},
    {(char *)"0BANNER", ATH_banner},
    {(char *)"0SET-CONSOLE", FR_setConsole},

#endif
#ifdef EMBEDDED
    {(char *)"0SEMIHOSTING", ATH_semihosting},
#endif

#ifdef ANSI
    {(char *)"0CELL", ANSI_cell},
    {(char *)"0CELLS", ANSI_cells},
    {(char *)"0CELL+", ANSI_cellplus},
    {(char *)"0CHARS", ANSI_chars},
    {(char *)"0ALLOCATE", ANSI_allocate},
    {(char *)"0FREE", ANSI_free},
    {(char *)"0SBRK", ATH_sbrk},
    {(char *)"0MALLINFO", ATH_mallinfo},
    {(char *)"0MALLINFO-ARENA", ATH_mallinfoArena },
    {(char *)"0MALLINFO-USED", ATH_mallinfoUsed },
    {(char *)"0MALLINFO-FREE", ATH_mallinfoFree },

#endif

#ifdef FREERTOS
    {(char *)"0(TOUCH)", RT_touch},
    {(char *)"0MKFILE", RT_mkfile},
    {(char *)"0CRCFILE", RT_crcfile},
    {(char *)"0PERROR", ATH_perror},

#ifdef NAND
	{(char *)"0MKSCMD", FR_mkScmd},
	{(char *)"0NAND-SETUP", FR_NANDSetup},
	{(char *)"0NAND-BLOCKS", FR_NANDBlocks},
	{(char *)"0NAND-MKBAD", FR_badBlockMap },
	{(char *)"0NAND-LOCK", FR_NANDLock},
	{(char *)"0NAND-UNLOCK", FR_NANDUnlock },
	{(char *)"0NAND-READ", FR_NANDRead },
	{(char *)"0NAND-WRITE", FR_NANDWrite },
	{(char *)"0NAND-ERASE", FR_NANDErase },
	{(char *)"0NAND-GETPARAMS", FR_NANDGetParams },
#endif

#ifdef ELCOFS
	{(char *)"0ELCOFS-FORMAT", FR_elcofsFormat },
	{(char *)"0ELCOFS-MOUNT", FR_elcofsMount },
	{(char *)"0ELCOFS-UNMOUNT", FR_elcofsUnmount },
#endif

#ifdef YAFFS
	{(char *)"0YAFFS-INSTALL", FR_yaffsInstall },
#endif

	{(char *)"0FIXED-OPEN", FR_fixedOpen },
	{(char *)"0FIXED-CLOSE", FR_fixedClose },
	{(char *)"0FIXED-READ", FR_fixedRead },

	{(char *)"0FS-SYNC", FR_fsSync },
	{(char *)"0SEEK_SET", FR_yaffsSeekSet },
	{(char *)"0SEEK_CUR", FR_yaffsSeekCur },
	{(char *)"0SEEK_END", FR_yaffsSeekEnd },
	{(char *)"0LOAD-FILE", FR_loadFile },

	{(char *)"0BATCH-BUCKET-SIZE", FR_batchBucketSize },
	{(char *)"0BATCH-CREATE", FR_batchCreate },
	{(char *)"0MK-BATCH-FILENAME", FR_mkBatchFilename },
	{(char *)"0BATCH-OPEN", FR_batchOpen },
	{(char *)"0BATCH-CLOSE", FR_batchClose },
	{(char *)"0BATCH-RENAME", FR_batchRename },
	{(char *)"0BATCH-COPY", FR_batchCopy },

	{(char *)"0BATCH-COUNT", FR_getNumberOfBatches },

	{(char *)"0BATCH-RM", FR_batchRm },
	{(char *)"0BATCH-RM-ALL", FR_batchRmAll },
	{(char *)"0BATCH-RM-READING", FR_batchRmReading },

	{(char *)"0BATCH-CLEAR", FR_batchClear },

	{(char *)"0ADD-READING", FR_batchAddReading },

	{(char *)"0GET-FIRST-BATCH", FR_getFirstBatch },
	{(char *)"0GET-NEXT-BATCH", FR_getNextBatch },
	{(char *)"0GET-LAST-BATCH", FR_getLastBatch },

	{(char *)"0GET-READING-COUNT", FR_getReadingCount },
	{(char *)"0GET-READING-SIZE", FR_batchGetReadingSize },

	{(char *)"0GET-READING", FR_batchGetReading },
	{(char *)"0GET-BATCH-ID", FR_getBatchID },
	{(char *)"0GET-BATCH-NAME", FR_getBatchName },
	{(char *)"0GET-BATCH-SIZE", FR_getBatchSize },
	{(char *)"0SET-BATCH-SIZE", FR_setBatchSize },

	{(char *)"0GET-TIME", FR_getTime },
	{(char *)"0SET-TIME", FR_setTime },

	{(char *)"0BR@", FR_getBR },
	{(char *)"0BR!", FR_setBR },


	{(char *)"0RX", FR_rxXmodem},
    {(char *)"0QID@", FR_getQid},
    {(char *)"0DB@", FR_getTaskDb},
    {(char *)"0DB!", FR_setTaskDb},

	{(char *)"0WRITE-PIN", FR_writePin},
	{(char *)"0READ-PIN", FR_readPin},
	{(char *)"0SET-BACKLIGHT", FR_setBacklight},

    {(char *)"0POOL@", FR_getPoolId } ,
    {(char *)"0POOL-FREE", FR_poolFree } ,
    {(char *)"0POOL-ALLOCATE", FR_poolAllocate } ,

    {(char *)"0CMD-PARSE", FR_cmdParse },
    {(char *)"0TICK@", FR_getSysTick },
	{(char *)"0LCD-RESET", FR_lcdReset },
	{(char *)"0LCD-REG-SET",FR_lcdRegSet},

	{(char *)"0DEV-RESET", FR_devReset},
	{(char *)"0?UART-RX", FR_uartRxReady},
	{(char *)"0UART-TYPE", FR_uartTxBuffer},
	{(char *)"0UART-KEY", FR_uartRxByte},
	{(char *)"0UART-READLINE",FR_uartReadLine},
	{(char *)"0UART-EMIT",  FR_uartEmit},
	{(char *)"0RING-BUFFER@", FR_ringBufferPtr },
	{(char *)"0RING-BUFFER-ERASE", FR_ringBufferErase },
	{(char *)"0RING-BUFFER-LOCK@", FR_getRingBufferLock },

	{(char *)"0?KEY", FR_qconsoleKey },
	{(char *)"0KEY", FR_consoleKey },
	{(char *)"0EXPECT", FR_consoleExpect },

	{(char *)"0SEM@", FR_semGet },
	{(char *)"0SEM-COUNT@", FR_semGetCount },
	{(char *)"0SEM-GIVE", FR_semGive },
	{(char *)"0SEM-TAKE", FR_semTake },
	{(char *)"0STACK-HWM", FR_stackHighWaterMark },
	{(char *)"0PS", FR_ps },
	{(char *)"0DOVER-PS", FR_doverPs },

	{(char *)"0KILL", FR_kill },
	{(char *)"0SUSPEND", FR_suspend },
	{(char *)"0RESUME", FR_resume },

	{(char *)"0NICE", FR_nice },
	{(char *)"0QDEPTH", FR_qdepth },
	{(char *)"0QSPACE", FR_qspace },

	{(char *)"0GET-TASK-HANDLE", FR_getTaskHandle },
	{(char *)"0GET-TASK-STATE", FR_getTaskState },
	{(char *)"0BT-GET-SETTING", FR_BTgetSetting },
	{(char *)"0BT-ENTER-DATA", FR_BTenterDataMode },
	{(char *)"0BT-EXIT-DATA", FR_BTexitDataMode },

#endif
#ifdef PUBSUB
	{(char *)"0MKDB",     FR_mkdb},
	{(char *)"0DUMPDB",   FR_dumpDb},
	{(char *)"0DUMP-INI",   FR_dumpDbIni},

	{(char *)"0ADD-RECORD",  FR_addRecord},
	{(char *)"0SET-READONLY",  FR_setReadOnly},
	{(char *)"0ADD-INI-SECTION",  FR_addIniSection},
	{(char *)"0ADD-INI-RECORD",  FR_addIniRecord},
	{(char *)"0LOOKUP-INI-BOOL", FR_lookupIniRecordBool  },

	{(char *)"0FIND-FIRST",  FR_findFirst},
	{(char *)"0FIND-NEXT",  FR_findNext},

	{(char *)"0LOOKUP",  FR_lookup},
	{(char *)"0LOOKUP-REC",  FR_lookupRecord},
	{(char *)"0PUBLISH",  FR_publish},
	{(char *)"0GET-SUBCOUNT",  FR_subCount},
	{(char *)"0.RECORD",  FR_displayRecord},
	{(char *)"0ALLOC-MSG",  FR_allocMsg},
    {(char *)"0MESSAGE@", FR_getMessage},
    {(char *)"0MESSAGE!", FR_putMessage},
    {(char *)"0EVENT@", FR_getEvent},
#ifdef FREERTOS
    // This code is compiled if PUBSUB AND FREERTOS are defined
    {(char *)"0MKMSG-GET", FR_mkmsgGet},
    {(char *)"0MKMSG-SET", FR_mkmsgSet},
    {(char *)"0MKMSG-SUB", FR_mkmsgSub},
    {(char *)"0MKMSG-UNSUB", FR_mkmsgUnsub},
    {(char *)"0MKMSG-DONE", FR_mkmsgDone },
    {(char *)"0MSG-ERASE", FR_msgErase },
    {(char *)"0.MSG-TEST", FR_displayMsg },

//    {(char *)"0MKMSG-OPEN", FR_mkmsgOpen},

    {(char *)"0SET-CMD", FR_setCmd},
    {(char *)"0GET-CMD", FR_getCmd},

    {(char *)"0SET-KEY", FR_setKey},
    {(char *)"0GET-KEY", FR_getKey},

    {(char *)"0SET-VALUE", FR_setValue},
    {(char *)"0GET-VALUE", FR_getValue},

    {(char *)"0SET-FIELD-COUNT", FR_setFieldCnt},
    {(char *)"0GET-FIELD-COUNT", FR_getFieldCnt},

    {(char *)"0SET-SENDER", FR_setSender},
    {(char *)"0GET-SENDER", FR_getSender},
    {(char *)"0SET-CHANGES", FR_setChanges },
#endif
#ifdef INIPARSER
    {(char *)"0LOAD-INI", INI_loadIniFile },
    {(char *)"0INI-GET-BOOLEAN", INI_getBoolean },
    {(char *)"0SETTING-DB@", INI_getSettingsDb },
#endif
    {(char *)"0DEBUG-ATTACHED", ATH_debuggerAttached },
#ifdef PTHREAD
	// TODO Rename all in this section from FR_ to PS_
    {(char *)"0COMMS", PS_comms},
#endif

#endif
    {NULL, (codeptr) 0}
};
#pragma GCC diagnostic pop

/*  ATL_PRIMDEF  --  Initialise the dictionary with the built-in primitive
    words.  To save the memory overhead of separately
    allocated word items, we get one buffer for all
    the items and link them internally within the buffer. */

void atl_primdef(const struct primfcn *pt)
{
    const struct primfcn *pf = pt;
    dictword *nw;
    int i, n = 0;
#ifdef WORDSUSED
#ifdef READONLYSTRINGS
    unsigned int nltotal;
    char *dynames, *cp;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */

    /* Count the number of definitions in the table. */

    while (pf->pname != NULL) {
        n++;
        pf++;
    }

#ifdef WORDSUSED
#ifdef READONLYSTRINGS
    nltotal = n;
    for (i = 0; i < n; i++) {
        nltotal += strlen(pt[i].pname);
    }
    cp = dynames = alloc(nltotal);
    for (i = 0; i < n; i++) {
        strcpy(cp, pt[i].pname);
        cp += strlen(cp) + 1;
    }
    cp = dynames;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */

    nw = (dictword *) alloc((unsigned int) (n * sizeof(dictword)));

    nw[n - 1].wnext = dict;
    dict = nw;
    for (i = 0; i < n; i++) {
        nw->wname = (char*)pt->pname;
#ifdef WORDSUSED
#ifdef READONLYSTRINGS
        nw->wname = cp;
        cp += strlen(cp) + 1;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */
        nw->wcode = pt->pcode;
        if (i != (n - 1)) {
            nw->wnext = nw + 1;
        }
        nw++;
        pt++;
    }
}

#ifdef WALKBACK

/*  PWALKBACK  --  Print walkback trace.  */

static void pwalkback() {
    if (atl_walkback && ((curword != NULL) || (wbptr > wback))) {
#ifdef EMBEDDED
        sprintf(outBuffer,"Walkback:\r\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
        
        if (curword != NULL) {
#ifdef EMBEDDED
            sprintf(outBuffer,"   %s\r\n", curword->wname + 1); // EMBEDDED
#endif
     printf("%s",outBuffer);
        }
        while (wbptr > wback) {
            dictword *wb = *(--wbptr);
#ifdef EMBEDDED
            sprintf(outBuffer,"   %s\r\n", wb->wname + 1); // EMBEDDED
#endif
     printf("%s",outBuffer);
        }
    }
}
#endif /* WALKBACK */

/*  TROUBLE  --  Common handler for serious errors.  */

static void trouble(const char *kind) {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
    sprintf(outBuffer,"\n%s.\r\n", kind); // EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
#ifdef WALKBACK
    pwalkback();
#endif /* WALKBACK */
    P_abort();			      /* Abort */
    atl_comment = state = Falsity;    /* Reset all interpretation state */
    forgetpend = defpend = stringlit =
        tickpend = ctickpend = False;
}

/*  ATL_ERROR  --  Handle error detected by user-defined primitive.  */

void atl_error(char *kind) {
    trouble(kind);
    evalstat = ATL_APPLICATION;       /* Signify application-detected error */
}

#ifndef NOMEMCHECK

/*  STAKOVER  --  Recover from stack overflow.	*/

void stakover() {
    trouble("Stack overflow");
    evalstat = ATL_STACKOVER;
}

/*  STAKUNDER  --  Recover from stack underflow.  */

void stakunder()
{
    trouble("Stack underflow");
    evalstat = ATL_STACKUNDER;
}

/*  RSTAKOVER  --  Recover from return stack overflow.	*/

void rstakover()
{
    trouble("Return stack overflow");
    evalstat = ATL_RSTACKOVER;
}

/*  RSTAKUNDER	--  Recover from return stack underflow.  */

void rstakunder()
{
    trouble("Return stack underflow");
    evalstat = ATL_RSTACKUNDER;
}

/*  HEAPOVER  --  Recover from heap overflow.  Note that a heap
    overflow does NOT wipe the heap; it's up to
    the user to do this manually with FORGET or
    some such. */

void heapover()
{
    trouble("Heap overflow");
    evalstat = ATL_HEAPOVER;
}

/*  BADPOINTER	--  Abort if bad pointer reference detected.  */

void badpointer()
{
    trouble("Bad pointer");
    evalstat = ATL_BADPOINTER;
}

/*  NOTCOMP  --  Compiler word used outside definition.  */

static void notcomp()
{
    trouble("Compiler word outside definition");
    evalstat = ATL_NOTINDEF;
}

/*  DIVZERO  --  Attempt to divide by zero.  */

static void divzero()
{
    trouble("Divide by zero");
    evalstat = ATL_DIVZERO;
}

#endif /* !NOMEMCHECK */

/*  EXWORD  --	Execute a word (and any sub-words it may invoke). */

static void exword(dictword *wp)
{
    curword = wp;
#ifdef TRACE
    if (atl_trace) {

	 P_dots();
     printf("\t\t");
#ifdef EMBEDDED
        sprintf(outBuffer,"\nTrace: %s ", curword->wname + 1); //  EMBEDDED
#endif
     printf("%s",outBuffer);
    }
#endif /* TRACE */
    (*curword->wcode)();	      /* Execute the first word */
    while (ip != NULL) {
#ifdef BREAK
#ifdef Keybreak
        Keybreak();		      /* Poll for asynchronous interrupt */
#endif
        if (broken) {		      /* Did we receive a break signal */
            trouble("Break signal");
            evalstat = ATL_BREAK;
            break;
        }
#endif /* BREAK */
        curword = *ip++;
#ifdef TRACE
        if (atl_trace) {
	 P_dots();
     printf("\t\t");
#ifdef EMBEDDED
     sprintf(outBuffer,"\nTrace: %s ", curword->wname + 1); // EMBEDDED
#endif
     printf("%s",outBuffer);
        }
#endif /* TRACE */
        (*curword->wcode)();	      /* Execute the next word */
    }
    curword = NULL;
}

/*  ATL_INIT  --  Initialise the ATLAST system.  The dynamic storage areas
    are allocated unless the caller has preallocated buffers
    for them and stored the addresses into the respective
    pointers.  In either case, the storage management
    pointers are initialised to the correct addresses.  If
    the caller preallocates the buffers, it's up to him to
    ensure that the length allocated agrees with the lengths
    given by the atl_... cells.  */

void atl_init()
{
    if (dict == NULL) {
        atl_primdef(primt);	      /* Define primitive words */
        dictprot = dict;	      /* Set protected mark in dictionary */

        /* Look up compiler-referenced words in the new dictionary and
           save their compile addresses in static variables. */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

#define Cconst(cell, name)  cell = (stackitem) lookup(name); if(cell==0)abort()
        Cconst(s_exit, "EXIT");
        Cconst(s_lit, "(LIT)");
#ifdef REAL
        Cconst(s_flit, "(FLIT)");
#endif
        Cconst(s_strlit, "(STRLIT)");
        Cconst(s_dotparen, ".(");
        Cconst(s_qbranch, "?BRANCH");
        Cconst(s_branch, "BRANCH");
        Cconst(s_xdo, "(XDO)");
        Cconst(s_xqdo, "(X?DO)");
        Cconst(s_xloop, "(XLOOP)");
        Cconst(s_pxloop, "(+XLOOP)");
        Cconst(s_abortq, "ABORT\"");
#undef Cconst
#pragma GCC diagnostic pop


        if (stack == NULL) {	      /* Allocate stack if needed */
            stack = (stackitem *)
                alloc(((unsigned int) atl_stklen) * sizeof(stackitem));
        }
        stk = stackbot = stack;
#ifdef MEMSTAT
        stackmax = stack;
#endif
        stacktop = stack + atl_stklen;
        if (rstack == NULL) {	      /* Allocate return stack if needed */
            rstack = (dictword ***)
                alloc(((unsigned int) atl_rstklen) *
                        sizeof(dictword **));
        }
        rstk = rstackbot = rstack;
#ifdef MEMSTAT
        rstackmax = rstack;
#endif
        rstacktop = rstack + atl_rstklen;
#ifdef WALKBACK
        if (wback == NULL) {
            wback = (dictword **) alloc(((unsigned int) atl_rstklen) *
                    sizeof(dictword *));
        }
        wbptr = wback;
#endif
        if (heap == NULL) {

            /* The temporary string buffers are placed at the start of the
               heap, which permits us to pointer-check pointers into them
               as within the heap extents.  Hence, the size of the buffer
               we acquire for the heap is the sum of the heap and temporary
               string requests. */

            int i;
            char *cp;

            /* Force length of temporary strings to even number of
               stackitems. */
            atl_ltempstr += sizeof(stackitem) -
                (atl_ltempstr % sizeof(stackitem));

//            cp = alloc((((unsigned int) atl_heaplen) * sizeof(stackitem)) + ((unsigned int) (atl_ntempstr * atl_ltempstr)));
            int heapSize=((((unsigned int) atl_heaplen) * sizeof(stackitem)) + ((unsigned int) (atl_ntempstr * atl_ltempstr)));
            cp = alloc( heapSize );

            heapbot = (stackitem *) cp;
            strbuf = (char **) alloc(((unsigned int) atl_ntempstr) * sizeof(char *));

            for (i = 0; i < atl_ntempstr; i++) {
                strbuf[i] = cp;
                cp += ((unsigned int) atl_ltempstr);
            }
            cstrbuf = 0;
            heap = (stackitem *) cp;  /* Allocatable heap starts after
                                         the temporary strings */
        }
        /* The system state word is kept in the first word of the heap
           so that pointer checking doesn't bounce references to it.
           When creating the heap, we preallocate this word and initialise
           the state to the interpretive state. */
        hptr = heap + 1;
        state = Falsity;
#ifdef MEMSTAT
        heapmax = hptr;
#endif
        heaptop = heap + atl_heaplen;

        /* Now that dynamic memory is up and running, allocate constants
           and variables built into the system.  */

#ifdef FILEIO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
        {   static struct {
                              char *sfn;
                              FILE *sfd;
                          } stdfiles[] = {
                              {"STDIN", NULL},
                              {"STDOUT", NULL},
                              {"STDERR", NULL}
                          };
        int i;
        dictword *dw;

        /* On some systems stdin, stdout, and stderr aren't
           constants which can appear in an initialisation.
           So, we initialise them at runtime here. */

        stdfiles[0].sfd = stdin;
        stdfiles[1].sfd = stdout;
        stdfiles[2].sfd = stderr;

        for (i = 0; i < (int) ELEMENTS(stdfiles); i++) {
            if ((dw = atl_vardef(stdfiles[i].sfn,
                            2 * sizeof(stackitem))) != NULL) {
                stackitem *si = atl_body(dw);
                *si++ = FileSent;
                *si = (stackitem) stdfiles[i].sfd;
            }
        }
        }
#pragma GCC diagnostic pop
#endif /* FILEIO */
        dictprot = dict;	      /* Protect all standard words */
    }
    /*
    rf = atl_vardef("runflag",sizeof(int));
    if( rf != NULL) {
        *((int *) atl_body(rf)) = -1;
    }
    */
}

/*  ATL_LOOKUP	--  Look up a word in the dictionary.  Returns its
    word item if found or NULL if the word isn't
    in the dictionary. */

dictword *atl_lookup(char *name)
{
    V strcpy(tokbuf, name);	      /* Use built-in token buffer... */
    ucase(tokbuf);                    /* so ucase() doesn't wreck arg string */
    return lookup(tokbuf);	      /* Now use normal lookup() on it */
}

/*  ATL_BODY  --  Returns the address of the body of a word, given
    its dictionary entry. */

stackitem *atl_body(dictword *dw)
{
    return ((stackitem *) dw) + Dictwordl;
}

/*  ATL_EXEC  --  Execute a word, given its dictionary address.  The
    evaluation status for that word's execution is
    returned.  The in-progress evaluation status is
    preserved. */

int atl_exec(dictword *dw)
{
    int sestat = evalstat, restat;

    evalstat = ATL_SNORM;
#ifdef BREAK
    broken = False;		      /* Reset break received */
#endif
#undef Memerrs
#define Memerrs evalstat
    Rso(1);
    Rpush = ip; 		      /* Push instruction pointer */
    ip = NULL;			      /* Keep exword from running away */
    exword(dw);
    if (evalstat == ATL_SNORM) {      /* If word ran to completion */
        Rsl(1);
        ip = R0;		      /* Pop the return stack */
        Rpop;
    }
#undef Memerrs
#define Memerrs
    restat = evalstat;
    evalstat = sestat;
    return restat;
}

/*  ATL_VARDEF  --  Define a variable word.  Called with the word's
    name and the number of bytes of storage to allocate
    for its body.  All words defined with atl_vardef()
    have the standard variable action of pushing their
    body address on the stack when invoked.  Returns
    the dictionary item for the new word, or NULL if
    the heap overflows. */

dictword *atl_vardef( char *name, int size)
{
    dictword *di;
    int isize = (size + (sizeof(stackitem) - 1)) / sizeof(stackitem);

#undef Memerrs
#define Memerrs NULL
    evalstat = ATL_SNORM;
    Ho(Dictwordl + isize);
#undef Memerrs
#define Memerrs
    if (evalstat != ATL_SNORM)	      /* Did the heap overflow */
        return NULL;		      /* Yes.  Return NULL */
    createword = (dictword *) hptr;   /* Develop address of word */
    createword->wcode = P_var;	      /* Store default code */
    hptr += Dictwordl;		      /* Allocate heap space for word */
    while (isize > 0) {
        Hstore = 0;		      /* Allocate heap area and clear it */
        isize--;
    }
    V strcpy(tokbuf, name);	      /* Use built-in token buffer... */
    ucase(tokbuf);                    /* so ucase() doesn't wreck arg string */
    enter(tokbuf);		      /* Make dictionary entry for it */
    di = createword;		      /* Save word address */
    createword = NULL;		      /* Mark no word underway */
    return di;			      /* Return new word */
}

/*  ATL_MARK  --  Mark current state of the system.  */

void atl_mark(atl_statemark *mp)
{
    mp->mstack = stk;		      /* Save stack position */
    mp->mheap = hptr;		      /* Save heap allocation marker */
    mp->mrstack = rstk; 	      /* Set return stack pointer */
    mp->mdict = dict;		      /* Save last item in dictionary */
}

/*  ATL_UNWIND	--  Restore system state to previously saved state.  */

void atl_unwind(atl_statemark *mp)
{

    /* If atl_mark() was called before the system was initialised, and
       we've initialised since, we cannot unwind.  Just ignore the
       unwind request.	The user must manually atl_init before an
       atl_mark() request is made. */

    if (mp->mdict == NULL)	      /* Was mark made before atl_init ? */
        return; 		      /* Yes.  Cannot unwind past init */

    stk = mp->mstack;		      /* Roll back stack allocation */
    hptr = mp->mheap;		      /* Reset heap state */
    rstk = mp->mrstack; 	      /* Reset the return stack */

    /* To unwind the dictionary, we can't just reset the pointer,
       we must walk back through the chain and release all the name
       buffers attached to the items allocated after the mark was
       made. */

    while (dict != NULL && dict != dictprot && dict != mp->mdict) {
        free(dict->wname);	      /* Release name string for item */
        dict = dict->wnext;	      /* Link to previous item */
    }
}

#ifdef BREAK

/*  ATL_BREAK  --  Asynchronously interrupt execution.	Note that this
    function only sets a flag, broken, that causes
    exword() to halt after the current word.  Since
    this can be called at any time, it daren't touch the
    system state directly, as it may be in an unstable
    condition. */

void atl_break()
{
    broken = True;		      /* Set break request */
}
#endif /* BREAK */

/*  ATL_LOAD  --  Load a file into the system.	*/
#ifdef FILEIO
int atl_load(int fp)
{
    int es = ATL_SNORM;
    char s[134];
    atl_statemark mk;
    atl_int scomm = atl_comment;      /* Stack comment pending state */
    dictword **sip = ip;	      /* Stack instruction pointer */
    char *sinstr = instream;	      /* Stack input stream */
    int lineno = 0;		      /* Current line number */

    atl_errline = 0;		      /* Reset line number of error */
    atl_mark(&mk);
    ip = NULL;			      /* Fool atl_eval into interp state */
    while (atl_fgetsp(s, 132, fp) != NULL) {
        lineno++;
        if ((es = atl_eval(s)) != ATL_SNORM) {
            atl_errline = lineno;     /* Save line number of error */
            atl_unwind(&mk);
            break;
        }
    }
    /* If there were no other errors, check for a runaway comment.  If
       we ended the file in comment-ignore mode, set the runaway comment
       error status and unwind the file.  */
    if ((es == ATL_SNORM) && (atl_comment == Truth)) {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
        sprintf(outBuffer,"\nRunaway `(' comment.\n"); // EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
        es = ATL_RUNCOMM;
        atl_unwind(&mk);
    }
    atl_comment = scomm;	      /* Unstack comment pending status */
    ip = sip;			      /* Unstack instruction pointer */
    instream = sinstr;		      /* Unstack input stream */
    return es;
}
#endif
/*  ATL_PROLOGUE  --  Recognise and process prologue statement.
    Returns 1 if the statement was part of the
    prologue and 0 otherwise. */

int atl_prologue(const char *sp)
{
    static struct {
        const char *pname;
        atl_int *pparam;
    } proname[] = {
        {"STACK ", &atl_stklen},
        {"RSTACK ", &atl_rstklen},
        {"HEAP ", &atl_heaplen},
        {"TEMPSTRL ", &atl_ltempstr},
        {"TEMPSTRN ", &atl_ntempstr}
    };

    if (strncmp(sp, "\\ *", 3) == 0) {
        int i;
        char *vp = (char*)sp + 3, *ap;

        ucase(vp);
        for (i = 0; i < (int) ELEMENTS(proname); i++) {
            if (strncmp(sp + 3, proname[i].pname,
                        strlen(proname[i].pname)) == 0) {
                if ((ap = strchr(sp + 3, ' ')) != NULL) {
                    V sscanf(ap + 1, "%li", proname[i].pparam);
#ifdef PROLOGUEDEBUG
#ifdef EMBEDDED
                    sprintf(outBuffer,"Prologue set %sto %ld\n", proname[i].pname, *proname[i].pparam); // EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*  ATL_EVAL  --  Evaluate a string containing ATLAST words.  */

int atl_eval(const char *sp) {
    int i = 0;

#undef Memerrs
#define Memerrs evalstat
    instream =(char*) sp;
    evalstat = ATL_SNORM;	      /* Set normal evaluation status */
#ifdef BREAK
    broken = False;		      /* Reset asynchronous break */
#endif

    /* If automatic prologue processing is configured and we haven't yet
       initialised, check if this is a prologue statement.	If so, execute
       it.	Otherwise automatically initialise with the memory specifications
       currently operative. */

#ifdef PROLOGUE
    if (dict == NULL) {
        if (atl_prologue(sp))
            return evalstat;
        atl_init();
    }
#endif /* PROLOGUE */

    while ((evalstat == ATL_SNORM) && (i = token(&instream)) != TokNull) {
        dictword *di;

        switch (i) {
            case TokWord:
                if (forgetpend) {
                    forgetpend = False;
                    ucase(tokbuf);
                    if ((di = lookup(tokbuf)) != NULL) {
                        dictword *dw = dict;

                        /* Pass 1.  Rip through the dictionary to make sure
                           this word is not past the marker that
                           guards against forgetting too much. */

                        while (dw != NULL) {
                            if (dw == dictprot) {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
                                sprintf(outBuffer,"\nForget protected.\n"); //  EMBEDDED
#endif

     printf("%s",outBuffer);
#endif
                                evalstat = ATL_FORGETPROT;
                                di = NULL;
                            }
                            if (strcmp(dw->wname + 1, tokbuf) == 0)
                                break;
                            dw = dw->wnext;
                        }

                        /* Pass 2.  Walk back through the dictionary
                           items until we encounter the target
                           of the FORGET.  Release each item's
                           name buffer and dechain it from the
                           dictionary list. */

                        if (di != NULL) {
                            do {
                                dw = dict;
                                if (dw->wname != NULL)
                                    free(dw->wname);
                                dict = dw->wnext;
                            } while (dw != di);
                            /* Finally, back the heap allocation pointer
                               up to the start of the last item forgotten. */
                            hptr = (stackitem *) di;
                            /* Uhhhh, just one more thing.  If this word
                               was defined with DOES>, there's a link to
                               the method address hidden before its
                               wnext field.  See if it's a DOES> by testing
                               the wcode field for P_dodoes and, if so,
                               back up the heap one more item. */
                            if (di->wcode == (codeptr) P_dodoes) {
#ifdef FORGETDEBUG
#ifdef EMBEDDED
                                sprintf(outBuffer," Forgetting DOES> word. "); //  EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
                                hptr--;
                            }
                        }
                    } else {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
                        sprintf(outBuffer," '%s' undefined ", tokbuf); // EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
                        // evalstat = ATL_UNDEFINED;
                    }
                } else if (tickpend) {
                    tickpend = False;
                    ucase(tokbuf);
                    if ((di = lookup(tokbuf)) != NULL) {
                        So(1);
                        Push = (stackitem) di; /* Push word compile address */
                    } else {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
                        sprintf(outBuffer," '%s' undefined ", tokbuf); // EMBEDDED
#endif
     printf("%s",outBuffer);
#endif
                        evalstat = ATL_UNDEFINED;
                    }
                } else if (defpend) {
                    /* If a definition is pending, define the token and
                       leave the address of the new word item created for
                       it on the return stack. */
                    defpend = False;
                    ucase(tokbuf);
                    if (atl_redef && (lookup(tokbuf) != NULL)) {
#ifdef EMBEDDED
                        sprintf(outBuffer,"\n%s isn't unique.", tokbuf); // NOT EMBEDDED
#endif
     printf("%s",outBuffer);
                    }

                    enter(tokbuf);
                } else {
                    di = lookup(tokbuf);
                    if (di != NULL) {
                        /* Test the state.  If we're interpreting, execute
                           the word in all cases.  If we're compiling,
                           compile the word unless it is a compiler word
                           flagged for immediate execution by the
                           presence of a space as the first character of
                           its name in the dictionary entry. */
                        if (state &&
                                (cbrackpend || ctickpend ||
                                 !(di->wname[0] & IMMEDIATE))) {
                            if (ctickpend) {
                                /* If a compile-time tick preceded this
                                   word, compile a (lit) word to cause its
                                   address to be pushed at execution time. */
                                Ho(1);
                                Hstore = s_lit;
                                ctickpend = False;
                            }
                            cbrackpend = False;
                            Ho(1);	  /* Reserve stack space */
                            Hstore = (stackitem) di;/* Compile word address */
                        } else {
                            exword(di);   /* Execute word */
                        }
                    } else {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
                        sprintf(outBuffer," '%s' undefined\r\n", tokbuf); // EMBEDDED
#endif
                        printf("%s",outBuffer);
#endif
                        evalstat = ATL_UNDEFINED;
                        state = Falsity;
                    }
                }
                break;

            case TokInt:
                if (state) {
                    Ho(2);
                    Hstore = s_lit;   /* Push (lit) */
                    Hstore = tokint;  /* Compile actual literal */
                } else {
                    So(1);
                    Push = tokint;
                }
                break;

#ifdef REAL
            case TokReal:
                if (state) {
                    int i;
                    union {
                        atl_real r;
                        stackitem s[Realsize];
                    } tru;

                    Ho(Realsize + 1);
                    Hstore = s_flit;  /* Push (flit) */
                    tru.r = tokreal;
                    for (i = 0; i < Realsize; i++) {
                        Hstore = tru.s[i];
                    }
                } else {
                    int i;
                    union {
                        atl_real r;
                        stackitem s[Realsize];
                    } tru;

                    So(Realsize);
                    tru.r = tokreal;
                    for (i = 0; i < Realsize; i++) {
                        Push = tru.s[i];
                    }
                }
                break;
#endif /* REAL */

#ifdef STRING
            case TokString:
                if (stringlit) {
                    stringlit = False;
                    if (state) {
                        int l = (strlen(tokbuf) + 1 + sizeof(stackitem)) /
                            sizeof(stackitem);
                        Ho(l);
                        *((char *) hptr) = l;  /* Store in-line skip length */
                        V strcpy(((char *) hptr) + 1, tokbuf);
                        hptr += l;
                    } else {
#ifdef EMBEDDED
                        sprintf(outBuffer,"%s", tokbuf); // EMBEDDED
#endif
     printf("%s",outBuffer);
                    }
                } else {
                    if (state) {
                        int l = (strlen(tokbuf) + 1 + sizeof(stackitem)) /
                            sizeof(stackitem);
                        Ho(l + 1);
                        /* Compile string literal instruction, followed by
                           in-line skip length and the string literal */
                        Hstore = s_strlit;
                        *((char *) hptr) = l;  /* Store in-line skip length */
                        V strcpy(((char *) hptr) + 1, tokbuf);
                        hptr += l;
                    } else {
                        So(1);
                        V strcpy(strbuf[cstrbuf], tokbuf);
                        Push = (stackitem) strbuf[cstrbuf];
                        cstrbuf = (cstrbuf + 1) % ((int) atl_ntempstr);
                    }
                }
                break;
#endif /* STRING */
            default:
#ifdef EMBEDDED
                sprintf(outBuffer,"\nUnknown token type %d\n", i); // EMBEDDED
#endif
     printf("%s",outBuffer);
                break;
        }
    }
    return evalstat;
}
