/*
*   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in
*   all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*   THE SOFTWARE.
*/

/*

	Modified Lempel-Ziv method (LZW)

Basically finds common substrings and replaces them with a variable size code.
This is deterministic, and can be done on the fly.
Thus, the decompression procedure needs no input table, but tracks the way the table was built.


Referenced code from `ncompress 4.2`

*/

#include "slib/crypto/lzw.h"

#include "slib/core/base.h"
#include "slib/core/io.h"
#include "slib/core/memory_reader.h"
#include "slib/core/memory_output.h"

namespace slib
{

	namespace priv
	{
		namespace lzw
		{

			typedef long int code_int;
			typedef	unsigned char	char_type;
			typedef long int	 		count_int;
			typedef long int			cmp_code_int;

#define CHECK_GAP 10000
#define MAXCODE(n)	(1L << (n))
#define INIT_BITS 9			/* initial number of bits/code */

			/* the next two codes should not be changed lightly, as they must not	*/
			/* lie within the contiguous general code space.						*/
#define FIRST	257					/* first free entry 							*/
#define	CLEAR	256					/* table clear output code 						*/
			/* Defines for third byte of header 					*/

#define	MAGIC_1		(char_type)'\037'/* First byte of compressed file				*/
#define	MAGIC_2		(char_type)'\235'/* Second byte of compressed file				*/
#define BIT_MASK	0x1f			/* Mask for 'number of compresssion bits'		*/
			/* Masks 0x20 and 0x40 are free.  				*/
			/* I think 0x20 should mean that there is		*/
			/* a fourth header byte (for expansion).    	*/
#define BLOCK_MODE	0x80			/* Block compresssion if table is full and		*/
			/* compression rate is dropping flush tables	*/

#	define	htabof(i)				htab[i]
#	define	codetabof(i)			codetab[i]
#	define	tab_prefixof(i)			codetabof(i)
#	define	tab_suffixof(i)			((char_type *)(htab))[i]
#	define	de_stack				((char_type *)&(htab[HSIZE-1]))
#	define	clear_htab()			Base::resetMemory(htab, sizeof(htab), 0xff)
#	define	clear_tab_prefixof()	Base::zeroMemory(codetab, 256);

#		define BITS	16
#		define HSIZE	69001		/* 95% occupancy */

#define BUFSIZ 1024
#	define	IBUFSIZ	BUFSIZ	/* Defailt input buffer size							*/
#	define	OBUFSIZ	BUFSIZ	/* Default output buffer size							*/

#define	output(b,o,c,n)	{	char_type	*p = &(b)[(o)>>3];				\
							long		 i = ((long)(c))<<((o)&0x7);	\
							p[0] |= (char_type)(i);							\
							p[1] |= (char_type)(i>>8);						\
							p[2] |= (char_type)(i>>16);						\
							(o) += (n);										\
						}

#define	input(b,o,c,n,m){	char_type 	*p = &(b)[(o)>>3];			\
							(c) = ((((long)(p[0]))|((long)(p[1])<<8)|		\
									 ((long)(p[2])<<16))>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}

			// params


