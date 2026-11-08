#include <stdio.h>                  // printf, fprintf, fopen, fread, fwrite, fseek, snprintf, perror
#include <string.h>                 // strrchr, strcmp, strlen
#include "decode.h"                 // DecodeInfo struct + decoding function declarations
#include "types.h"                  // Status (e_success / e_failure)
#include "common.h"                 // MAGIC_STRING definition

#define BMP_HEADER_SIZE 54          // BMP header is 54 bytes; no hidden data is stored in it

/* ---------- read_and_validate_decode_args ---------- */
/* argv: ./a.out -d <stego.bmp> [output_base_name] */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)   // Validates command-line args
{
    char *ext = strrchr(argv[2], '.');                  // Find last '.' in stego image name
    if (ext == NULL || strcmp(ext, ".bmp") != 0)        // No extension or not ".bmp"
    {
        printf("ERROR: Stego image must have .bmp extension\n");   // Error message
        return e_failure;                               // Reject
    }
    decInfo->stego_image_fname = argv[2];               // Store stego image name

    if (argv[3] != NULL)                                // User gave an output base name
    {
        if (strlen(argv[3]) > 100)                      // Guard against names too long for the buffer
        {
            printf("ERROR: Output name is too long\n"); // Error message
            return e_failure;                           // Reject
        }
        decInfo->output_base_name = argv[3];            // Store the base name (extension added later)
    }
    else                                                // No output name given
    {
        decInfo->output_base_name = "decoded";          // Default base name -> decoded.txt etc.
    }

    return e_success;                                   // Arguments are valid
}

/* ---------- open_decode_files ---------- */
Status open_decode_files(DecodeInfo *decInfo)           // Opens the stego image
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");   // Open in read-binary mode
    if (decInfo->fptr_stego_image == NULL)              // Open failed
    {
        perror("fopen");                                // Print system error reason
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);   // Print which file
        return e_failure;                               // Stop
    }
    return e_success;                                   // File opened
}

/* ---------- skip_bmp_header ---------- */
Status skip_bmp_header(FILE *fptr_stego_image)          // Moves file pointer past the header
{
    if (fseek(fptr_stego_image, BMP_HEADER_SIZE, SEEK_SET) != 0)   // Jump to byte 54 from the start
        return e_failure;                               // Seek failed
    return e_success;                                   // Now at the first pixel byte
}

/* ---------- decode_byte_from_lsb ---------- */
/* Rebuild one byte from the LSBs of 8 image bytes (MSB first) */
char decode_byte_from_lsb(const char *image_buffer)     // Reverse of encode_byte_to_lsb
{
    unsigned char ch = 0;                               // Will hold the rebuilt byte
    for (int i = 0; i < 8; i++)                         // For each of the 8 image bytes
        ch = (ch << 1) | (image_buffer[i] & 1);         // Shift left, then add this byte's LSB
    return (char)ch;                                    // Return the rebuilt character
}

/* ---------- decode_size_from_lsb ---------- */
/* Rebuild a 32-bit int from the LSBs of 32 image bytes (MSB first) */
int decode_size_from_lsb(const char *image_buffer)      // Reverse of encode_size_to_lsb
{
    unsigned int size = 0;                              // Will hold the rebuilt number
    for (int i = 0; i < 32; i++)                        // For each of the 32 image bytes
        size = (size << 1) | (image_buffer[i] & 1);     // Shift left, then add this byte's LSB
    return (int)size;                                   // Return the rebuilt number
}

/* ---------- decode_magic_string ---------- */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)   // Checks the image really has hidden data
{
    char buffer[8];                                     // 8 image bytes = 1 character

    for (size_t i = 0; i < strlen(magic_string); i++)   // For every character of the magic string
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)   // Read 8 bytes from image
            return e_failure;                           // Read failed

        if (decode_byte_from_lsb(buffer) != magic_string[i])       // Decoded char must equal expected char
            return e_failure;                           // Mismatch: no hidden data / wrong image
    }
    return e_success;                                   // Magic string matched
}

/* ---------- decode_secret_file_extn_size ---------- */
Status decode_secret_file_extn_size(DecodeInfo *decInfo)   // Reads length of the extension
{
    char buffer[32];                                    // 32 image bytes = one int

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)     // Read 32 bytes
        return e_failure;                               // Read failed

    decInfo->size_extn = decode_size_from_lsb(buffer);  // Rebuild the number and store it

    if (decInfo->size_extn < 1 || decInfo->size_extn >= (long)sizeof(decInfo->extn_secret_file))   // Sanity check: must fit array
        return e_failure;                               // Invalid size (protects against buffer overflow)

    return e_success;                                   // Extension size is valid
}

