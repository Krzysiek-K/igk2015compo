/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.


These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#ifndef _MD5_H
#define _MD5_H


class MD5Hash {
public:

	MD5Hash() { context = &ctx; Init(); }
	MD5Hash(const MD5Hash &h) { context = &ctx; ctx = h.ctx; }

	void Init();
	void Update(unsigned char *, unsigned int);
	void Final(unsigned char [16]);

    static void Compute(const void *data,int size,unsigned char hash[16])
    {
        MD5Hash h;
        h.Init();
        h.Update((unsigned char*)data,(unsigned int)size);
        h.Final(hash);
    }

private:
	/* MD5 context. */
	typedef struct {
	  DWORD state[4];                                   /* state (ABCD) */
	  DWORD count[2];        /* number of bits, modulo 2^64 (lsb first) */
	  unsigned char buffer[64];                         /* input buffer */
	} MD5_CTX;

	MD5_CTX	ctx;
	MD5_CTX	*context;
};


#endif
