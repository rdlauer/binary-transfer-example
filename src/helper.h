#ifndef HELPER_H
#define HELPER_H

#include <stddef.h>

void binary_send_to_notehub(void);
int binary_send_to_notecard(const void *data, size_t len);
int binary_setup(size_t chunk_size, size_t buf_size);

#endif // HELPER_H
