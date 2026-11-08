
#ifndef DECODE_H
#define DECODE_H
#include <stdio.h>
 
#include "types.h" // Contains user defined types
 
/*
 * Structure to store information required for
 * decoding the secret file from a stego image
 */
typedef struct _DecodeInfo
{
    /* Stego Image info */
    char *stego_image_fname;
    FILE *fptr_stego_image;
 
    /* Output (decoded secret) file info */
    char *output_base_name;        /* user supplied name without extension, or NULL */
    char output_fname[110];        /* final name = base name + decoded extension   */
    FILE *fptr_output;
 
    char extn_secret_file[5];      /* e.g. ".txt" */
    long size_extn;
    long size_secret_file;
} DecodeInfo;
 
/* Read and validate decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
 
/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);
 
/* Open the stego image */
Status open_decode_files(DecodeInfo *decInfo);
 
/* Skip the 54 byte BMP header */
Status skip_bmp_header(FILE *fptr_stego_image);
 
/* Decode and verify magic string */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);
 
/* Decode extension size, extension, secret file size, secret data */
Status decode_secret_file_extn_size(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);
 
/* Rebuild a byte / an int from LSBs of 8 / 32 image bytes (MSB first) */
char decode_byte_from_lsb(const char *image_buffer);
int  decode_size_from_lsb(const char *image_buffer);
 
#endif
 
