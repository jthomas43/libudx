#include <string.h>
#include <uv.h>
#include <sys/socket.h>

#include "io.h"

#define UDX_RECVMMSG_BATCH_SIZE 20

ssize_t
udx__sendmsg (udx_socket_t *handle, const uv_buf_t bufs[], unsigned int bufs_len, struct sockaddr *addr, int addr_len) {
  ssize_t size;
  struct msghdr h;

  memset(&h, 0, sizeof(h));

  h.msg_name = addr;
  h.msg_namelen = addr_len;

  h.msg_iov = (struct iovec *) bufs;
  h.msg_iovlen = bufs_len;

  do {
    size = sendmsg(handle->io_poll.io_watcher.fd, &h, 0);
  } while (size == -1 && errno == EINTR);

  return size == -1 ? uv_translate_sys_error(errno) : size;
}

ssize_t
udx__recvmsg (udx_socket_t *handle, uv_buf_t *buf, struct sockaddr *addr, int addr_len) {
  ssize_t size;
  struct msghdr h;

  memset(&h, 0, sizeof(h));

  h.msg_name = addr;
  h.msg_namelen = addr_len;

  h.msg_iov = (struct iovec *) buf;
  h.msg_iovlen = 1;

  do {
    size = recvmsg(handle->io_poll.io_watcher.fd, &h, 0);
  } while (size == -1 && errno == EINTR);

  return size == -1 ? uv_translate_sys_error(errno) : size;
}

int
udx__recvmmsg( udx_socket_t *handle, uv_buf_t *bufs, struct sockaddr *addr, int addrlen, unsigned int *size, int nbufs)
{
  int sent = 0;
  struct mmsghdr h[UDX_RECVMMSG_BATCH_SIZE];

  int npkts = nbufs < UDX_RECVMMSG_BATCH_SIZE ? nbufs : UDX_RECVMMSG_BATCH_SIZE;

  while (nbufs > 0) {
    memset(h, 0, sizeof(struct mmsghdr) * npkts);

    for (int i = 0; i < npkts; i++) {
      h[i].msg_hdr.msg_name = addr + i;
      h[i].msg_hdr.msg_namelen = addrlen;

      h[i].msg_hdr.msg_iov = (struct iovec *) bufs + i;;
      h[i].msg_hdr.msg_iovlen = 1;
    }
    int rc;

    do {
       rc = recvmmsg(handle->io_poll.io_watcher.fd, h, npkts, 0, NULL);
    } while (rc == -1 && errno == EINTR);

    // debug_printf("recvmmsg rc=%d\n", rc);

    if (rc > 0) {
      for (int i = 0; i < rc; i++) {
        // debug_printf("msg_len=%d\n", h[i].msg_len);
        size[sent+i] = h[i].msg_len;
      }
      nbufs -= rc;
      sent += rc;
    }

    if (rc < 0 || rc < nbufs) {
      return sent > 0 ? sent : uv_translate_sys_error(errno);
    }
  }

  return sent;
}

#undef UDX_RECVMMSG_BATCH_SIZE

