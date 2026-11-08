#include <stdio.h>                  // printf, fprintf, fopen, fread, fwrite, fseek, perror
#include <string.h>                 // strrchr, strcmp, strlen, strcpy
#include <stdlib.h>                 // general utilities
#include "encode.h"                 // EncodeInfo struct + function declarations
#include "types.h"                  // uint, Status (e_success / e_failure)
#include "common.h"                 // MAGIC_STRING definition

/* ---------- get_image_size_for_bmp ---------- */
uint get_image_size_for_bmp(FILE *fptr_image)   // Returns how many bytes the image can offer for hiding data
{
    uint width, height;                         // Variables to hold BMP width and height
    fseek(fptr_image, 18, SEEK_SET);            // BMP stores width at byte offset 18

    fread(&width, sizeof(int), 1, fptr_image);  // Read 4 bytes = width
    printf("width = %u\n", width);              // Show width

    fread(&height, sizeof(int), 1, fptr_image); // Read next 4 bytes = height
    printf("height = %u\n", height);            // Show height

    return width * height * 3;                  // 3 bytes per pixel (RGB) = total usable bytes
}

/* ---------- get_file_size ---------- */
uint get_file_size(FILE *fptr)                  // Returns size of a file in bytes
{
    fseek(fptr, 0, SEEK_END);                   // Move pointer to end of file
    uint size = ftell(fptr);                    // Current position = file size
    rewind(fptr);                               // Move pointer back to start

    return size;                                // Return the size
}

/* ---------- read_and_validate_encode_args ---------- */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)  // Validates command-line args
{
    char *ext = strrchr(argv[2], '.');          // Find last '.' in source image name

    if (ext == NULL || strcmp(ext, ".bmp") != 0)            // No extension or not ".bmp"
    {
        printf("ERROR: Source image must have .bmp extension\n");   // Error message
        return e_failure;                       // Reject
    }

    encInfo->src_image_fname = argv[2];         // Store source image name

    ext = strrchr(argv[3], '.');                // Find last '.' in secret file name

    if (ext == NULL || ext[1] == '\0')          // No extension, or nothing after the dot
    {
        printf("ERROR: Secret file must have an extension\n");      // Error message
        return e_failure;                       // Reject
    }

    if (strlen(ext) >= sizeof(encInfo->extn_secret_file))           // Extension too long for the array
    {
        printf("ERROR: Secret file extension is too long\n");       // Error message
        return e_failure;                       // Reject
    }

    encInfo->secret_fname = argv[3];            // Store secret file name
    strcpy(encInfo->extn_secret_file, ext);     // Store extension (e.g. ".txt")

    if (argv[4] != NULL)                        // User gave an output name
    {
        ext = strrchr(argv[4], '.');            // Find extension of output name

        if (ext == NULL || strcmp(ext, ".bmp") != 0)                // Must be .bmp
        {
            printf("ERROR: Output image must have .bmp extension\n");   // Error message
            return e_failure;                   // Reject
        }

        encInfo->stego_image_fname = argv[4];   // Store output name
    }
    else                                        // No output name given
    {
        encInfo->stego_image_fname = "stego.bmp";   // Use default name
    }

    return e_success;                           // All arguments are valid
}

/* ---------- open_files ---------- */
Status open_files(EncodeInfo *encInfo)          // Opens all three files
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");    // Open source image, read binary
    if (encInfo->fptr_src_image == NULL)        // Open failed
    {
        perror("fopen");                        // Print system error reason
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);  // Print which file
        return e_failure;                       // Stop
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");          // Open secret file, read binary
    if (encInfo->fptr_secret == NULL)           // Open failed
    {
        perror("fopen");                        // Print system error reason
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);     // Print which file
        return e_failure;                       // Stop
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb"); // Open output image, write binary
    if (encInfo->fptr_stego_image == NULL)      // Open failed
    {
        perror("fopen");                        // Print system error reason
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname); // Print which file
        return e_failure;                       // Stop
    }

    return e_success;                           // All files opened
}

