/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "io-util.h"
#include "string-util.h"
#include "time-util.h"

int flush_fd(int fd) {
        int count = 0;

        /* Read from the specified file descriptor, until POLLIN is not set anymore, throwing away everything
         * read. Note that some file descriptors (notable IP sockets) will trigger POLLIN even when no data can be read
         * (due to IP packet checksum mismatches), hence this function is only safe to be non-blocking if the fd used
         * was set to non-blocking too. */

        for (;;) {
                char buf[LINE_MAX];
                ssize_t l;
                int r;

                r = fd_wait_for_event(fd, POLLIN, 0);
                if (r < 0) {
                        if (r == -EINTR)
                                continue;

                        return r;
                }
                if (r == 0)
                        return count;

                l = read(fd, buf, sizeof(buf));
                if (l < 0) {
                        if (errno == EINTR)
                                continue;

                        if (errno == EAGAIN)
                                return count;

                        return -errno;
                } else if (l == 0)
                        return count;

                count += (int) l;
        }
}

ssize_t loop_read(int fd, void *buf, size_t nbytes, bool do_poll) {
        uint8_t *p = buf;
        ssize_t n = 0;

        assert(fd >= 0);
        assert(buf);

        /* If called with nbytes == 0, let's call read() at least
         * once, to validate the operation */

        if (nbytes > (size_t) SSIZE_MAX)
                return -EINVAL;

        do {
                ssize_t k;

                k = read(fd, p, nbytes);
                if (k < 0) {
                        if (errno == EINTR)
                                continue;

                        if (errno == EAGAIN && do_poll) {

                                /* We knowingly ignore any return value here,
                                 * and expect that any error/EOF is reported
                                 * via read() */

                                (void) fd_wait_for_event(fd, POLLIN, USEC_INFINITY);
                                continue;
                        }

                        return n > 0 ? n : -errno;
                }

                if (k == 0)
                        return n;

                assert((size_t) k <= nbytes);

                p += k;
                nbytes -= k;
                n += k;
        } while (nbytes > 0);

        return n;
}

int loop_read_exact(int fd, void *buf, size_t nbytes, bool do_poll) {
        ssize_t n;

        n = loop_read(fd, buf, nbytes, do_poll);
        if (n < 0)
                return (int) n;
        if ((size_t) n != nbytes)
                return -EIO;

        return 0;
}

int loop_write(int fd, const void *buf, size_t nbytes, bool do_poll) {
        const uint8_t *p = buf;

        assert(fd >= 0);
        assert(buf);

        if (_unlikely_(nbytes > (size_t) SSIZE_MAX))
                return -EINVAL;

        do {
                ssize_t k;

                k = write(fd, p, nbytes);
                if (k < 0) {
                        if (errno == EINTR)
                                continue;

                        if (errno == EAGAIN && do_poll) {
                                /* We knowingly ignore any return value here,
                                 * and expect that any error/EOF is reported
                                 * via write() */

                                (void) fd_wait_for_event(fd, POLLOUT, USEC_INFINITY);
                                continue;
                        }

                        return -errno;
                }

                if (_unlikely_(nbytes > 0 && k == 0)) /* Can't really happen */
                        return -EIO;

                assert((size_t) k <= nbytes);

                p += k;
                nbytes -= k;
        } while (nbytes > 0);

        return 0;
}

int pipe_eof(int fd) {
        int r;

        r = fd_wait_for_event(fd, POLLIN, 0);
        if (r <= 0)
                return r;

        return !!(r & POLLHUP);
}

int ppoll_usec(struct pollfd *fds, size_t nfds, usec_t timeout) {
        struct timespec ts;
        int r;

        assert(fds || nfds == 0);

        if (nfds == 0)
                return 0;

        r = ppoll(fds, nfds, timeout == USEC_INFINITY ? NULL : timespec_store(&ts, timeout), NULL);
        if (r < 0)
                return -errno;
        if (r == 0)
                return 0;

        for (size_t i = 0, n = r; i < nfds && n > 0; i++) {
                if (fds[i].revents == 0)
                        continue;
                if (fds[i].revents & POLLNVAL)
                        return -EBADF;
                n--;
        }

        return r;
}

