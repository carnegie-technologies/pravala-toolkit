#pragma once

#if !defined(HAVE_RECVMMSG) && !defined(HAVE_SENDMMSG) && !defined(HAVE_MMSGHDR)

#include <sys/socket.h>

struct mmsghdr {
    struct msghdr   msg_hdr;
    unsigned int    msg_len;
};

#endif
