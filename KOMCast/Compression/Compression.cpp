#include "Compression.h"

// These 2 are stolen from https://www.experts-exchange.com/articles/3189/In-Memory-Compression-and-Decompression-Using-ZLIB.html

inline int GetMaxCompressedLen(int nLenSrc)
{
    int n16kBlocks = (nLenSrc + 16383) / 16384; // round up any fraction of a block
    return (nLenSrc + 6 + (n16kBlocks * 5));
}

int zlib_compress_mem(char* _source, uint32_t _inSize, char** _outBuffer, uint32_t* _outSize)
{
    if (_source == NULL || _inSize == NULL || _outBuffer == NULL || _outSize == NULL)
        return Z_ERRNO;

    if (*_outBuffer == NULL)
        *_outBuffer = (char*)malloc(GetMaxCompressedLen(_inSize));

    z_stream zInfo = { 0 };
    zInfo.total_in = zInfo.avail_in = _inSize;
    zInfo.total_out = zInfo.avail_out = GetMaxCompressedLen(_inSize);
    zInfo.next_in = (BYTE*)_source;
    zInfo.next_out = (Bytef*)*_outBuffer;

    int nErr, nRet = -1;
    nErr = deflateInit(&zInfo, Z_BEST_COMPRESSION); // zlib function
    if (nErr == Z_OK)
    {
        nErr = deflate(&zInfo, Z_FINISH);              // zlib function
        if (nErr == Z_STREAM_END) {
            nRet = zInfo.total_out;
        }
    }
    deflateEnd(&zInfo);    // zlib function
    *_outSize = zInfo.total_out;
    return (nRet);
}

// outSize is a must know variable, store it somewhere.
int zlib_decompress_mem(char* _source, uint32_t _inSize, char** _outBuffer, uint32_t _outSize)
{
    if (_source == NULL || _inSize == NULL || _outBuffer == NULL || _outSize == NULL)
        return Z_ERRNO;

    if (*_outBuffer == NULL) {
        *_outBuffer = (char*)malloc(_outSize + 1);
        memset(*_outBuffer, 0, _outSize + 1);
    }

    z_stream zInfo = { 0 };
    zInfo.total_in = zInfo.avail_in = _inSize;
    zInfo.total_out = zInfo.avail_out = _outSize;
    zInfo.next_in = (BYTE*)_source;
    zInfo.next_out = (Bytef*)*_outBuffer;

    int nErr, nRet = -1;
    nErr = inflateInit(&zInfo);               // zlib function
    if (nErr == Z_OK) {
        nErr = inflate(&zInfo, Z_FINISH);     // zlib function
        if (nErr == Z_STREAM_END) {
            nRet = zInfo.total_out;
        }
    }
    inflateEnd(&zInfo);   // zlib function
    return nRet; // -1 or len of output
}

int zlib_compress_file(const char* _inFile, const char* _outFile)
{
    FILE* _finFile = NULL, * _foutFile = NULL;

    if (!(_inFile != NULL && _outFile != NULL))
        return Z_ERRNO;

    fopen_s(&_finFile, _inFile, "rb");
    fopen_s(&_foutFile, _outFile, "wb");

    if (_finFile != NULL && _foutFile != NULL)
    {
        auto _return = zlib_compress(_finFile, _foutFile, 9);
        fclose(_finFile);
        fclose(_foutFile);

        return _return;
    }

    return Z_ERRNO;
}

int zlib_decompress_file(const char* _inFile, const char* _outFile)
{
    FILE* _finFile = NULL, * _foutFile = NULL;

    if (!(_inFile != NULL && _outFile != NULL))
        return Z_ERRNO;

    fopen_s(&_finFile, _inFile, "rb");
    fopen_s(&_foutFile, _outFile, "wb");

    if (_finFile != NULL && _foutFile != NULL)
    {
        auto _return = zlib_decompress(_finFile, _foutFile);
        fclose(_finFile);
        fclose(_foutFile);

        return _return;
    }

    return Z_ERRNO;
}

int zlib_compress(FILE* source, FILE* dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

int zlib_decompress(FILE* source, FILE* dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void zlib_error(int ret)
{
    switch (ret)
    {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}