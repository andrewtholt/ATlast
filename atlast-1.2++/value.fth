
variable flag 
-1 flag !


: value    constant ;

: value!   >body ! ;

: to
    state @ if
        1 flag !
        [compile] ['] [compile] value!
    else
        0 flag !
        ' value!
    then 
; immediate


0 value fred

