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
// 
// Stack : -- vector
//
prim P_newVector() {
    Sl(0);
    So(1);

    vector<string> *tst = new vector<string>(0);
    Push = tst;
}
// 
// Stack : string vector --
//
prim P_addToBack() {
    //    vector<void *> *tst ;
    vector<string> *tst ;
    char *value;

    Sl(2);
    So(0);

    tst=( vector<string> *)S0;
    value=(char *)S1;

    string tmp = string(value);

    tst->push_back(tmp);
    Pop2;
}
// 
// Stack : vector -- string
//
prim P_takeFromFront() {
    Sl(1);
    So(1);

    vector<string> *tst ;
    char *value;

    string *tmp = new string;

    tst=( vector<string> *)S0;

    tmp->assign(tst->front());
    tst->erase(tst->begin());

    /*
       strncpy(rrString[rrIdx], tmp.c_str(), RR_STRING);

       S0 = (char *) &rrString[rrIdx];
       rrIdx = (rrIdx + 1) % RR_NUM;
       */
    S0 = tmp;

}
// 
// Stack: vector -- int 
//
prim P_vectorSize() {
    vector<string> *tst ;
    char *value;

    Sl(1);
    So(1);

    tst=( vector<string> *)S0;

    S0=(int)tst->size();
}
// 
// Stack: -- string
//
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
// 
// Stack : addr string --
//
prim P_setString() {
    Sl(2);
    Sl(0);

    string *str = S0;
    char *data = (char *)S1;
    Pop2;

    str->assign( data );
}
// 
// string -- addr
//
prim P_getString() {
    Sl(1);
    So(1);

    char *tmp;

    string *str = S0;

    tmp = str->c_str();

    S0=tmp;
}
// 
// Stack : string -- 
//
prim P_typeString() {
    Sl(1);
    So(1);

    string *str = S0;

    cout << *str ;

    Pop;
}
// 
// Stack: string -- addr
//
prim P_stringToCstr() {
    Sl(2);
    Sl(0);

    string *str = S0;
    char *out = (char *)S1;

    Pop2;

    strcpy(out, str->c_str());
}
// 
// Stack: string -- int
//
prim P_stringToInt() {
    Sl(1);
    So(1);

    int out;
    string *str = S0;

    out = stoi(*str,nullptr,0);

    S0 = out;
}
// 
// Stack : int -- string
//
prim P_intToString() {
    Sl(2);
    So(1);

    int in = (int)S1;
    string *str = S0;
    Pop2;

    *str = to_string(in);
}
// 
// Stack string1 string2 -- string1+string2
//
prim P_appendString() {
    Sl(2);
    Sl(0);

    string *add = S1;
    string *base = S0;

    *add += *base;
}

vector<string> *split(string *str, char c = ' ') {

    int idx=0;
    int noMatch=0;
    int start=0;

    vector<string> *result = new vector<string>(0);

    int stringLen = str->size();

    string tmp;

    for(idx=0; idx < str->size(); idx++) {  
        if( str->at(idx) == c ) {
            tmp = str->substr( start, noMatch );
            noMatch = 1 ;
            idx++;
            start=idx;

            result->push_back(tmp);
        } else {
            noMatch++;
        }
    }
    if ( start != stringLen) {
        tmp = str->substr( start, noMatch );
        result->push_back(tmp);
    }

    return result;
}
// 
// Take each member string of the vector, in turn and join them with the seperator
// between.
//
// NOTE: If the seperator is 0 (NULL) then the elements are simply concatnated.
// e.g.
//
// Vector contain "Hello","world"
//
// vector char " " vector-join 
//
// results in a string:
//
// "Hello world"
//
// Stack: vector sep -- string 
//
prim P_vectorJoin() {
    string *res = new string();;
    string sep;

    int i=1;

    vector<string> *v = S1;
    char c = S0;

    if (c != 0) {
        string sep(1,c);
    }
    Pop;

    int vecLen = v->size();

    for (const auto &s: *v) {
        res->append(s);

        if( c !=0) {
            if ( i != vecLen) {
                res->append(sep);
            }
        }
        i++;
    }

    S0=res;
}

prim P_vectorAt() {
    Sl(1);
    So(1);

    vector<string> *v = S0;
    int idx = S1;
    Pop;

    string *res = new string();;

    *res = v->at(idx);
    S0 = res;
}
// 
// Stack: string sep -- vector
//
prim P_splitString() {
    Sl(2);
    So(1);

    char c = S0;
    string *str = S1;
    Pop;

    vector<string> *result= split( str, c );
    S0=result;
}

inline string *ltrim(string *s, const char* t = " \t\n\r\f\v") {
    s->erase(0, s->find_first_not_of(t));
    return s;
}

inline string *rtrim(string *s, const char* t = " \t\n\r\f\v") {
    s->erase(s->find_last_not_of(t) + 1);
    return s;
}

string *trim(string *s, const char* t = " \t\n\r\f\v") {
    return ltrim(rtrim(s, t), t);
}
// 
// Stack: string --
//
prim P_trimStringLeft() {

    string *tmp;
    Sl(1);
    So(1);

    string *str = S0;
    tmp = ltrim(str);

    S0 = tmp;
}
// 
// Stack: string --
//
prim P_trimStringRight() {

    string *tmp;
    Sl(1);
    So(1);

    string *str = S0;
    tmp = rtrim(str);

    S0 = tmp;
}
// 
// Stack: string --
//
prim P_trimString() {

    string *tmp;
    Sl(1);
    So(1);

    string *str = S0;
    tmp = trim(str);

    S0 = tmp;
}

static struct primfcn cpp_extras [] = {
    {"0TESTING", crap},
    {"0NEW-VECTOR",P_newVector},
    {"0TO-BACK",P_addToBack},
    {"0FROM-FRONT",P_takeFromFront},
    {"0VECTOR-SIZE",P_vectorSize},
    {"0VECTOR-JOIN",P_vectorJoin},
    {"0VECTOR@",P_vectorAt},

    {"0NEW-STRING",P_newString},
    {"0STRING!",P_setString},
    {"0STRING@",P_getString},
    {"0STRING-TYPE",P_typeString},
    {"0STRING-APPEND",P_appendString},
    {"0STRING-SPLIT",P_splitString},
    {"0STRING-LEFT-TRIM",P_trimStringLeft},
    {"0STRING-RIGHT-TRIM",P_trimStringRight},
    {"0STRING-TRIM",P_trimString},

    {"0STOC",P_stringToCstr},
    {"0STOI",P_stringToInt},
    {"0ITOS",P_intToString},
    {NULL, (codeptr) 0}
};

void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

