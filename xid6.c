#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xid6.h"

#define HEADER_SIZE 27
#define SPC_HEADER          ("SNES-SPC700 Sound File Data")
#define XID6_HEADER_MAGIC   ("xid6")
#define XID6_HEADER_LENGTH  8 // magic + length
#define XID6_OFFSET         0x10200

struct xid6 {
    char        *song;
    char        *game;
    char        *artist;
    char        *dumper;
    uint32_t    dumped_date;
    uint8_t     emulator;
    char        *comments;
    char        *ost_title; // official soundtrack title
    uint8_t     ost_disc;
    uint16_t    ost_track;
    char        *publishers_name;
    uint16_t    copyright_year;
    uint32_t    intro_length;
    uint32_t    loop_length;
    uint32_t    end_length;
    uint32_t    fade_length;
    uint8_t     muted_voices;
    uint8_t     number_of_times_to_loop;
    uint32_t    mixing_level;
};

uint32_t parse_u32(const uint8_t*);

int valid_spc(struct binary_file *file) {
    if (file->size < HEADER_SIZE) {
        return 0;
    }
    return memcmp( file->data, SPC_HEADER, HEADER_SIZE ) == 0;
}

int valid_xid6( struct binary_file *file ) {
    if ( file->size < (XID6_OFFSET + XID6_HEADER_LENGTH) ) {
        return 0;
    }
    return memcmp( &file->data[ XID6_OFFSET ], XID6_HEADER_MAGIC, 4 ) == 0;
}

void *allocate_copy (const uint8_t *src, size_t len) {
    if (len < 4 || len > 256) {
        fprintf(stderr, "Length must be between 4-256\n");
        exit(-1);
    }
    uint8_t *buf = malloc ( len );
    memcpy (buf, src, len );
    return buf;
}

void set_intro_length( struct xid6 *tags, const uint8_t *src ) {
    tags->intro_length = parse_u32( src );
}

void set_fade_length( struct xid6 *tags, const uint8_t *src ) {
    tags->fade_length = parse_u32( src );
}

void set_dumped_date( struct xid6 *tags, const uint8_t *src ) {
    tags->dumped_date = parse_u32( src );
}

void set_loop_length( struct xid6 *tags, const uint8_t *src ) {
    tags->loop_length = parse_u32( src );
}

void set_end_length( struct xid6 *tags, const uint8_t *src ) {
    tags->end_length = parse_u32( src );
}

void set_mixing_level( struct xid6 *tags, const uint8_t *src ) {
    tags->mixing_level = parse_u32( src );
}

inline uint32_t parse_u32(const uint8_t *src) {
    return src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24;
}

