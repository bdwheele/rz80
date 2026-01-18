#include "rz80.h"
/* Circular buffer stuff */

struct cbuf {
    int size;
    void *buffer;
    int write;
    int read;
};

struct cbuf *cbuf_new(int size, int esize) {
    struct cbuf *cbuf = calloc(1, sizeof(struct cbuf));
    cbuf->buffer = calloc(size, esize);
    cbuf->size = size;
}

int cbuf_full(struct cbuf *cbuf) {
    return (cbuf->write % cbuf->size) + 1 == cbuf->read;
}

int cbuf_free(struct cbuf *cbuf) {
    return cbuf->write - cbuf->read;
}

int cbuf_put(struct cbuf *cbuf) {
    
}
