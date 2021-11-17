#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../src/ucp.h"
#include "../src/fifo.h"
#include "../src/cirbuf.h"

static ucp_t client;
static ucp_stream_t client_sock;
static ucp_send_t sreq;
static uint32_t sbuf;

static size_t sent = 0;
static int rt = 100;
static size_t send_buf_len = 100;
static char *send_buf;

static void
on_message (ucp_t *self, char *buf, ssize_t nread, const struct sockaddr_in *from) {
  if (nread < 4) return;

  ucp_set_callback(&client, UCP_ON_MESSAGE, NULL);

  uint32_t id = *((uint32_t *) buf);
  printf("remote socket id: %u\n", id);

  send_buf = (char *) calloc(send_buf_len, 1);

  ucp_stream_connect(&client_sock, id, (const struct sockaddr *) from);

  for (int i = 0; i < 10; i++) {
    ucp_write_t *req = (ucp_write_t *) malloc(sizeof(ucp_write_t));
    sent += send_buf_len;
    ucp_stream_write(&client_sock, req, send_buf, send_buf_len);
  }
}

static void
on_write (ucp_stream_t *stream, ucp_write_t *req, int status) {
  // printf("on write\n");

  if (--rt > 0) {
    sent += send_buf_len;
    ucp_stream_write(stream, req, send_buf, send_buf_len);
  }

  printf("total sent=%zu\n", sent);
}

int
main () {
  srand(time(0));

  uv_loop_t* loop = malloc(sizeof(uv_loop_t));
  uv_loop_init(loop);

  struct sockaddr_in addr;

  ucp_init(&client, loop);

  uv_ip4_addr("0.0.0.0", 10102, &addr);
  ucp_bind(&client, (const struct sockaddr *) &addr);

  ucp_set_callback(&client, UCP_ON_MESSAGE, on_message);

  ucp_stream_init(&client, &client_sock);

  printf("local socket id: %u\n", client_sock.local_id);

  sbuf = client_sock.local_id;

  uv_ip4_addr("127.0.0.1", 10101, &addr);
  ucp_send(&client, &sreq, (char *) &sbuf, 4, (const struct sockaddr *) &addr);

  ucp_stream_set_callback(&client_sock, UCP_ON_WRITE, on_write);

  // printf("server stream id is: %u\n", server_sock.local_id);

  // ucp_stream_connect(&server_sock, client_sock.local_id, (const struct sockaddr *) &addr);

  // ucp_stream_set_callback(&server_sock, UCP_ON_READ, on_read);

  // for (int i = 0; i < 1000; i++) {
  //   ucp_write_t *req = (ucp_write_t *) malloc(sizeof(ucp_write_t));
  //   sent += buf_len;
  //   ucp_stream_write(&client_sock, req, buf, buf_len);
  // }

  // printf("running...\n");

  uv_run(loop, UV_RUN_DEFAULT);
  free(loop);

  return 0;
}
