BEGIN {
    FS="="
}

{
    len=length($0)
    if( len > 0) {
        key=$1
        val=$2
        printf("0 value %s\n",key)
        printf("mk-string-map to %s\n", key)
        printf("\"%s\" %s\n",val,key)
    }
}