int fd_wait_for_event(int fd, int event, usec_t timeout) {
        struct pollfd pollfd = {
                .fd = fd,
                .events = event,
        };
        int r;

        r = ppoll_usec(&pollfd, 1, timeout);
        if (r <= 0)
                return r;

        return pollfd.revents;
}

static size_t nul_length(const uint8_t *p, size_t sz) {
        size_t n = 0;

        while (sz > 0) {
                if (*p != 0)
                        break;

                n++;
                p++;
                sz--;
        }

        return n;
}

ssize_t sparse_write(int fd, const void *p, size_t sz, size_t run_length) {
        const uint8_t *q, *w, *e;
        ssize_t l;

        q = w = p;
        e = q + sz;
        while (q < e) {
                size_t n;

                n = nul_length(q, e - q);

                /* If there are more than the specified run length of
                 * NUL bytes, or if this is the beginning or the end
                 * of the buffer, then seek instead of write */
                if ((n > run_length) ||
                    (n > 0 && q == p) ||
                    (n > 0 && q + n >= e)) {
                        if (q > w) {
                                l = write(fd, w, q - w);
                                if (l < 0)
                                        return -errno;
                                if (l != q -w)
                                        return -EIO;
                        }

                        if (lseek(fd, n, SEEK_CUR) == (off_t) -1)
                                return -errno;

                        q += n;
                        w = q;
                } else if (n > 0)
                        q += n;
                else
                        q++;
        }

        if (q > w) {
                l = write(fd, w, q - w);
                if (l < 0)
                        return -errno;
                if (l != q - w)
                        return -EIO;
        }

        return q - (const uint8_t*) p;
}

#if 0 /// UNNEEDED by elogind
char* set_iovec_string_field(struct iovec *iovec, size_t *n_iovec, const char *field, const char *value) {
        char *x;

        x = strjoin(field, value);
        if (x)
                iovec[(*n_iovec)++] = IOVEC_MAKE_STRING(x);
        return x;
}

char* set_iovec_string_field_free(struct iovec *iovec, size_t *n_iovec, const char *field, char *value) {
        char *x;

        x = set_iovec_string_field(iovec, n_iovec, field, value);
        free(value);
        return x;
}

struct iovec_wrapper *iovw_new(void) {
        return malloc0(sizeof(struct iovec_wrapper));
}

void iovw_free_contents(struct iovec_wrapper *iovw, bool free_vectors) {
        if (free_vectors)
                for (size_t i = 0; i < iovw->count; i++)
                        free(iovw->iovec[i].iov_base);

        iovw->iovec = mfree(iovw->iovec);
        iovw->count = 0;
        iovw->size_bytes = 0;
}

struct iovec_wrapper *iovw_free_free(struct iovec_wrapper *iovw) {
        iovw_free_contents(iovw, true);

        return mfree(iovw);
}

struct iovec_wrapper *iovw_free(struct iovec_wrapper *iovw) {
        iovw_free_contents(iovw, false);

        return mfree(iovw);
}

int iovw_put(struct iovec_wrapper *iovw, void *data, size_t len) {
        if (iovw->count >= IOV_MAX)
                return -E2BIG;

        if (!GREEDY_REALLOC(iovw->iovec, iovw->size_bytes, iovw->count + 1))
                return -ENOMEM;

        iovw->iovec[iovw->count++] = IOVEC_MAKE(data, len);
        return 0;
}

int iovw_put_string_field(struct iovec_wrapper *iovw, const char *field, const char *value) {
        _cleanup_free_ char *x = NULL;
        int r;

        x = strjoin(field, value);
        if (!x)
                return -ENOMEM;

        r = iovw_put(iovw, x, strlen(x));
        if (r >= 0)
                TAKE_PTR(x);

        return r;
}

int iovw_put_string_field_free(struct iovec_wrapper *iovw, const char *field, char *value) {
        _cleanup_free_ _unused_ char *free_ptr = value;

        return iovw_put_string_field(iovw, field, value);
}

void iovw_rebase(struct iovec_wrapper *iovw, char *old, char *new) {
        for (size_t i = 0; i < iovw->count; i++)
                iovw->iovec[i].iov_base = (char *)iovw->iovec[i].iov_base - old + new;
}

size_t iovw_size(struct iovec_wrapper *iovw) {
        size_t n = 0;

        for (size_t i = 0; i < iovw->count; i++)
                n += iovw->iovec[i].iov_len;

        return n;
}
#endif // 0
