#include <stdbool.h>
#include <stdint.h>

#include <microkit.h>
#include <sddf/network/queue.h>
#include <sddf/util/printf.h>
#include <sddf/util/util.h>

const microkit_channel TO_RECV = 2;

uintptr_t tx_buffer_data;
uintptr_t tx_free;
uintptr_t tx_active;
uintptr_t uart_base;

typedef struct state {
  net_queue_handle_t tx_queue;
} state_t;

state_t state;

void send(uint64_t data) {

  // get a free buffer
  // NOTE: my current understanding is that one should NOT typically do
  // this, since the microkit does not expect init() and notify() to take
  // very long, and thus is not necessarily guaranteed to handle incoming
  // notifications well.
  net_buff_desc_t buffer;
  while (net_dequeue_free(&state.tx_queue, &buffer) == -1) {
    // nothing
  }

  // write data to buffer
  *(int *)(buffer.io_or_offset + tx_buffer_data) = data;
  buffer.len = sizeof(int);

  // enqueue buffer to active queue
  int err = net_enqueue_active(&state.tx_queue, buffer);
  assert(!err);
}

void transmit(void) {
  for (uint64_t i = 0;; i++) {
    sddf_dprintf("TX: Transmitting %ld\r\n", i);
    microkit_notify(TO_RECV);
    send(i);
  }
}

void init(void) {
  sddf_dprintf("TX init\r\n");

  // Init queue's representation for this process
  net_queue_init(&state.tx_queue, (net_queue_t *)tx_free,
                 (net_queue_t *)tx_active, 512);

  // Init buffers in shared memory
  net_buffers_init(&state.tx_queue, 0);

  transmit();
}

void notified(microkit_channel ch) {
  switch (ch) {
  default:
    sddf_dprintf("recv: received notification on unexpected channel: %u\r\n",
                 ch);
    break;
  }
}