			static long Compress(IReader* reader, IWriter* writer)
			{
				long		hp;
				int			rpos;
				int			outbits;
				int			rlop;
				int			rsize;
				int			stcode;
				code_int	free_ent;
				int			boff;
				int			n_bits;
				int			ratio;
				long		checkpoint;
				code_int	extcode;
				union
				{
					long			code;
					struct
					{
						char_type		c;
						unsigned short	ent;
					} e;
				} fcode;

				/* states */
				count_int		htab[HSIZE];
				unsigned short	codetab[HSIZE];
				char_type		inbuf[IBUFSIZ + 64];	/* Input buffer									*/
				char_type		outbuf[OBUFSIZ + 2048]; /* Output buffer								*/
				long 			bytes_in;			/* Total number of byte from input				*/
				long	 		bytes_out;			/* Total number of byte to output				*/
				int				block_mode = BLOCK_MODE;/* Block compress mode -C compatible with 2.0*/
				int				maxbits = BITS;		/* user settable max # bits/code 				*/

				ratio = 0;
				checkpoint = CHECK_GAP;
				extcode = MAXCODE(n_bits = INIT_BITS) + 1;
				stcode = 1;
				free_ent = FIRST;

				Base::zeroMemory(outbuf, sizeof(outbuf));
				bytes_out = 0; bytes_in = 0;
				outbuf[0] = MAGIC_1;
				outbuf[1] = MAGIC_2;
				outbuf[2] = (char)(maxbits | block_mode);
				boff = outbits = (3 << 3);
				fcode.code = 0;

				clear_htab();

				while ((rsize = reader->read32(inbuf, IBUFSIZ)) > 0) {
					if (bytes_in == 0) {
						fcode.e.ent = inbuf[0];
						rpos = 1;
					} else {
						rpos = 0;
					}
					rlop = 0;
					do {
						if (free_ent >= extcode && fcode.e.ent < FIRST) {
							if (n_bits < maxbits) {
								boff = outbits = (outbits - 1) + ((n_bits << 3) - ((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
								if (++n_bits < maxbits) {
									extcode = MAXCODE(n_bits) + 1;
								} else {
									extcode = MAXCODE(n_bits);
								}
							} else {
								extcode = MAXCODE(16) + OBUFSIZ;
								stcode = 0;
							}
						}
						if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST) {
							long int rat;
							checkpoint = bytes_in + CHECK_GAP;
							if (bytes_in > 0x007fffff) {
								/* shift will overflow */
								rat = (bytes_out + (outbits >> 3)) >> 8;
								if (rat == 0) {
									/* Don't divide by zero */
									rat = 0x7fffffff;
								} else {
									rat = bytes_in / rat;
								}
							} else {
								rat = (bytes_in << 8) / (bytes_out + (outbits >> 3));	/* 8 fractional bits */
							}
							if (rat >= ratio) {
								ratio = (int)rat;
							} else {
								ratio = 0;
								clear_htab();
								output(outbuf, outbits, CLEAR, n_bits);
								boff = outbits = (outbits - 1) + ((n_bits << 3) -
									((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
								extcode = MAXCODE(n_bits = INIT_BITS) + 1;
								free_ent = FIRST;
								stcode = 1;
							}
						}

						if (outbits >= (OBUFSIZ << 3)) {
							if (writer->writeFully(outbuf, OBUFSIZ) != OBUFSIZ) {
								// write error
								return -1;
							}
							outbits -= (OBUFSIZ << 3);
							boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
							bytes_out += OBUFSIZ;
							Base::copyMemory(outbuf, outbuf + OBUFSIZ, (outbits >> 3) + 1);
							Base::zeroMemory(outbuf + (outbits >> 3) + 1, OBUFSIZ);
						}

						{
							int		i;
							i = rsize - rlop;
							if ((code_int)i > extcode - free_ent)	i = (int)(extcode - free_ent);
							int imax = ((sizeof(outbuf) - 32) * 8 - outbits) / n_bits;
							if (i > imax) {
								i = imax;
							}
							if (!stcode && (long)i > checkpoint - bytes_in) {
								i = (int)(checkpoint - bytes_in);
							}
							rlop += i;
							bytes_in += i;
						}
						goto next;
					hfound:
						fcode.e.ent = codetabof(hp);
					next:
						if (rpos >= rlop) {
							goto endlop;
						}
					next2:
						fcode.e.c = inbuf[rpos++];
						{
							code_int	i;
#define fc fcode.code
							hp = (((long)(fcode.e.c)) << (BITS - 8)) ^ (long)(fcode.e.ent);

							if ((i = htabof(hp)) == fc) {
								goto hfound;
							}
							if (i != -1) {
								long		disp;
								disp = (HSIZE - hp) - 1;	/* secondary hash (after G. Knott) */
								do {
									if ((hp -= disp) < 0)	hp += HSIZE;
									if ((i = htabof(hp)) == fc)
										goto hfound;
								} while (i != -1);
							}
						}
#undef	fc

						output(outbuf, outbits, fcode.e.ent, n_bits);

						{
							long	fc;
							fc = fcode.code;
							fcode.e.ent = fcode.e.c;
							if (stcode) {
								codetabof(hp) = (unsigned short)free_ent++;
								htabof(hp) = fc;
							}
						}

						goto next;

					endlop:
						if (fcode.e.ent >= FIRST && rpos < rsize) {
							goto next2;
						}
						if (rpos > rlop)
						{
							bytes_in += rpos - rlop;
							rlop = rpos;
						}
					} while (rlop < rsize);
				}

				if (rsize < 0) {
					// read error
					return -1;
				}

				if (bytes_in > 0) {
					output(outbuf, outbits, fcode.e.ent, n_bits);
				}

				if (writer->writeFully(outbuf, (outbits + 7) >> 3) != (outbits + 7) >> 3) {
					// write error
					return -1;
				}

				bytes_out += (outbits + 7) >> 3;

				return bytes_out;
			}

			static sl_bool Decompress(IReader* reader, IWriter* writer)
			{
				char_type 		*stackp;
				code_int		 code;
				int				 finchar;
				code_int		 oldcode;
				code_int		 incode;
				int				 inbits;
				int				 posbits;
				int				 outpos;
				int				 insize;
				int				 bitmask;
				code_int		 free_ent;
				code_int		 maxcode;
				code_int		 maxmaxcode;
				int				 n_bits;
				int				 rsize;

				// states
				count_int		htab[HSIZE];
				unsigned short	codetab[HSIZE];
				char_type		inbuf[IBUFSIZ + 64];	/* Input buffer									*/
				char_type		outbuf[OBUFSIZ + 2048]; /* Output buffer								*/
				long 			bytes_in;			/* Total number of byte from input				*/
				int				block_mode = BLOCK_MODE;/* Block compress mode -C compatible with 2.0*/
				int				maxbits = BITS;		/* user settable max # bits/code 				*/

				bytes_in = 0;
				insize = 0;

				rsize = 0;
				while (insize < 3 && (rsize = reader->read32(inbuf + insize, IBUFSIZ)) > 0) {
					insize += rsize;
				}

				if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2) {
					if (rsize < 0) {
						// reading error
						return sl_false;
					}
					if (insize > 0) {
						// not in compressed format
						return sl_false;
					}
					return sl_true;
				}

				maxbits = inbuf[2] & BIT_MASK;
				block_mode = inbuf[2] & BLOCK_MODE;
				maxmaxcode = MAXCODE(maxbits);

				if (maxbits > BITS) {
					// compressed with `maxbits` bits, can only handle `BITS` bits
					return sl_false;
				}

				bytes_in = insize;
				maxcode = MAXCODE(n_bits = INIT_BITS) - 1;
				bitmask = (1 << n_bits) - 1;
				oldcode = -1;
				finchar = 0;
				outpos = 0;
				posbits = 3 << 3;

				free_ent = ((block_mode) ? FIRST : 256);

				clear_tab_prefixof();	/* As above, initialize the first
										256 entries in the table. */

				for (code = 255; code >= 0; --code) {
					tab_suffixof(code) = (char_type)code;
				}

				do
				{
			resetbuf:
					{
						int	i;
						int				e;
						int				o;

						o = posbits >> 3;
						e = o <= insize ? insize - o : 0;

						for (i = 0; i < e; ++i)
							inbuf[i] = inbuf[i + o];

						insize = e;
						posbits = 0;
					}

					if (insize < sizeof(inbuf) - IBUFSIZ)
					{
						if ((rsize = reader->read32(inbuf + insize, IBUFSIZ)) < 0) {
							// reading error
							return sl_false;
						}
						insize += rsize;
					}

					inbits = ((rsize > 0) ? (insize - insize % n_bits) << 3 : (insize << 3) - (n_bits - 1));

					while (inbits > posbits) {
						if (free_ent > maxcode) {
							posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
							++n_bits;
							if (n_bits == maxbits) {
								maxcode = maxmaxcode;
							} else {
								maxcode = MAXCODE(n_bits) - 1;
							}
							bitmask = (1 << n_bits) - 1;
							goto resetbuf;
						}

						input(inbuf, posbits, code, n_bits, bitmask);

						if (oldcode == -1) {
							if (code >= 256) {
								// corrupt input
								return sl_false;
							}
							outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
							continue;
						}

						if (code == CLEAR && block_mode) {
							clear_tab_prefixof();
							free_ent = FIRST - 1;
							posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
							maxcode = MAXCODE(n_bits = INIT_BITS) - 1;
							bitmask = (1 << n_bits) - 1;
							goto resetbuf;
						}

						incode = code;
						stackp = de_stack;

						if (code >= free_ent) {
							/* Special case for KwKwK string.	*/
							if (code > free_ent) {
								// corrupt input
								return sl_false;
							}
							*--stackp = (char_type)finchar;
							code = oldcode;
						}

						while ((cmp_code_int)code >= (cmp_code_int)256) {
							/* Generate output characters in reverse order */
							*--stackp = tab_suffixof(code);
							code = tab_prefixof(code);
						}

						*--stackp = (char_type)(finchar = tab_suffixof(code));

						/* And put them out in forward order */
						{
							int	i;

							if (outpos + (i = (int)(de_stack - stackp)) >= OBUFSIZ) {
								do {
									if (i > OBUFSIZ - outpos) {
										i = OBUFSIZ - outpos;
									}
									if (i > 0) {
										Base::copyMemory(outbuf + outpos, stackp, i);
										outpos += i;
									}
									if (outpos >= OBUFSIZ) {
										if (writer->writeFully(outbuf, outpos) != outpos) {
											// writing error
											return sl_false;
										}
										outpos = 0;
									}
									stackp += i;
								} while ((i = (int)(de_stack - stackp)) > 0);
							} else {
								Base::copyMemory(outbuf + outpos, stackp, i);
								outpos += i;
							}
						}

						if ((code = free_ent) < maxmaxcode) {
							/* Generate the new entry. */
							tab_prefixof(code) = (unsigned short)oldcode;
							tab_suffixof(code) = (char_type)finchar;
							free_ent = code + 1;
						}

						oldcode = incode;	/* Remember previous code.	*/
					}


					bytes_in += rsize;

				} while (rsize > 0);

				if (outpos > 0 && writer->writeFully(outbuf, outpos) != outpos) {
					// writing error
					return sl_false;
				}

				return sl_true;
			}

		}
	}

	using namespace priv::lzw;

	sl_bool LZW::compress(IReader* reader, IWriter* writer)
	{
		return Compress(reader, writer) > 0;
	}

	sl_bool LZW::decompress(IReader* reader, IWriter* writer)
	{
		return Decompress(reader, writer);
	}

	Memory LZW::compress(const void* data, sl_size size)
	{
		if (size) {
			MemoryReader reader(data, size);
			MemoryOutput writer;
			if (Compress(&reader, &writer) > 0) {
				return writer.getData();
			}
		}
		return sl_null;
	}

	Memory LZW::decompress(const void* data, sl_size size)
	{
		if (size) {
			MemoryReader reader(data, size);
			MemoryOutput writer;
			if (Decompress(&reader, &writer)) {
				return writer.getData();
			}
		}
		return sl_null;
	}

}
