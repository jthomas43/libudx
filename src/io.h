#ifndef UDX_IO_H
#define UDX_IO_H

#include "../include/udx.h"

int
udx__get_link_mtu (const struct sockaddr *s);

ssize_t
udx__sendmsg (udx_socket_t *handle, const uv_buf_t bufs[], unsigned int bufs_len, struct sockaddr *addr, int addr_len);

ssize_t
udx__recvmsg (udx_socket_t *handle, uv_buf_t *buf, struct sockaddr *addr, int addr_len);

int
udx__udp_set_rxq_ovfl (uv_os_sock_t fd);

int
udx__udp_set_dontfrag (uv_os_sock_t fd, bool is_ipv6);

#endif // UDX_IO_H
