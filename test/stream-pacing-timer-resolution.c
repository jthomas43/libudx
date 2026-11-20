#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../include/udx.h"

// a paced bulk write holds the raised timer resolution while it is pacing and
// releases it once everything is acked; the receiver never needs a hold

#define WRITE_SIZE (4 * 1024 * 1024)

uv_loop_t loop;
udx_t udx;

udx_socket_t asock;
udx_stream_t astream;

udx_socket_t bsock;
udx_stream_t bstream;

uv_check_t check;
uv_timer_t after_ack;

udx_stream_write_t *req;
char *data;

size_t read_bytes = 0;
bool ack_called = false;
uint32_t max_holds = 0;
int nclosed = 0;

void
on_check (uv_check_t *handle) {
  if (udx.timer_resolution_holds > max_holds) max_holds = udx.timer_resolution_holds;

  assert(!astream.timer_resolution_held);
  assert(udx.timer_resolution_holds <= 1);

  if (ack_called) {
    assert(udx.timer_resolution_holds == 0);
    assert(!bstream.timer_resolution_held);
  }
}

void
on_close (udx_stream_t *s, int status) {
  assert(status == 0);

  if (++nclosed == 2) {
    uv_close((uv_handle_t *) &check, NULL);
    uv_close((uv_handle_t *) &after_ack, NULL);
    udx_socket_close(&asock);
    udx_socket_close(&bsock);
  }
}

void
on_after_ack (uv_timer_t *timer) {
  assert(udx.timer_resolution_holds == 0);
  assert(!bstream.timer_resolution_held);

  udx_stream_destroy(&astream);
  udx_stream_destroy(&bstream);
}

void
on_ack (udx_stream_write_t *req, int status, int unordered) {
  assert(status == 0);

  ack_called = true;

  // keep both streams open briefly so the test specifically verifies release
  // after the final ack rather than release during stream close.
  uv_timer_start(&after_ack, on_after_ack, 50, 0);
}

void
on_read (udx_stream_t *handle, ssize_t read_len, const uv_buf_t *buf) {
  assert(read_len > 0);
  read_bytes += read_len;
}

int
main () {
  int e;

  req = malloc(udx_stream_write_sizeof(1));
  data = calloc(1, WRITE_SIZE);

  uv_loop_init(&loop);

  e = udx_init(&loop, &udx, NULL);
  assert(e == 0);

  e = udx_socket_init(&udx, &asock, NULL);
  assert(e == 0);

  e = udx_socket_init(&udx, &bsock, NULL);
  assert(e == 0);

  struct sockaddr_in baddr;
  uv_ip4_addr("127.0.0.1", 18132, &baddr);
  e = udx_socket_bind(&bsock, (struct sockaddr *) &baddr, 0);
  assert(e == 0);

  struct sockaddr_in aaddr;
  uv_ip4_addr("127.0.0.1", 18131, &aaddr);
  e = udx_socket_bind(&asock, (struct sockaddr *) &aaddr, 0);
  assert(e == 0);

  e = udx_stream_init(&udx, &astream, 1, on_close, NULL);
  assert(e == 0);

  e = udx_stream_init(&udx, &bstream, 2, on_close, NULL);
  assert(e == 0);

  e = udx_stream_connect(&astream, &asock, 2, (struct sockaddr *) &baddr);
  assert(e == 0);

  e = udx_stream_connect(&bstream, &bsock, 1, (struct sockaddr *) &aaddr);
  assert(e == 0);

  e = udx_stream_read_start(&astream, on_read);
  assert(e == 0);

  uv_timer_init(&loop, &after_ack);
  uv_check_init(&loop, &check);
  uv_check_start(&check, on_check);

  uv_buf_t buf = uv_buf_init(data, WRITE_SIZE);
  e = udx_stream_write(req, &bstream, &buf, 1, on_ack);
  assert(e == 0 && "not drained");

  e = uv_run(&loop, UV_RUN_DEFAULT);
  assert(e == 0);

  assert(ack_called);
  assert(read_bytes == WRITE_SIZE);
  assert(max_holds == 1 && "the sender never waited on its pacing deadline");
  assert(udx.timer_resolution_holds == 0);
  assert(!udx.timer_resolution_raised);

  free(req);
  free(data);

  return 0;
}
