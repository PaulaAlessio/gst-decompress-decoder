/* 
 * Not copyrighted -- provided to the public domain
 * inflatebuf.c: use of zlib's inflate() to decompress a buffer
 * Code taken from zlib public repo (file zpipe.c) 
 **/

#include "stdlib.h"
#include "zlib.h"

/* zlib chunks */
#define CHUNK 16384

int inflate_buffer(unsigned char *source, unsigned char **dest, uint lenIn, uint *lenOut)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char *in = source;
  unsigned char *out;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  // This function is needed to be called instead of inflateInit
  // in order for it to work for G-streams
  ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK)   return ret;

  uInt dest_size = lenIn;
  uInt realloc_size = dest_size > CHUNK ? dest_size : CHUNK;
  uInt dest_free_size = dest_size;
  *dest = malloc(dest_size);
  if(*dest == NULL) return -1;
  out = *dest;
  *lenOut = 0;

  uInt remainingIn = lenIn;
  /* decompress until deflate stream ends or end of file */
  do
  {
    int next_chunk_length = remainingIn >= CHUNK ? CHUNK : remainingIn;
    strm.avail_in = next_chunk_length;

    if (strm.avail_in == 0)
        break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      if(dest_free_size < CHUNK)
      {
        unsigned char * new_dest;
        new_dest = realloc(*dest, dest_size + realloc_size);
        if(!new_dest)
        {
          free(*dest);
          return Z_DATA_ERROR;
        }
        *dest = new_dest;
        dest_size += realloc_size;
        dest_free_size += realloc_size;
        out = *dest + *lenOut;
      }
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      if(ret == Z_STREAM_ERROR)  /* state not clobbered */
      {
        (void)inflateEnd(&strm);
        free (*dest);
        return ret;
      }
      switch (ret)
      {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;     /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void)inflateEnd(&strm);
          free (*dest);
          return ret;
      }
      have = CHUNK - strm.avail_out;
      *lenOut += have;
      out += have;
      dest_free_size -= have;

    } while (strm.avail_out == 0);

    in += next_chunk_length;
    remainingIn -= next_chunk_length;
    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  if(*lenOut)
  {
    unsigned char * new_dest;
    new_dest = realloc(*dest, *lenOut);
    if(!new_dest)
    {
      free(*dest);
      return Z_DATA_ERROR;
    }
    *dest = new_dest;
  }
  else
  {
    free(*dest);
    return Z_DATA_ERROR;
  }

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}



