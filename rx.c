#include <stdbool.h>
#include <stdint.h>

#include <microkit.h>
#include <sddf/network/queue.h>
#include <sddf/util/printf.h>
#include <sddf/util/util.h>


const microkit_channel FROM_SEND = 2;

uintptr_t tx_buffer_data;
uintptr_t tx_free;
uintptr_t tx_active;
uintptr_t uart_base;

typedef struct state {
  net_queue_handle_t tx_queue;
} state_t;

state_t state;

int receive(void) {

  // make sure there is something to receive
  if(net_queue_empty_active(&state.tx_queue)) {
    return -1;
  }

  // get free buffer
  net_buff_desc_t buffer;
  int err;
  err = net_dequeue_active(&state.tx_queue, &buffer);
  assert(!err);

  // read data from buffer
  uint64_t data = *(uint64_t *)(buffer.io_or_offset + tx_buffer_data);
  buffer.len = 0;
  
  sddf_dprintf("RX: Received %ld\r\n", data);

  // return buffer to free queue
  err = net_enqueue_free(&state.tx_queue, buffer);
  assert(!err);
  return 0;
}

void receive_all(void) {
    int num_buffs = net_queue_size(state.tx_queue.active);
    sddf_dprintf("RX: Receiving %d buffers\r\n", num_buffs);
    int err;
    do {
        err = receive();
    } while (!err);
    sddf_dprintf("RX: Waiting for more data\r\n");
}

void init(void) {
  sddf_dprintf("RX init\r\n");

  // Init queue's representation for this process
  net_queue_init(&state.tx_queue, (net_queue_t *)tx_free,
                 (net_queue_t *)tx_active, 512);
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case FROM_SEND:
        receive_all();
        break;
    default:
        sddf_dprintf("recv: received notification on unexpected channel: %u\r\n", ch);
        break;
    }
}