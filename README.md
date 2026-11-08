# Steganography (LSB Image Steganography in C)

A command-line tool written in C that hides a secret file inside a 24-bit BMP image and extracts it back. It uses **Least Significant Bit (LSB) steganography**: each bit of the secret data replaces the lowest bit of one image byte, changing every colour value by at most 1, so the stego image looks identical to the original.

## Features
- Hide any small file (text, source code, etc.) inside a `.bmp` image
- Extract the hidden file back with its original extension
- Magic string (`#*`) check so images without hidden data are rejected
- Capacity check before encoding
- Argument validation and clear error messages

## Project structure
| File | Purpose |
|---|---|
| `main.c` | Entry point: reads `-e` / `-d` and calls encode or decode |
| `encode.c`, `encode.h` | Encoding logic and `EncodeInfo` structure |
| `decode.c`, `decode.h` | Decoding logic and `DecodeInfo` structure |
| `types.h` | Shared types (`uint`, `Status`, `OperationType`) |
| `common.h` | Magic string definition |
| `makefile` | Build script |

## Build
```bash
gcc *.c -o a.out
```

## Usage
**Encode** (hide a file in an image):
```bash
./a.out -e <source.bmp> <secret_file> [stego.bmp]
```
Example:
```bash
./a.out -e beautiful.bmp secret.txt stego.bmp
```
If no output name is given, the result is saved as `stego.bmp`.

**Decode** (extract the hidden file):
```bash
./a.out -d <stego.bmp> [output_name]
```
Example:
```bash
./a.out -d stego.bmp
cat decoded.txt
```
If no output name is given, the file is saved as `decoded` plus the original extension (for example `decoded.txt`).

## How it works
Data is stored in the pixel bytes right after the 54-byte BMP header, in this order:

1. Magic string `#*` (16 bits)
2. Secret file extension length (32 bits)
3. Secret file extension (8 bits per character)
4. Secret file size (32 bits)
5. Secret file data (8 image bytes per secret byte)

The rest of the image is copied unchanged. Decoding reads the same fields in the same order.

**Capacity:** an image can hide roughly `width × height × 3 / 8` bytes. For example, a 1024×768 image holds about 290 KB.

## Limitations
- Only 24-bit BMP images are supported
- The secret file extension can be at most 4 characters including the dot (such as `.txt`, `.c`, `.sh`)
- No encryption or password protection

## Author
Shashank M R
