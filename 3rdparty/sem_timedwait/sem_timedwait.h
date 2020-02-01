
#pragma once

extern "C"
{
#include <semaphore.h>

// On some systems sem_timedwait is not implemented, so use a custom implementation. See sem_timedwait.c
int sem_timedwait ( sem_t * sem, const struct timespec * abs_timeout );
}
