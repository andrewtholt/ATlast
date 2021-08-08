## MK-STRING-LIST

###### --- list

Create an empty C++ list for strings,

## RM-STRING-LIST

###### list --

Delete an exist C++ string list



## STRING-LIST-SIZE

###### list -- size

Return the number of list members



## STRING-LIST-DUMP

###### list --

Show the contents of a list,



###### STRING-PUSH-FRONT

###### list ptr --

Put the string pointed to by ptr at the front of the list.



## STRING-POP-FRONT

###### list ptr --

Take the string from the front of the list and copy it the memory pointed to by ptr.



## STRING-PUSH-BACK

###### list ptr -

Put the string pointed to by ptr at the back of the list.



## STRING-POP-BACK

###### list ptr --

Take the string from the back of the list and copy it the memory pointed to by ptr.

## STRING-LIST-EMPTY?

###### list -- bool

Return true if the list has no members



## MK-STRING-MAP

###### -- map

Create a new, empty map of strings <string,string>



## RM-STRING-MAP

###### map --

Remove a map.



## MAP-DUMP

###### map --

Display the members of the given list.



## MAP-ADD

###### map key value --

Add the key/value pair to the map

## MAP-RM

###### map key --

Remove the key and value from the map.



###### JSON-SEARCH (Experimental )

###### json key buffer --

Find the key in the json, and copy the value to the buffer.