/* ---------- decode_secret_file_extn ---------- */
Status decode_secret_file_extn(DecodeInfo *decInfo)     // Reads extension text and opens output file
{
    char buffer[8];                                     // 8 image bytes = 1 character

    for (long i = 0; i < decInfo->size_extn; i++)       // For each extension character
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)   // Read 8 image bytes
            return e_failure;                           // Read failed

        decInfo->extn_secret_file[i] = decode_byte_from_lsb(buffer);   // Rebuild and store the character
    }
    decInfo->extn_secret_file[decInfo->size_extn] = '\0';   // Terminate the string (e.g. ".txt")

    snprintf(decInfo->output_fname, sizeof(decInfo->output_fname), "%s%s",
             decInfo->output_base_name, decInfo->extn_secret_file);    // Build name: base + extension (decoded.txt)

    decInfo->fptr_output = fopen(decInfo->output_fname, "wb");         // Create output file, write-binary
    if (decInfo->fptr_output == NULL)                   // Open failed
    {
        perror("fopen");                                // Print system error reason
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->output_fname);   // Print which file
        return e_failure;                               // Stop
    }
    return e_success;                                   // Extension decoded, output file ready
}

/* ---------- decode_secret_file_size ---------- */
Status decode_secret_file_size(DecodeInfo *decInfo)     // Reads size of the hidden file
{
    char buffer[32];                                    // 32 image bytes = one int

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)     // Read 32 bytes
        return e_failure;                               // Read failed

    decInfo->size_secret_file = decode_size_from_lsb(buffer);      // Rebuild the number and store it

    if (decInfo->size_secret_file < 0)                  // A negative size is impossible
        return e_failure;                               // Corrupt data

    return e_success;                                   // Size is valid
}

/* ---------- decode_secret_file_data ---------- */
Status decode_secret_file_data(DecodeInfo *decInfo)     // Extracts the hidden file's contents
{
    char buffer[8];                                     // 8 image bytes = 1 secret byte

    for (long i = 0; i < decInfo->size_secret_file; i++)   // Repeat for every secret byte
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)   // Read 8 image bytes
            return e_failure;                           // Read failed

        char ch = decode_byte_from_lsb(buffer);         // Rebuild the secret byte

        if (fwrite(&ch, 1, 1, decInfo->fptr_output) != 1)          // Write it to the output file
            return e_failure;                           // Write failed
    }
    return e_success;                                   // All data extracted
}

/* ---------- do_decoding ---------- */
Status do_decoding(DecodeInfo *decInfo)                 // Controls the whole decoding process
{
    if (open_decode_files(decInfo) == e_failure)        // Step 1: open stego image
    {
        printf("ERROR: Opening files failed\n");
        return e_failure;
    }
    printf("Files opened successfully\n");

    if (skip_bmp_header(decInfo->fptr_stego_image) == e_failure)   // Step 2: skip 54-byte header
    {
        printf("ERROR: Skipping BMP header failed\n");
        return e_failure;
    }

    if (decode_magic_string(MAGIC_STRING, decInfo) == e_failure)   // Step 3: verify magic string
    {
        printf("ERROR: Magic string not found - image has no hidden data\n");
        return e_failure;
    }
    printf("Magic string matched\n");

    if (decode_secret_file_extn_size(decInfo) == e_failure)        // Step 4: read extension length
    {
        printf("ERROR: Decoding extension size failed\n");
        return e_failure;
    }

    if (decode_secret_file_extn(decInfo) == e_failure)             // Step 5: read extension, open output file
    {
        printf("ERROR: Decoding extension failed\n");
        return e_failure;
    }

    if (decode_secret_file_size(decInfo) == e_failure)             // Step 6: read secret file size
    {
        printf("ERROR: Decoding secret file size failed\n");
        return e_failure;
    }

    if (decode_secret_file_data(decInfo) == e_failure)             // Step 7: extract secret data
    {
        printf("ERROR: Decoding secret data failed\n");
        return e_failure;
    }

    printf("Secret data written to %s\n", decInfo->output_fname);  // Tell the user the output file name

    fclose(decInfo->fptr_stego_image);                  // Close stego image
    fclose(decInfo->fptr_output);                       // Close output file

    return e_success;                                   // Decoding finished
}