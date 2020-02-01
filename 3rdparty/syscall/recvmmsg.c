
#include <sys/socket.h>
#include "mmsghdr.h"
#include "time.h"
#include "syscall.h"

// This is NOT the same as the normal libc version - as this doesn't support being a cancellation point!

int recvmmsg(int fd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags, struct timespec *timeout)
{
    return syscall(__NR_recvmmsg, fd, msgvec, vlen, flags, timeout);
}
