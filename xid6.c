/* xid6info
 * Copyright (C) 2023 Anosh D. Ullenius
 *
 * GPL-3.0-only
 *
 * This file is part of xid6info
 *
 * xid6info is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 3.0.
 *
 * xid6info is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xid6.h"

#define HEADER_SIZE         27
#define SPC_HEADER          ("SNES-SPC700 Sound File Data")
#define XID6_HEADER_MAGIC   ("xid6")
#define XID6_HEADER_LENGTH  8 // magic + length
#define XID6_OFFSET         0x10200

#define MIN_STRING_LEN      4
#define MAX_STRING_LEN      256

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

void *allocate_copy (const uint8_t *src, const size_t len) {
    if (len < MIN_STRING_LEN || len > MAX_STRING_LEN) {
        fprintf(stderr, "Length must be between 4-256\n");
        exit(-1);
    }
    const int add_null = src[len - 1] != '\0';
    uint8_t *buf = malloc ( len + add_null );
    memcpy (buf, src, len );
    if (add_null) {
        buf[len] = '\0';
    }
    return buf;
}

// validates OST track (track no and optional ASCII char)
// returns valid OST track (2-parts)
// zeroes invalid part(s) (some SPCs are broken)
uint16_t cleanup_ost_track(const uint16_t track) {
    uint8_t no = track >> 8;
    if (no > 99) { // valid track numbers 0-99
        no = 0;
    }
    uint8_t ch = track & 0xFF;
    const int is_ascii = (ch > 32 && ch < 127);
    if ( !is_ascii ) {
        ch = 0;
    }
    return ( no << 8 ) | ch;
}

inline uint32_t parse_u32(const uint8_t *src) {
    return src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24;
}

void free_ifpresent( void *target ) {
    if ( target ) {
        free ( target );
    }
}

void printBits(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", ( (1 << i) & val) != 0 );
    }
}

void parse_xid6( struct binary_file *spc ) {
    struct xid6 tags = { .ost = { 0 } };
    const uint8_t *len = &spc->data[ XID6_OFFSET + 4 ];
    const uint32_t chunk_size = len[0] | len[1] << 8 | len[2] << 16 | len[3] << 24;

    size_t offset = XID6_OFFSET + XID6_HEADER_LENGTH;
    while ( offset < ( XID6_OFFSET + XID6_HEADER_LENGTH + chunk_size) ) {
        uint8_t id = spc->data[ offset++ ];
        uint8_t type = spc->data[ offset++ ];

        // if stored *after* the sub-chunk header, this contains the length
    	uint16_t val = spc->data[ offset++ ];
    	val |= spc->data[ offset++ ] << 8;

        switch (id) {
            case 0x1:
                tags.song = allocate_copy( &spc->data[ offset ], val );   break;
            case 0x2:
                tags.game = allocate_copy( &spc->data[ offset ], val );   break;
            case 0x3:
                tags.artist = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x4:
                tags.dumper = allocate_copy( &spc->data[ offset ], val ); break;
            case 0x5:
                tags.dumped_date = parse_u32( &spc->data[ offset ] );     break;
            case 0x6:
                tags.emulator = val & 0xFF;                               break;
            case 0x7:
                tags.comments = allocate_copy(&spc->data[ offset ], val); break;
            case 0x10:
                tags.ost.title = allocate_copy(&spc->data[offset ], val); break;
            case 0x11:
                tags.ost.disc = val;                                      break;
            case 0x12:
                tags.ost.track = cleanup_ost_track( val );                break;
            case 0x13:
                tags.publisher = allocate_copy( &spc->data[offset], val); break;
            case 0x14:
                tags.copyright_year = val;                                break;
            case 0x30:
                tags.intro_length = parse_u32( &spc->data[ offset ] );    break;
            case 0x31:
                tags.loop_length =  parse_u32( &spc->data[ offset ] );    break;
            case 0x32:
                tags.end_length = parse_u32( &spc->data[ offset ] );      break;
            case 0x33:
                tags.fade_length = parse_u32( &spc->data[ offset ] );     break;
            case 0x34:
                tags.muted_voices = val & 0xFF;                           break;
            case 0x35:
                tags.number_of_times_to_loop = val & 0xFF;                break;
            case 0x36:
                tags.mixing_level = parse_u32( &spc->data[ offset ] );    break;
            default:
                fprintf(stderr, "Unknown id: %#x\n", id);
                exit(-1);
        }
        if ( type ) { // value stored after sub-chunk header
            // 4-byte alignment of data
            uint8_t padding = (4 - (val % 4) ) % 4;
            offset += val + padding;
        }
    }

    if (tags.game)
        printf("Game name : %s\n",                  tags.game );
    if (tags.song)
        printf("Song name : %s\n",                  tags.song );
    if (tags.artist)
        printf("Artist : %s\n",                     tags.artist );
    if (tags.dumper)
        printf("Dumped by : %s\n",                  tags.dumper );
    if ( tags.dumped_date )
        printf("Date song was dumped : %d\n",       tags.dumped_date );
    if ( tags.emulator )
        printf("Emulator : %d\n",                   tags.emulator );
    if (tags.comments)
        printf("Comments : %s\n",                   tags.comments );
    if (tags.ost.title)
        printf("Official Soundtrack Title : %s\n",  tags.ost.title );
    if (tags.ost.disc)
        printf("OST disc : %d\n",                   tags.ost.disc );
    if (tags.ost.track) {
        const char ch = tags.ost.track & 0xFF;
        printf("OST track : %d", tags.ost.track >> 8 );
        if (ch)
            putchar(ch);
        printf("\n");
    }
    if (tags.publisher)
        printf("Publisher : %s\n",                  tags.publisher );
    if (tags.copyright_year)
        printf("Copyright year : %d\n",             tags.copyright_year );
    if (tags.intro_length)
        printf("Intro length : %#4x\n",             tags.intro_length );
    if (tags.loop_length)
        printf("Loop length : %d\n",                tags.loop_length );
    if (tags.end_length)
        printf("End length : %d\n",                 tags.end_length );
    if (tags.fade_length)
        printf("Fade length : %d\n",                tags.fade_length );
    if (tags.muted_voices) {
        printf("Muted voices : ");
        printBits(                                  tags.muted_voices );
        printf("\n");
    }
    if (tags.number_of_times_to_loop)
        printf("No. times to loop : %d\n", tags.number_of_times_to_loop );
    if (tags.mixing_level)
        printf("Mixing (preamp) level : %#04x\n",  tags.mixing_level );

    free_ifpresent( tags.publisher );
    free_ifpresent( tags.artist );
    free_ifpresent( tags.game );
    free_ifpresent( tags.song );
    free_ifpresent( tags.dumper );
    free_ifpresent( tags.comments );
    free_ifpresent( tags.ost.title );
}

struct binary_file *read_file(FILE *file) {
    fseek( file, 0, SEEK_SET );
    fseek( file, 0, SEEK_END );
    const size_t size = ftell( file );
    
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
        fprintf(stderr, "Usage: xid6info file.spc\n");
        return -1;
    }
    FILE *file= fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file\n");
        return -1;
    }
    struct binary_file *spc = read_file( file );
    if ( fclose( file ) != 0 ) {
        fprintf(stderr, "Error closing file\n");
        free ( spc->data );
        free ( spc );
        return -1;
    };
    if ( !valid_spc ( spc ) ) {
        fprintf(stderr, "Not a valid SPC file\n");
        free( spc->data );
        free( spc );
        return -1;
    }
    if (valid_xid6 ( spc ) ) {
        parse_xid6( spc );
    } else {
        fprintf(stderr, "Unable to read xid6 tags\n");
        free( spc->data );
        free( spc );
        return -1;
    }
    free( spc->data );
    free( spc );

    return 0;
}
