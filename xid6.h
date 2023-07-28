#include <stdint.h>

struct binary_file {
    uint8_t *data;
    size_t size;
};

int valid_spc( struct binary_file *file );
int valid_xid6( struct binary_file *file );
