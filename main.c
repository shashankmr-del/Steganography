/*
  NAME: SHASHANK M R
  DISCRIPTION: STEGNOGRAPHY
  DATE: 28-09-2026
*/                                      // Header comment: author, project, date (ignored by compiler)
#include <stdio.h>                      // printf, fprintf, stderr
#include <string.h>                     // strcmp, to compare the -e / -d option
#include "encode.h"                     // EncodeInfo struct + encoding function declarations
#include "decode.h"                     // DecodeInfo struct + decoding function declarations
#include "types.h"                      // Shared enums: OperationType, Status (e_success, e_failure)

OperationType check_operation_type(char *);   // Prototype: tells compiler this function exists below

int main(int argc, char *argv[])        // Entry point. argc = argument count, argv = argument strings
{
    if (argc < 2)                       // No option given (only ./a.out typed)
    {
        fprintf(stderr, "Usage: %s -e <source.bmp> <secret_file> [stego.bmp]\n", argv[0]);  // Print usage to error stream
        return 1;                       // Exit with error code 1
    }

    OperationType op = check_operation_type(argv[1]);   // Check argv[1] (-e or -d) and store the operation type

    if (op == e_encode)                 // User asked for encoding
    {
        if (argc < 4)                   // Need: ./a.out -e source.bmp secret_file (4 args minimum)
        {
            fprintf(stderr, "Usage: %s -e <source.bmp> <secret_file> [stego.bmp]\n", argv[0]);  // Show correct usage
            return 1;                   // Exit with error
        }

        EncodeInfo encInfo;             // Structure holding file pointers, names, sizes for encoding

        if (read_and_validate_encode_args(argv, &encInfo) == e_success)   // Validate .bmp/secret file, fill encInfo
        {
            printf("1 -> Encoding arguments validated\n");   // Progress message

            if (do_encoding(&encInfo) == e_success)          // Run the encoding (hide secret bits in image)
            {
                printf("Encoding done\n");                   // Success message
            }
            else                        // Encoding failed
            {
                printf("Encoding failed\n");                 // Error message
                return 1;               // Exit with error
            }
        }
        else                            // Argument validation failed
        {
            printf("arguments are wrong\n");                 // Error message
            return 1;                   // Exit with error
        }
    }
    else if (op == e_decode)            // User asked for decoding
    {
        if (argc < 3)                   // Need: ./a.out -d stego.bmp (3 args minimum)
        {
            fprintf(stderr, "Usage: %s -d <stego.bmp> [output_name]\n", argv[0]);   // Show correct usage
            return 1;                   // Exit with error
        }

        DecodeInfo decInfo;             // Structure holding file pointers and names for decoding

        if (read_and_validate_decode_args(argv, &decInfo) == e_success)   // Validate stego file/output name, fill decInfo
        {
            printf("1 -> Decoding arguments validated\n");   // Progress message

            if (do_decoding(&decInfo) == e_success)          // Run decoding (extract hidden data to file)
            {
                printf("Decoding done\n");                   // Success message
            }
            else                        // Decoding failed
            {
                printf("Decoding failed\n");                 // Error message
                return 1;               // Exit with error
            }
        }
        else                            // Argument validation failed
        {
            printf("arguments are wrong\n");                 // Error message
            return 1;                   // Exit with error
        }
    }
    else                                // Option was neither -e nor -d
    {
        printf("invalid operation\n");  // Error message
        return 1;                       // Exit with error
    }

    return 0;                           // Program finished successfully
}

OperationType check_operation_type(char *symbol)   // Converts the option string into an OperationType
{
    if (strcmp(symbol, "-e") == 0)      // strcmp returns 0 when strings match
        return e_encode;                // "-e" means encode

    if (strcmp(symbol, "-d") == 0)      // Check for decode option
        return e_decode;                // "-d" means decode

    return e_unsupported;               // Anything else is not supported
}

// TO RUN THE CODE

/* 

gcc main.c encode.c decode.c -o a.out or gcc *.c    // Compile all source files into executable a.out

./a.out -e beautiful.bmp secret.txt stego.bmp       // Hide secret.txt inside beautiful.bmp, output stego.bmp

./a.out -d stego.bmp                                // Extract hidden data from stego.bmp (saved as decoded.txt)

cat decoded.txt                                     // Display the extracted secret message

*/