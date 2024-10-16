/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <lfqueue.h>

struct sc_queue_t {
    atomic_int head;
    atomic_int tail;
    sc_uint length;
    sc_uint data[];
};

sc_queue * allocate_queue(sc_uint num) {
    sc_queue * q = (sc_queue*)malloc(sizeof(sc_queue) + num*sizeof(sc_uint*));
    q->head = 0;
    q->tail = 0;
    q->length = num;
    for (int i = 0; i < num; i++) {
        q->data[i] = 0;
    }
    return q;
}

sc_bool enqueue(sc_queue *queue, sc_uint value) {
    int tail = atomic_fetch_add_explicit(&queue->tail, 1, memory_order_release);

    if ((tail + 1) % queue->length == queue->head) {
        // Queue is full
        return FALSE;
    }
    queue->data[tail % queue->length] = value;
    atomic_thread_fence(memory_order_release);
    return TRUE;
}

sc_uint dequeue(sc_queue *queue) {
    sc_uint head = atomic_fetch_add_explicit(&queue->head, 1, memory_order_release);
    sc_uint value = queue->data[head % queue->length];
    atomic_thread_fence(memory_order_release);
    return value;
}

sc_bool is_empty(sc_queue *queue) {
    return queue->head == queue->tail ? TRUE : FALSE;
}