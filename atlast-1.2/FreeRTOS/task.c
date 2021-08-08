#include "atlsrc.h"

void atlastThread(void const *args) {
    uint8_t buff[80];
    //	int i=0;

    static struct taskData myData;
    memset((void *)&myData,0,sizeof(struct taskData));

    outBuffer=(char *)malloc(OUT_SIZE);
    memset(outBuffer,0,OUT_SIZE);


    myData.iam=osMessageCreate(osMessageQ(atlastQ),NULL);
    // myData.lock = xSemaphoreCreateMutex();

    task[TST_HARNESS] = &myData;

    txBuffer(&huart2, (uint8_t *)"\r\n");
    atl_init();

#ifdef BANNER
    txBuffer(&huart2, (uint8_t *)"\r\nBased on ATLAST 1.2 (2007-10-07)\n");
    txBuffer(&huart2, (uint8_t *)"The original version of this program is in the public domain.\n");
    txBuffer(&huart2, (uint8_t *)"Modifications are the property of Elcometer Ltd.\n");
    sprintf(outBuffer,"Compiled: %s\n",__DATE__);

    txBuffer(&huart2, (uint8_t *)outBuffer);

#endif
    extern dictword *rf;
    Boolean *runFlag;
    int8_t len=0;

    uint8_t lineBuffer[MAX_LINE];

    runFlag = (Boolean *) atl_body( rf );

    // This only works if the enumeration increases in value from the first to the last

    sprintf(outBuffer,"%d constant NO_TASK", NO_TASK);
    atl_eval(outBuffer);

    sprintf(outBuffer,"%d constant TST_HARNESS",TST_HARNESS);
    atl_eval(outBuffer);

    sprintf(outBuffer,"%d constant TST_RX",TST_RX);
    atl_eval(outBuffer);

    sprintf(outBuffer,"%d constant LAST_TASK",LAST_TASK);
    atl_eval(outBuffer);

    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval(lineBuffer);
    } while(len >=0);

    while(true) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        txBuffer(&huart2, (uint8_t *)"--> ");
        (void)rxLine(console , buff, 80,true );
        atl_eval(buff);
        txBuffer(&huart2, (uint8_t *)"\r\n");
    }
}
