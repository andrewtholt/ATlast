/*			      ATLMAIN.C

                  Main driver program for interactive ATLAST

                  Designed and implemented in January of 1990 by John Walker.

                  This program is in the public domain.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "atldef.h"
#include "atlcfig.h"
#include <inttypes.h>

#ifdef PUBSUB
#warning "PubSub defined"
#include <Small.h>
#include "linuxParser.h"
#include "msgs.h"
#include <errno.h>

#ifdef PTHREAD
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#endif
#endif

#define FALSE	0
#define TRUE	1

#define V   (void)

/*  Globals imported  */

#ifdef PUBSUB
#ifdef PTHREAD
#warning "Define lock"
pthread_mutex_t lock;

pthread_t tid[2];
struct Small *table;
char *queueName ;

void doSmallCallback(struct nlist *rec, uint8_t idx) {
    struct client *tmp;
    void *fred;

    printf("doSmallCallback\n");

    tmp=(struct client *)nlistGetSubscriber(rec,idx);

    if(tmp != 0) {
        struct cmdMessage subMessage;

        memset(&subMessage,0,sizeof(struct cmdMessage));

        subMessage.payload.message.fields = 3;
        strncpy(subMessage.sender,queueName,MAX_SUB_NAME);
        strncpy(subMessage.payload.message.cmd,"SET",MAX_CMD);
        strncpy(subMessage.payload.message.key,nlistGetName(rec),MAX_KEY);
        strncpy(subMessage.payload.message.value,nlistGetDef(rec),MAX_VALUE);

        int rc=mq_send(tmp->pipe,&subMessage,sizeof(struct cmdMessage),NULL);
    }
}

void *doSmall(void *arg) {
    bool runFlag=true;
//    char *queueName ;
    mqd_t mq;
    struct mq_attr attr;
    ssize_t len;
    struct cmdMessage buffer;
    //    char *res;

    struct linuxParser *p;

    bool ff;
    struct Small *myTable=arg;

    // This lock is held by this threads parent.
    // Once the parent has completed its setup it will release the lock, and
    // then we can continue.
    //
    pthread_mutex_lock(&lock);
    fprintf(stderr,"Started\n");

    queueName = dbLookup(myTable,"QNAME");
    if(!queueName) {
        ff=addRecord(myTable,"QNAME","/atlast");
        if(ff) {
            fprintf(stderr,"db install faulure, fatal error\n");
            exit(1);
        }
        queueName = dbLookup(myTable,"QNAME");
    }

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    printf("size %d\n", sizeof(struct cmdMessage));
    attr.mq_msgsize = sizeof(struct cmdMessage);
    attr.mq_curmsgs = 0;

    mq = mq_open(queueName, O_CREAT | O_RDONLY, 0644, &attr);
    if( (mqd_t)-1 == mq) {
        perror("mq_open");
        if(errno == ENOSYS) {
            fprintf(stderr,"Fatal ERROR\n");
            exit(1);
        }
    }

    mq_setattr(mq, &attr,NULL);

    p=newParser(myTable);

    setIam(p,queueName);

    while(runFlag) {
        len = mq_receive(mq, &buffer, sizeof(buffer), NULL);
        if( len < 0) {
            perror("mq_receive");
        } else {
            ff=cmdParse(p,&buffer,NULL);
        }
    }

}
#endif
#endif

/*  MAIN  --  Main program.  */

int main(int argc, char *argv[]) {
    int i;
    int fname = FALSE, defmode = FALSE;
    FILE *ifp;
    char *include[20];
    int in = 0;

    int *tst;

#define PR(x) (void) fprintf(stderr, x)

#ifdef BANNER
    PR("ATLAST 1.2 (2007-10-07) This program is in the public domain.\n");
    printf("Compiled: %s %s\n",__DATE__,__TIME__);
#endif

    ifp = stdin;

    int opt;

    while((opt = getopt(argc,argv, "C:DH:I:R:S:T?U")) != -1) {
        switch(opt) {
            case 'C':
                chdir(optarg);
                break;
            case 'D':
                defmode = TRUE;
                break;
            case 'H':
                atl_heaplen = atol(optarg);
                break;
            case 'I':
                include[in]=(char *)malloc(strlen(optarg)+1);
                strncpy(include[in++], optarg, strlen(optarg));
                break;
            case 'R':
                atl_rstklen = atol(optarg);
                break;

            case 'S':
                atl_stklen = atol(optarg);
                break;

            case 'T':
                atl_trace = TRUE;
                break;
            case '?':
            case 'U':
                PR("Usage:  ATLAST [options] [inputfile]\n");
                PR("        Options:\n");
                PR("           -C <dir>  Change to this directory\n");
                PR("           -D        Treat file as definitions\n");
                PR("           -Hn       Heap length n\n");
                PR("           -I <file> Include named definition file\n");
                PR("           -R n      Return stack length n\n");
                PR("           -S n      Stack length n\n");
                PR("           -T        Set TRACE mode\n");
                PR("           -U        Print this message\n");
                return 0;
        }
    }
    // OK, so init the system
    //
    atl_init();
#ifdef LINUX
#ifdef EXTRAS
    extern void extrasLoad();
    extrasLoad();
#endif
#endif

#ifdef MQTT
    extern void mqttLoad();
    mqttLoad();
#endif

    int8_t len=0;
    uint8_t lineBuffer[MAX_LINE];
    char t[132];
#ifdef PUBSUB
    table = newSmall();
#ifdef PTHREAD
#warning Pthreads
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }


#endif
#ifdef LINUX
    sprintf(t,"0x%x constant TABLE",table);
    sprintf(t,"0x%" PRIx64 " constant TABLE",table);

    atl_eval(t);
    memset(t,0x00,sizeof(t));
#endif
    pthread_mutex_lock(&lock);
    int err = pthread_create(&(tid[0]), NULL, &doSmall, (void *)table);


#endif


    /* If any include files were named, load each in turn before
       we execute the program. */

    for (i = 0; i < in; i++) {
        int stat;
        char fn[132];
        FILE *fp;

        V strcpy(fn, include[i]);
        if (strchr(fn, '.') == NULL)
            V strcat(fn, ".atl");
        fp = fopen(fn,
#ifdef FBmode
                "rb"
#else
                "r"
#endif
                );
        if (fp == NULL) {
            V fprintf(stderr, "Unable to open include file %s\n",
                    include[i]);
            return 1;
        }
        stat = atl_load(fp);
        V fclose(fp);
        if (stat != ATL_SNORM) {
            V printf("\nError %d in include file %s\n", stat, include[i]);
        }
    }

    /* Now that all the preliminaries are out of the way, fall into
       the main ATLAST execution loop. */
    while (true) {

        if (!fname)
            V printf(atl_comment ? "(  " :  /* Show pending comment */
                    /* Show compiling state */
                    (((heap != NULL) && state) ? ":> " : "-> "));
        if (fgets(t, 132, ifp) == NULL) {
            if (fname && defmode) {
                fname = defmode = FALSE;
                ifp = stdin;
                continue;
            }
            break;
        }
        V atl_eval(t);
    }
    if (!fname)
        V printf("\n");
    return 0;
}