void parse_xid6( struct binary_file *spc ) {
    struct xid6 tags = { 0 };
    uint8_t *len = &spc->data[ XID6_OFFSET + 4 ];
    uint32_t chunk_size = len[0] | len[1] << 8 | len[2] << 16 | len[3] << 24;

    printf("chunk size: %d\n", chunk_size );

    size_t offset = XID6_OFFSET + XID6_HEADER_LENGTH;
    while ( offset < ( XID6_OFFSET + XID6_HEADER_LENGTH + chunk_size) ) {
        uint8_t id = spc->data[ offset++ ];
        uint8_t type = spc->data[ offset++ ];
        printf("id: %#x\n", id);
        //printf("type: %d\n", type);
        printf("stored in header: %d\n", type == 0);

    	uint16_t val = spc->data[ offset++];
    	val |= spc->data[offset++] << 8;           

        switch (id) {
            case 0x1:
                tags.song = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x2:
                tags.game = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x3:
                tags.artist = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x4:
                tags.dumper = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x5:
                set_dumped_date( &tags, &spc->data[ offset ] ); break;
            case 0x6:
                tags.emulator = val & 0xFF; break;
            case 0x7:
                tags.comments = allocate_copy( &spc->data[ offset ], val );
                break;
            case 0x10:
                tags.ost_title = allocate_copy( &spc->data[ offset ], val );
                break;
            case 0x11:
                tags.ost_disc = val; break;
            case 0x12:
                tags.ost_track = val; break;
            case 0x13:
                tags.publishers_name = allocate_copy( &spc->data[ offset ], val ); 
                break;
            case 0x14:
                tags.copyright_year = val; break;
            case 0x30:
                set_intro_length( &tags, &spc->data[ offset ] ); break;
            case 0x31:
                set_loop_length( &tags, &spc->data[ offset ] ); break;
            case 0x32:
                set_end_length( &tags, &spc->data[ offset ] ); break;
            case 0x33:
                set_fade_length( &tags, &spc->data[ offset ] ); break;
            case 0x34:
                tags.muted_voices = val & 0xFF; break;
            case 0x35:
                tags.number_of_times_to_loop = val & 0xFF; break;
            case 0x36:
                set_mixing_level( &tags, &spc->data[ offset ] ); break;
            default:
                fprintf(stderr, "Unknown id: %#x\n", id);
                exit(-1);
        }

        if ( type == 0 ) {
            printf("val stored in sub-chunk header: %d\n", val);
        } else {
            printf("increase offset by: %d\n", val); // TODO use hard-coded ?
            // 4-byte alignment of data
            uint8_t padding = (4 - (val % 4) ) % 4; // sub-chunk header is 4 bytes
            if (padding) {
                printf("extra padding: %d bytes\n", padding);
            }
            offset += val + padding;
        }
    }

    printf("Game name: %s\n", tags.game ? tags.game : "" );
    printf("Song name: %s\n", tags.song ? tags.song : "" );
    printf("Artist: %s\n", tags.artist ? tags.artist : "" );
    printf("Dumped by: %s\n", tags.dumper ? tags.dumper : "" );
    if ( tags.dumped_date ) {
        printf("Date song was dumped: %d\n", tags.dumped_date );
    }
    if ( tags.emulator ) {
        printf("Emulator: %d\n", tags.emulator );
    }
    printf("Copyright year: %d\n", tags.copyright_year );
    printf("Comments: %s\n", tags.comments ? tags.comments : "" );
    if (tags.ost_title) {
            printf("Official Soundtrack Title: %s\n", tags.ost_title );
    }
    if (tags.ost_disc) {
        printf("OST disc: %d\n", tags.ost_disc );
    }
    printf("Publishers name: %s\n", tags.publishers_name );
    printf("Intro length: %#x\n", tags.intro_length );
    printf("Fade length: %d\n", tags.fade_length );
    printf("Number of times to loop: %d\n", tags.number_of_times_to_loop );
    if (tags.ost_track) {
        printf("OST track: %d %c\n", tags.ost_track >> 8, tags.ost_track & 0xFF );
    }

    if ( tags.publishers_name ) {
        free( tags.publishers_name );
    }
    if ( tags.artist ) {
        free( tags.artist);
    }
    if ( tags.game ) {
        free( tags.game );
    }
    if ( tags.song ) {
        free( tags.song );
    }
    if ( tags.dumper ) {
        free( tags.dumper );
    }
    if ( tags.comments ) {
        free ( tags.comments );
    }
    if ( tags.ost_title ) {
        free( tags.ost_title );
    }
}

struct binary_file *read_file(FILE *file) {
    fseek( file, 0, SEEK_SET );
    fseek( file, 0, SEEK_END );
    size_t size = ftell( file );
    
    struct binary_file *bin = malloc( sizeof(struct binary_file) );
    bin->size = size;
    bin->data = malloc( size );
    fseek( file, 0, SEEK_SET );
    if ( fread( bin->data, size, 1, file ) != 1) {
        fprintf(stderr, "fread failed\n");
        exit(-1);
    }
    return bin;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: xid6 file.spc\n");
        return -1;
    }
    FILE *file= fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file\n");
        return -1;
    }
    struct binary_file *spc = read_file( file );
    printf("file size: %ld\n", spc->size );
    printf("valid SPC header: %d\n", valid_spc( spc ) );
    printf("valid xid6 header: %d\n", valid_xid6( spc ) );

    if (valid_xid6 ( spc ) ) {
        parse_xid6( spc );
    }

    free( spc->data );
    free( spc );

    return 0;
}
