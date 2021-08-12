#include <stdio.h>
#include "client.h"
void *returntask(void * arg)
{
    struct client_status *t=(struct client_status*)(void*)arg;
    t->fin=1;
    return NULL;
}