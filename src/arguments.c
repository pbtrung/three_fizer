#include "include/arguments.h"

void initArguments(arguments* arg)
{
    arg->encrypt = true;
    arg->free = false;
    arg->hash_from_file = false;
    arg->rename = false;
    arg->hash = true;
    arg->target_file = NULL;
    arg->state_size = Skein512;
    arg->key_file = NULL;
    arg->password = NULL;
    arg->pw_length = 0;
    arg->file_size = 0;
}