/* ---------- check_capacity ---------- */
Status check_capacity(EncodeInfo *encInfo)      // Checks if the image can hold all the data
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);  // Bytes available in image
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);            // Secret file size in bytes

    uint required_size = 16 +                                   // Magic string: 2 chars x 8 bits
        32 +                                                    // Extension size: 32 bits
        (strlen(encInfo->extn_secret_file) * 8) +               // Extension characters x 8 bits
        32 +                                                    // Secret file size: 32 bits
        (encInfo->size_secret_file * 8);                        // Secret data x 8 bits

    if (encInfo->image_capacity >= required_size)               // Enough room (1 image byte hides 1 bit)
    {
        return e_success;                       // OK
    }

    printf("ERROR: Image does not have enough capacity\n");     // Not enough room
    return e_failure;                           // Fail
}

/* ---------- copy_bmp_header ---------- */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)    // Copies the 54-byte BMP header
{
    rewind(fptr_src_image);                     // Go to start of source image

    char buffer[54];                            // Buffer for the header

    if (fread(buffer, sizeof(char), 54, fptr_src_image) != 54)         // Read 54 bytes
    {
        return e_failure;                       // Read failed
    }

    if (fwrite(buffer, sizeof(char), 54, fptr_dest_image) != 54)       // Write 54 bytes to output
    {
        return e_failure;                       // Write failed
    }

    return e_success;                           // Header copied unchanged
}

/* ---------- encode_magic_string ---------- */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)   // Hides the magic string
{
    char buffer[8];                             // 8 image bytes = 1 character

    for (size_t i = 0; i < strlen(magic_string); i++)           // For every character of magic string
    {
        if (fread(buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)   // Read 8 bytes from image
        {
            return e_failure;                   // Read failed
        }

        encode_byte_to_lsb(magic_string[i], buffer);            // Hide the character's 8 bits in the LSBs

        if (fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)   // Write modified bytes
        {
            return e_failure;                   // Write failed
        }
    }

    return e_success;                           // Magic string hidden
}

/* ---------- encode_secret_file_extn_size ---------- */
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)      // Hides extension length
{
    char buffer[32];                            // 32 image bytes = one int

    if (fread(buffer, sizeof(char), 32, encInfo->fptr_src_image) != 32) // Read 32 bytes from image
    {
        return e_failure;                       // Read failed
    }

    encode_size_to_lsb(size, buffer);           // Hide the 32 bits of the number

    if (fwrite(buffer, sizeof(char), 32, encInfo->fptr_stego_image) != 32)  // Write modified bytes
    {
        return e_failure;                       // Write failed
    }

    return e_success;                           // Extension size hidden
}

/* ---------- encode_secret_file_extn ---------- */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)  // Hides the extension text
{
    char buffer[8];                             // 8 image bytes = 1 character

    for (size_t i = 0; i < strlen(file_extn); i++)              // For every extension character
    {
        if (fread(buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)   // Read 8 image bytes
        {
            return e_failure;                   // Read failed
        }

        encode_byte_to_lsb(file_extn[i], buffer);               // Hide the character

        if (fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)   // Write to output
        {
            return e_failure;                   // Write failed
        }
    }

    return e_success;                           // Extension hidden
}

/* ---------- encode_secret_file_size ---------- */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)     // Hides the secret file size
{
    char buffer[32];                            // 32 image bytes

    if (fread(buffer, sizeof(char), 32, encInfo->fptr_src_image) != 32) // Read 32 bytes
    {
        return e_failure;                       // Read failed
    }

    encode_size_to_lsb((int)file_size, buffer); // Hide the size (cast long to int)

    if (fwrite(buffer, sizeof(char), 32, encInfo->fptr_stego_image) != 32)  // Write to output
    {
        return e_failure;                       // Write failed
    }

    return e_success;                           // Size hidden
}

