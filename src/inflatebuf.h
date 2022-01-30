/* 
 * Not copyrighted -- provided to the public domain
 * inflatebuf.h: inflatebuf.c header 
 **/

#ifndef __INFLATE_BUF_H__
#define __INFLATE_BUF_H__
int inflate_buffer(unsigned char *source, unsigned char **dest, 
                   uint lenIn, uint *lenOut);
#endif //__INFLATE_BUF_H__
