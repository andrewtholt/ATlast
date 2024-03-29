#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "atlast.h"
#include "small-linux-atlcfig.h"
// #include "atlcfig.h"
#include "atldef.h"

// uint8_t nvramrc[] = ": tst \n10 0 do \ni . cr \nloop \n; \n \n";
// uint8_t nvramrc[] = ": sifting\ntoken $sift\n;\n: [char] char ;";
// uint8_t nvramrc[] = "" ;
//

#include "nvramrc.h"

char outBuffer[OUTBUFFER];

int main() {
    char t[132];
    int8_t len;

    bool runFlag=true;

    uint8_t lineBuffer[MAX_LINE];

    dictword *start;
    dictword *stop;
    dictword *motor;
    dictword *tst;

    atl_init();

    FILE *fp = fopen((char *)"utils.atl","r");
    int stat = atl_load(fp);

    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval(lineBuffer);
    } while(len >= 0);

    start = atl_vardef("START",1);
    *((int *)atl_body(start))=0;

    stop = atl_vardef("STOP",1);
    motor = atl_vardef("MOTOR",1);

    tst = atl_lookup((char *)"START");

    /*
    if(var == NULL) {
        fprintf(stderr,"Vardef failed\n");
    } else {
        *((int *)atl_body(var))=42;
    }
    */

    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        printf("-> ");

        (void)fgets(t,132,stdin);
        atl_eval(t);

    }
    return 0;
}