/* ---------- encode_secret_file_data ---------- */
Status encode_secret_file_data(EncodeInfo *encInfo)     // Hides the secret file's contents
{
    char buffer[8];                             // 8 image bytes per secret byte
    char data;                                  // One byte of secret data

    rewind(encInfo->fptr_secret);               // Start reading secret file from the beginning

    for (int i = 0; i < encInfo->size_secret_file; i++)         // Repeat for every secret byte
    {
        if (fread(&data, sizeof(char), 1, encInfo->fptr_secret) != 1)       // Read 1 byte of secret
        {
            return e_failure;                   // Read failed
        }

        if (fread(buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)   // Read 8 image bytes
        {
            return e_failure;                   // Read failed
        }

        encode_byte_to_lsb(data, buffer);       // Hide the secret byte in those 8 bytes

        if (fwrite(buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)   // Write to output
        {
            return e_failure;                   // Write failed
        }
    }

    return e_success;                           // All secret data hidden
}

/* ---------- copy_remaining_img_data ---------- */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)     // Copies the rest of the image untouched
{
    char ch;                                    // One-byte buffer

    while (fread(&ch, 1, 1, fptr_src) == 1)     // Read until end of file
    {
        if (fwrite(&ch, 1, 1, fptr_dest) != 1)  // Write the byte to output
        {
            return e_failure;                   // Write failed
        }
    }

    if (ferror(fptr_src))                       // Loop may have ended due to a read error
    {
        return e_failure;                       // Report failure
    }

    return e_success;                           // Rest of image copied
}

/* ---------- encode_byte_to_lsb ---------- */
Status encode_byte_to_lsb(char data, char *image_buffer)    // Hides 1 byte in the LSBs of 8 bytes
{
    for (int i = 0; i < 8; i++)                 // For each of the 8 bits
    {
        int bit = (data >> (7 - i)) & 1;        // Extract bit i, starting from the MSB

        image_buffer[i] = (image_buffer[i] & 0xFE) | bit;   // Clear image byte's LSB, then put the bit there
    }

    return e_success;                           // Done
}

/* ---------- encode_size_to_lsb ---------- */
Status encode_size_to_lsb(int size, char *imageBuffer)      // Hides a 32-bit number in 32 bytes
{
    for (int i = 0; i < 32; i++)                // For each of the 32 bits
    {
        int bit = ((unsigned int)size >> (31 - i)) & 1;     // Extract bit i, starting from the MSB

        imageBuffer[i] = (imageBuffer[i] & 0xFE) | bit;     // Clear LSB, then insert the bit
    }

    return e_success;                           // Done
}

/* ---------- do_encoding ---------- */
Status do_encoding(EncodeInfo *encInfo)         // Controls the whole encoding process
{
    if (open_files(encInfo) == e_failure)       // Step 1: open files
    {
        printf("ERROR: Opening files failed\n");
        return e_failure;
    }
    printf("Files opened successfully\n");

    if (check_capacity(encInfo) == e_failure)   // Step 2: check image has enough room
    {
        printf("ERROR: Capacity check failed\n");
        return e_failure;
    }
    printf("Image has enough capacity\n");

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)  // Step 3: copy header
    {
        printf("ERROR: Copying BMP header failed\n");
        return e_failure;
    }
    printf("BMP header copied successfully\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)        // Step 4: hide magic string
    {
        printf("ERROR: Encoding magic string failed\n");
        return e_failure;
    }

    if (encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_failure)  // Step 5: hide extension length
    {
        printf("ERROR: Encoding extension size failed\n");
        return e_failure;
    }

    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)   // Step 6: hide extension text
    {
        printf("ERROR: Encoding extension failed\n");
        return e_failure;
    }

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)   // Step 7: hide secret size
    {
        printf("ERROR: Encoding secret file size failed\n");
        return e_failure;
    }

    if (encode_secret_file_data(encInfo) == e_failure)      // Step 8: hide secret data
    {
        printf("ERROR: Encoding secret file data failed\n");
        return e_failure;
    }

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)  // Step 9: copy rest of image
    {
        printf("ERROR: Copying remaining image data failed\n");
        return e_failure;
    }

    printf("Encoding completed successfully\n");

    fclose(encInfo->fptr_src_image);            // Close source image
    fclose(encInfo->fptr_secret);               // Close secret file
    fclose(encInfo->fptr_stego_image);          // Close output image

    return e_success;                           // Encoding finished
}