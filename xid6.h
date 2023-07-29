#include <stdint.h>

struct binary_file {
    uint8_t *data;
    size_t size;
};

struct ost { // official soundtrack
    char        *title;
    uint8_t     disc;
    uint16_t    track;
};

struct xid6 {
    char        *song;
    char        *game;
    char        *artist;
    char        *dumper;
    char        *publisher;
    char        *comments;
    uint32_t    dumped_date;
    uint32_t    intro_length;
    uint32_t    loop_length;
    uint32_t    end_length;
    uint32_t    fade_length;
    uint32_t    mixing_level;
    uint16_t    copyright_year;
    uint8_t     muted_voices;
    uint8_t     number_of_times_to_loop;
    uint8_t     emulator;
    struct ost  ost;
};

int valid_spc( struct binary_file *file );
int valid_xid6( struct binary_file *file );
uint32_t parse_u32( const uint8_t* );
