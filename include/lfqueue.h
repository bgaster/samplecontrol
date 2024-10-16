/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */
#ifndef QUEUE_HEADER_H
#define QUEUE_HEADER_H

#include <util.h>

struct sc_queue_t;
typedef struct sc_queue_t sc_queue;

sc_queue * allocate_queue(sc_uint num);
sc_bool enqueue(sc_queue *queue, sc_uint value);
sc_uint dequeue(sc_queue *queue);
sc_bool is_empty(sc_queue *queue);

#endif //QUEUE_HEADER_H