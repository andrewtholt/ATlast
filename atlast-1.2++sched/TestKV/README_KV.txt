
This feature requires "VerySmallDatabase" to be built and installed and findable via LD_LIBRARY_PATH



Words to access key value database

Name:   kv-init
Stack:  -- *kv
Desc:
    Create the structures.

Name:   kv-set
Stack:  *kv "key" "value" --
Desc:
    Add data.

Name:   kv-get
Stack:  *kv "key" --- F|T
Description:
    Returns 0 if the key is found and the value string at pad.

Name:   kv-display
Stack:  *kv --
Description:
    Display all the entries in the kv store.

Name:   kv-dump
Stack:  *kv "filename"-- F|T
Desc:
    Write each kv pair to thae file. Return False on success.

Name:   kv-load
Stack:  *kv "filename"-- F|T
Desc:
    Read the contents from the file.  Retuen False on success.

Name:   kv-close
Stack:  *kv ---
Desciption:
    Release the memory use by the store.

