#pragma once

// This is NOT the same as the normal libc version - as this doesn't support being a cancellation point!

int sendmmsg(int fd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags);
