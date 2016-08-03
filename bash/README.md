## rm
This is a safety replacement of /bin/rm. It mv files or directories which you want to delete into the trash. You could place this script into the $HOME/bin directory, and place $HOME/bin before /bin in PATH.

## tcmalloc.gdb
Debug a process with tcmalloc which symbols are stripped and print key structures of tcmalloc.
In gdb interactive mode, source this script and use functions below to print stuctures.

#### pthread_cache
Print the tcmalloc ThreadCache. First of all, you need to get the pointer of ThreadCache.

Usage:

   pthread_cache $address_of_ThreadCache

#### pfree_list

#### plist