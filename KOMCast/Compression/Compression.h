#pragma once
#ifndef _COMPRESSION_H_
#define _COMPRESSION_H_

// https://zlib.net/zlib_how.html

#include <stdio.h>
#include <string.h>
#include <assert.h>
#define ZLIB_WINAPI
#include <zlib.h>
#include <stdint.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384


int zlib_compress_mem(char* _source, uint32_t _inSize, char** _outBuffer, uint32_t* _outSize);
int zlib_decompress_mem(char* _source, uint32_t _inSize, char** _outBuffer, uint32_t _outSize);

int zlib_compress_file(const char* _inFile, const char* _outFile);
int zlib_decompress_file(const char* _inFile, const char* _outFile);

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int zlib_compress(FILE* source, FILE* dest, int level);

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int zlib_decompress(FILE* source, FILE* dest);

/* report a zlib or i/o error */
void zlib_error(int ret);

#endif