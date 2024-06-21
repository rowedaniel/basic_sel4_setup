#include <stdbool.h>
#include <stdint.h>

#include <microkit.h>
#include <sddf/network/queue.h>
#include <sddf/util/printf.h>
#include <sddf/util/util.h>

const microkit_channel DRIVER_CH = 0;

uintptr_t rx_free;
uintptr_t rx_active;
uintptr_t rx_buffer_data_vaddr;
uintptr_t rx_buffer_data_paddr;

uintptr_t uart_base;

typedef struct state {
  net_queue_handle_t rx_queue;
} state_t;

state_t state;

void dump_packet(void *packet, unsigned int packet_len) {
  unsigned char *p = (unsigned char *)packet;
  for (unsigned int i = 0; i < packet_len; i++) {

    if (i % 16 == 0) {
      sddf_dprintf("%d\t", i);
    } else if (i % 4 == 0) {
      sddf_dprintf(" ");
    }

    sddf_dprintf("%02x ", p[i]);
    if (i % 16 == 15) {
      sddf_dprintf("\r\n");
    }
  }
  sddf_dprintf("\r\n");
}

int receive(void) {

  // make sure there is something to receive
  if (net_queue_empty_active(&state.rx_queue)) {
    return -1;
  }

  // get free buffer
  net_buff_desc_t buffer;
  int err;
  err = net_dequeue_active(&state.rx_queue, &buffer);
  assert(!err);

  // invalidate cache (ask Alain about this--I'm not sure I totally understand)
  cache_clean_and_invalidate(
      rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset,
      rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset +
          ROUND_UP(buffer.len, 1 << CONFIG_L1_CACHE_LINE_SIZE_BITS));

  // read data from buffer
  dump_packet(rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset,
              buffer.len);
  buffer.len = 0;

  // return buffer to free queue
  err = net_enqueue_free(&state.rx_queue, buffer);
  assert(!err);
  return 0;
}

void receive_all(void) {
  int num_buffs = net_queue_size(state.rx_queue.active);
  sddf_dprintf("RX: Receiving %d buffers\r\n", num_buffs);
  int err;
  do {
    err = receive();
  } while (!err);

  // ready for more notifications
  net_request_signal_active(&state.rx_queue);

  sddf_dprintf("RX: Waiting for more data\r\n");
}

void init(void) {
  sddf_dprintf("RX init\r\n");

  // Init queue's representation for this process
  net_queue_init(&state.rx_queue, (net_queue_t *)rx_free,
                 (net_queue_t *)rx_active, 512);
  net_buffers_init(&state.rx_queue, rx_buffer_data_paddr);
  microkit_notify(DRIVER_CH);
}

void notified(microkit_channel ch) {
  switch (ch) {
  case DRIVER_CH:
    receive_all();
    break;
  default:
    sddf_dprintf("recv: received notification on unexpected channel: %u\r\n",
                 ch);
    break;
  }
}