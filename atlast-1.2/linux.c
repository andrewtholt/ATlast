#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>


#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */


#include "atldef.h"
#ifdef PUBSUB
#include "msgs.h"
#endif

// Move this into atlast once updated frm FreeRTOS version
//

extern prim P_here();
extern prim P_swap();
extern prim P_allot();

#ifdef PUBSUB
void mkMsg(void *from, struct cmdMessage *msg, char *cmd, char *key, char *value) {

    memset(msg, 0, sizeof(struct cmdMessage));
    msg->payload.message.fields=3;

    strncpy(msg->sender,(char *)S1, SENDER_SIZE);

    strncpy(msg->payload.message.cmd,cmd,sizeof(msg->payload.message.cmd));
    if( value == NULL) {
        msg->payload.message.fields=2;
        msg->payload.message.value[0]='\0';
    } else {
        strncpy(msg->payload.message.value, value, sizeof(msg->payload.message.value));
    }
    if( key == NULL) {
        msg->payload.message.fields=1;
        msg->payload.message.key[0]='\0';
        msg->payload.message.value[0]='\0';
    } else {
        strncpy(msg->payload.message.key, key, sizeof(msg->payload.message.key));
    }

}
#endif

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

// <ptr> name -- ptr
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
    
static struct primfcn extras[] = {
    {"0INIT-RAM", ATH_initRamBlocks},
    {"0GETENV", ATH_getenv},
    {"0MMAP", ATH_mmap},
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
