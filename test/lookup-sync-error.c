#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "../include/udx.h"

// a lookup that fails synchronously must not keep a ref on the udx handle

udx_t udx;
uv_loop_t loop;
udx_lookup_t req;

void
on_lookup (udx_lookup_t *req, int status, const struct sockaddr *addr, int addr_len) {
  assert(false && "callback must not run for a failed request");
}

int
main () {
  int e;

  uv_loop_init(&loop);
  udx_init(&loop, &udx, NULL);

  // no host and no service: uv_getaddrinfo fails before starting
  e = udx_lookup(&udx, &req, NULL, 0, on_lookup);
  assert(e < 0);
  assert(udx_is_idle(&udx));

  uv_run(&loop, UV_RUN_DEFAULT);

  return 0;
}
