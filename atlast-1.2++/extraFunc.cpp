#include <vector>
#include <string>
#include <iostream>

#include <stdio.h>
#include <string.h>

#include "atldef.h"

using namespace std;
#define RR_STRING 255
#define RR_NUM 4

char rrString[RR_NUM][RR_STRING];

static int rrIdx = 0;

extern atl_int ath_safe_memory;

prim crap() {
    printf("Hello\n");
}

prim P_newVector() {
//    vector<void *> *tst = new vector<void *>;
    vector<string> *tst = new vector<string>(0);
    Push = tst;
}

prim P_addToBack() {
//    vector<void *> *tst ;
    vector<string> *tst ;
    char *value;

    Sl(2);
    Sl(0);

    tst=( vector<string> *)S0;
    value=(char *)S1;

    string tmp = string(value);

    tst->push_back(tmp);
    Pop2;
}

prim P_takeFromFront() {
    vector<string> *tst ;
    char *value;

    Sl(1);
    So(1);

    tst=( vector<string> *)S0;
    string tmp = tst->front();
    tst->erase(tst->begin());

    strncpy(rrString[rrIdx], tmp.c_str(), RR_STRING);

    S0 = (char *) &rrString[rrIdx];
    rrIdx = (rrIdx + 1) % RR_NUM;

}
prim P_vectorSize() {
    vector<string> *tst ;
    char *value;

    Sl(1);
    So(1);

    tst=( vector<string> *)S0;

    S0=(int)tst->size();
}

prim P_newString() {
    // 
    // Access to string means return pointers outside the heap.
    // In order for things like type to work I need to disable this.
    // Caveat Emptor
    //
//    ath_safe_memory = 0;
    Sl(0);
    So(1);

    string *str = new string();
    Push=str;
}

prim P_setString() {
    Sl(2);
    Sl(0);

    string *str = S0;
    char *data = (char *)S1;
    Pop2;

    str->assign( data );
}

prim P_getString() {
    Sl(1);
    So(1);

    char *tmp;

    string *str = S0;

    tmp = str->c_str();

    S0=tmp;
}

prim P_typeString() {
    Sl(1);
    So(1);

    string *str = S0;

    cout << *str ;

    Pop;
}

prim P_stringToCstr() {
    Sl(2);
    Sl(0);

    string *str = S0;
    char *out = (char *)S1;

    Pop2;

    strcpy(out, str->c_str());
}

prim P_appendString() {
    Sl(2);
    Sl(0);

    string *add = S1;
    string *base = S0;

    *add += *base;

    cout << *add << endl;

}


static struct primfcn cpp_extras [] = {
    {"0TESTING", crap},
    {"0NEW-VECTOR",P_newVector},
    {"0TO-BACK",P_addToBack},
    {"0FROM-FRONT",P_takeFromFront},
    {"0VECTOR-SIZE",P_vectorSize},
    {"0NEW-STRING",P_newString},
    {"0STRING!",P_setString},
    {"0STRING@",P_getString},
    {"0STRING-TYPE",P_typeString},
    {"0STRING-APPEND",P_appendString},
    {"0STOC",P_stringToCstr},
    {NULL, (codeptr) 0}
};

void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

