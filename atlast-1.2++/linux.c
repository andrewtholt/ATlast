#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>


#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */


// #include "atldef.h"
#ifdef PUBSUB
#include "msgs.h"
#endif

// Move this into atlast once updated frm FreeRTOS version
//

#ifdef __cplusplus
extern "C" {
#endif
#include "atldef.h"

extern prim P_here();
extern prim P_swap();
extern prim P_allot();

prim ATH_initRamBlocks() {
    int size;
    Sl(1);
    So(2);

    P_here();
    P_swap();

    size=S0 * 1024;
    S0=size;

    P_allot();

    memset((void *)S0, ' ', size);
}

prim crap() {
    printf("Hello\n");
}
//
// <ptr> name -- ptr
//
prim ATH_getenv() {
    Sl(2); // On entry will use this many.
    So(1); // on exit will leave this many.

    char *name=(char *)S0;
    char *ptr=(char *)S1;
    char *tmp;

    Pop2;

    tmp = getenv(name);
    if(!tmp) {
        Push=-1;
    } else {
        strcpy(ptr, tmp);
        Push=(stackitem)ptr;
        Push=0;
    }
}

#ifdef SYSVIPC
// name -- fd
prim ATH_shmOpen() {
    int shmFd;
    Sl(1);
    So(2);
    
    shmFd=shm_open( (char *)S0, O_CREAT | O_RDWR, 0600 );  // only the owners processes can access
    
    if(shmFd < 0) {
        S0 = true;
    } else {
        S0 = shmFd;
        Push=false ;
    }
}

// file_descriptor size ---
prim ATH_shmSize() {
    off_t length;
    int shmFd;
    
    Sl(2);
    
    length = (off_t) S0;
    shmFd = (int)S1;
    
    /* configure the size of the shared memory segment */
	ftruncate(shmFd,length);
    Pop2;
}
#endif

// fd size -- ptr

prim ATH_mmap() {
    void *ptr;
    Sl(2);
    So(1);
    
    
	ptr = mmap(0,S0, PROT_READ | PROT_WRITE, MAP_SHARED, S1, 0);
    Pop;
    S0=(stackitem)ptr;
}

// msg -- 
prim ATH_perror() {
    
    Sl(1);
    So(0);

    perror((char *)S0);
    errno=0;
    Pop;
}

// Stack : addr len fd -- actual
//
prim P_fdRead() {
    char *data = (char *)S2;
    int len=(int)S1;
    int fd = (int)(S0);
    Pop2;

    bzero(data,len);

    ssize_t actual = read(fd,data,len);

    S0=actual;

}
//
// Stack : addr len fd -- actual
//
prim P_fdWrite() {
    char *data = (char *)S2;
    int len=(int)S1;
    int fd = (int)(S0);
    Pop2;

    ssize_t actual = write(fd,data,len);

    S0=actual;
}

prim P_strstr() {
    char *needle=(char *)S0;
    char *haystack=(char *)S1;
    Pop;

    char *res = strstr(haystack, needle);
    S0=res;
}

prim P_strcasestr() {
    char *needle=(char *)S0;
    char *haystack=(char *)S1;
    Pop;

    char *res = strcasestr(haystack, needle);
    S0=res;
}

// Stack: libname -- handle 
prim ATH_dlopen() {
    Sl(1);
    So(1);

    char *libname=(char *)S0;

    void *ret=dlopen(libname, RTLD_LAZY);

    S0 = ret;
}

// Stack ( symbol_name handle -- addr )
prim ATH_dlsym() {
    Sl(2);
    So(1);

    char *symName = (char *)S1;
    void *handle = (void *)S0;
    Pop;

    void *ret = dlsym(handle, symName);

    S0 = ret;
}

// Stack: argN ... arg1 N fn  -- ret 
prim ATH_ccall() {
    So(1);
    int i;
    int arg[8];
    int ret;

    int (*fn)();

    fn = (int (*)()) S0;
    int narg=(int)S1;
    Pop2;

    for (i = 0; i < narg; i++) {
        arg[i] = S0;
        Pop;
    }

    switch (narg) {
        case 0: ret = (*fn)(); break;
        case 1: ret = (*fn)(arg[0]); break;
        case 2: ret = (*fn)(arg[0], arg[1]); break;
        case 3: ret = (*fn)(arg[0], arg[1], arg[2]); break;
        case 4: ret = (*fn)(arg[0], arg[1], arg[2], arg[3]); break;
        case 5: ret = (*fn)(arg[0], arg[1], arg[2], arg[3], arg[4]); break;
        case 6: ret = (*fn)(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]); break;
        case 7: ret = (*fn)(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]); break;
        default:
                break;
    }
    Push=S0;
}
    
static struct primfcn extras[] = {
    {"0FD-READ", P_fdRead},
    {"0FD-WRITE", P_fdWrite},
    {"0STRSTR", P_strstr},
    {"0STRCASESTR", P_strcasestr},
    {"0INIT-RAM", ATH_initRamBlocks},
    {"0GETENV", ATH_getenv},
    {"0MMAP", ATH_mmap},
    {"0DLOPEN", ATH_dlopen},
    {"0DLSYM", ATH_dlsym},
    {"0(CCALL)", ATH_ccall},
#ifdef SYSVIPC
    {"0SHM-SIZE", ATH_shmSize},
    {"0SHM-OPEN", ATH_shmOpen},
#endif
    {"0PERROR", ATH_perror},
    {"0TESTING", crap},
    {NULL, (codeptr) 0}
};

void extrasLoad() {
    atl_primdef( extras );
}

#ifdef __cplusplus
}
#endif
