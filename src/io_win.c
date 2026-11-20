#include <assert.h>
#include <uv.h>

#include <timeapi.h>

#include "internal.h"
#include "io.h"

int
udx__get_link_mtu (const struct sockaddr *addr) {
  assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

  int s = socket(addr->sa_family, SOCK_DGRAM, 0);
  if (s == -1) {
    return -1;
  }

  int rc = connect(s, addr, addr->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  if (rc == -1) {
    return -1;
  }

  int mtu;
  int mtu_opt_size = sizeof mtu;

  if (addr->sa_family == AF_INET) {
    rc = getsockopt(s, IPPROTO_IP, IP_MTU, (char *) &mtu, &mtu_opt_size);
  } else {
    rc = getsockopt(s, IPPROTO_IPV6, IPV6_MTU, (char *) &mtu, &mtu_opt_size);
  }
  if (rc == -1) {
    closesocket(s);
    return -1;
  }

  closesocket(s);
  return mtu;
}

int
udx__get_socket_ttl (udx_socket_t *socket) {
  uv_os_fd_t fd;
  uv_fileno((uv_handle_t *) &socket->uv_udp, &fd);

  int ttl;
  socklen_t ttl_opt_size = sizeof ttl;
  int rc;
  if (socket->family == 4) {
    rc = getsockopt((SOCKET) fd, IPPROTO_IP, IP_TTL, (char *) &ttl, &ttl_opt_size);
  } else {
    rc = getsockopt((SOCKET) fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (char *) &ttl, &ttl_opt_size);
  }

  if (rc == -1) {
    return -1;
  }

  return ttl;
}

int
udx__udp_set_dontfrag (uv_os_sock_t fd, bool is_ipv6) {
  int rc;
  int val = IP_PMTUDISC_PROBE;
  if (is_ipv6) {
    rc = setsockopt(fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, (const char *) &val, sizeof(val));
  } else {
    rc = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, (const char *) &val, sizeof(val));
  }

  return rc;
}

// timeBeginPeriod(1) may not be honored, e.g. for minimized windowed
// processes on windows 11, leaving timers limited by the default timer
// resolution (~15.6 ms). the check in advance_next_send_ts is strict, so the
// credit can reach one packet interval beyond this.
const uint64_t udx__pacing_max_credit_ms = 16;

bool
udx__timer_resolution_begin (void) {
  // the pacer relies on short timers. ask for 1 ms timer resolution while
  // a stream is pacing.
  return timeBeginPeriod(1) == TIMERR_NOERROR;
}

void
udx__timer_resolution_end (void) {
  timeEndPeriod(1);
}
