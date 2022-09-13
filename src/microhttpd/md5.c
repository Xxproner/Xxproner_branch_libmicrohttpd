/*
     This file is part of GNU libmicrohttpd
     Copyright (C) 2022 Evgeny Grin (Karlson2k)

     GNU libmicrohttpd is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library.
     If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file microhttpd/md5.c
 * @brief  Calculation of MD5 digest as defined in RFC 1321
 * @author Karlson2k (Evgeny Grin)
 */

#include "md5.h"

#include <string.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif /* HAVE_MEMORY_H */
#include "mhd_bithelpers.h"
#include "mhd_assert.h"

/**
 * Initialise structure for MD5 calculation.
 *
 * @param ctx the calculation context
 */
void
MHD_MD5_init (struct Md5Ctx *ctx)
{
  /* Initial hash values, see RFC 1321, Clause 3.3 (step 3). */
  /* Note: values specified in RFC by bytes and should be loaded in
           little-endian mode, therefore hash values here are initialised with
           original bytes used in little-endian order. */
  ctx->H[0] = UINT32_C (0x67452301);
  ctx->H[1] = UINT32_C (0xefcdab89);
  ctx->H[2] = UINT32_C (0x98badcfe);
  ctx->H[3] = UINT32_C (0x10325476);

  /* Initialise the number of bytes. */
  ctx->count = 0;
}


/**
 * Base of MD5 transformation.
 * Gets full 64 bytes block of data and updates hash values;
 * @param H     hash values
 * @param M     the data buffer with #MD5_BLOCK_SIZE bytes block
 */
static void
md5_transform (uint32_t H[MD5_HASH_SIZE_WORDS],
               const void *M)
{
  /* Working variables,
     See RFC 1321, Clause 3.4 (step 4). */
  uint32_t A = H[0];
  uint32_t B = H[1];
  uint32_t C = H[2];
  uint32_t D = H[3];

  /* The data buffer. See RFC 1321, Clause 3.4 (step 4). */
  uint32_t X[16];

#ifndef _MHD_GET_32BIT_LE_UNALIGNED
  if (0 != (((uintptr_t) M) % _MHD_UINT32_ALIGN))
  { /* The input data is unaligned. */
    /* Copy the unaligned input data to the aligned buffer. */
    memcpy (X, M, sizeof(X));
    /* The X[] buffer itself will be used as the source of the data,
     * but the data will be reloaded in correct bytes order on
     * the next steps. */
    M = (const void *) X;
  }
#endif /* _MHD_GET_32BIT_LE_UNALIGNED */

  /* Four auxiliary functions, see RFC 1321, Clause 3.4 (step 4). */
  /* Some optimisations used. */
/* #define F_FUNC(x,y,z) (((x)&(y)) | ((~(x))&(z))) */ /* Original version */
#define F_FUNC(x,y,z) ((((y) ^ (z)) & (x)) ^ (z))
/* #define G_FUNC_1(x,y,z) (((x)&(z)) | ((y)&(~(z)))) */ /* Original version */
/* #define G_FUNC_2(x,y,z) UINT32_C(0) */ /* Original version */
#ifndef MHD_FAVOR_SMALL_CODE
#  define G_FUNC_1(x,y,z) ((~(z)) & (y))
#  define G_FUNC_2(x,y,z) ((z) & (x))
#else  /* MHD_FAVOR_SMALL_CODE */
#  define G_FUNC_1(x,y,z) ((((x) ^ (y)) & (z)) ^ (y))
#  define G_FUNC_2(x,y,z) UINT32_C(0)
#endif /* MHD_FAVOR_SMALL_CODE */
#define H_FUNC(x,y,z) ((x) ^ (y) ^ (z)) /* Original version */
/* #define I_FUNC(x,y,z) ((y) ^ ((x) | (~(z)))) */ /* Original version */
#define I_FUNC(x,y,z) (((~(z)) | (x)) ^ (y))

  /* One step of round 1 of MD5 computation, see RFC 1321, Clause 3.4 (step 4).
     The original function was modified to use X[k] and T[i] as
     direct inputs. */
  /* The input data is loaded in correct format before
     calculations on each step. */
#define MD5STEP_R1(va,vb,vc,vd,vX,vs,vT) do {          \
    (va) += (vX) + (vT);                               \
    (va) += F_FUNC((vb),(vc),(vd));                    \
    (va) = _MHD_ROTL32((va),(vs)) + (vb); } while (0)

  /* Get value of X(k) from input data buffer.
     See RFC 1321 Clause 3.4 (step 4). */
#define GET_X_FROM_DATA(buf,t) \
  _MHD_GET_32BIT_LE (((const uint32_t*) (buf)) + (t))

  /* Round 1. */

  MD5STEP_R1 (A, B, C, D, X[0] = GET_X_FROM_DATA (M, 0), 7, \
              UINT32_C (0xd76aa478));
  MD5STEP_R1 (D, A, B, C, X[1] = GET_X_FROM_DATA (M, 1), 12, \
              UINT32_C (0xe8c7b756));
  MD5STEP_R1 (C, D, A, B, X[2] = GET_X_FROM_DATA (M, 2), 17, \
              UINT32_C (0x242070db));
  MD5STEP_R1 (B, C, D, A, X[3] = GET_X_FROM_DATA (M, 3), 22, \
              UINT32_C (0xc1bdceee));

  MD5STEP_R1 (A, B, C, D, X[4] = GET_X_FROM_DATA (M, 4), 7, \
              UINT32_C (0xf57c0faf));
  MD5STEP_R1 (D, A, B, C, X[5] = GET_X_FROM_DATA (M, 5), 12, \
              UINT32_C (0x4787c62a));
  MD5STEP_R1 (C, D, A, B, X[6] = GET_X_FROM_DATA (M, 6), 17, \
              UINT32_C (0xa8304613));
  MD5STEP_R1 (B, C, D, A, X[7] = GET_X_FROM_DATA (M, 7), 22, \
              UINT32_C (0xfd469501));

  MD5STEP_R1 (A, B, C, D, X[8] = GET_X_FROM_DATA (M, 8), 7, \
              UINT32_C (0x698098d8));
  MD5STEP_R1 (D, A, B, C, X[9] = GET_X_FROM_DATA (M, 9), 12, \
              UINT32_C (0x8b44f7af));
  MD5STEP_R1 (C, D, A, B, X[10] = GET_X_FROM_DATA (M, 10), 17, \
              UINT32_C (0xffff5bb1));
  MD5STEP_R1 (B, C, D, A, X[11] = GET_X_FROM_DATA (M, 11), 22, \
              UINT32_C (0x895cd7be));

  MD5STEP_R1 (A, B, C, D, X[12] = GET_X_FROM_DATA (M, 12), 7, \
              UINT32_C (0x6b901122));
  MD5STEP_R1 (D, A, B, C, X[13] = GET_X_FROM_DATA (M, 13), 12, \
              UINT32_C (0xfd987193));
  MD5STEP_R1 (C, D, A, B, X[14] = GET_X_FROM_DATA (M, 14), 17, \
              UINT32_C (0xa679438e));
  MD5STEP_R1 (B, C, D, A, X[15] = GET_X_FROM_DATA (M, 15), 22, \
              UINT32_C (0x49b40821));

  /* One step of round 2 of MD5 computation, see RFC 1321, Clause 3.4 (step 4).
     The original function was modified to use X[k] and T[i] as
     direct inputs. */
#define MD5STEP_R2(va,vb,vc,vd,vX,vs,vT) do {         \
    (va) += (vX) + (vT);                              \
    (va) += G_FUNC_1((vb),(vc),(vd));                 \
    (va) += G_FUNC_2((vb),(vc),(vd));                 \
    (va) = _MHD_ROTL32((va),(vs)) + (vb); } while (0)

  /* Round 2. */

  MD5STEP_R2 (A, B, C, D, X[1], 5, UINT32_C (0xf61e2562));
  MD5STEP_R2 (D, A, B, C, X[6], 9, UINT32_C (0xc040b340));
  MD5STEP_R2 (C, D, A, B, X[11], 14, UINT32_C (0x265e5a51));
  MD5STEP_R2 (B, C, D, A, X[0], 20, UINT32_C (0xe9b6c7aa));

  MD5STEP_R2 (A, B, C, D, X[5], 5, UINT32_C (0xd62f105d));
  MD5STEP_R2 (D, A, B, C, X[10], 9, UINT32_C (0x02441453));
  MD5STEP_R2 (C, D, A, B, X[15], 14, UINT32_C (0xd8a1e681));
  MD5STEP_R2 (B, C, D, A, X[4], 20, UINT32_C (0xe7d3fbc8));

  MD5STEP_R2 (A, B, C, D, X[9], 5, UINT32_C (0x21e1cde6));
  MD5STEP_R2 (D, A, B, C, X[14], 9, UINT32_C (0xc33707d6));
  MD5STEP_R2 (C, D, A, B, X[3], 14, UINT32_C (0xf4d50d87));
  MD5STEP_R2 (B, C, D, A, X[8], 20, UINT32_C (0x455a14ed));

  MD5STEP_R2 (A, B, C, D, X[13], 5, UINT32_C (0xa9e3e905));
  MD5STEP_R2 (D, A, B, C, X[2], 9, UINT32_C (0xfcefa3f8));
  MD5STEP_R2 (C, D, A, B, X[7], 14, UINT32_C (0x676f02d9));
  MD5STEP_R2 (B, C, D, A, X[12], 20, UINT32_C (0x8d2a4c8a));

  /* One step of round 3 of MD5 computation, see RFC 1321, Clause 3.4 (step 4).
     The original function was modified to use X[k] and T[i] as
     direct inputs. */
#define MD5STEP_R3(va,vb,vc,vd,vX,vs,vT) do {         \
    (va) += (vX) + (vT);                              \
    (va) += H_FUNC((vb),(vc),(vd));                   \
    (va) = _MHD_ROTL32((va),(vs)) + (vb); } while (0)

  /* Round 3. */

  MD5STEP_R3 (A, B, C, D, X[5], 4, UINT32_C (0xfffa3942));
  MD5STEP_R3 (D, A, B, C, X[8], 11, UINT32_C (0x8771f681));
  MD5STEP_R3 (C, D, A, B, X[11], 16, UINT32_C (0x6d9d6122));
  MD5STEP_R3 (B, C, D, A, X[14], 23, UINT32_C (0xfde5380c));

  MD5STEP_R3 (A, B, C, D, X[1], 4, UINT32_C (0xa4beea44));
  MD5STEP_R3 (D, A, B, C, X[4], 11, UINT32_C (0x4bdecfa9));
  MD5STEP_R3 (C, D, A, B, X[7], 16, UINT32_C (0xf6bb4b60));
  MD5STEP_R3 (B, C, D, A, X[10], 23, UINT32_C (0xbebfbc70));

  MD5STEP_R3 (A, B, C, D, X[13], 4, UINT32_C (0x289b7ec6));
  MD5STEP_R3 (D, A, B, C, X[0], 11, UINT32_C (0xeaa127fa));
  MD5STEP_R3 (C, D, A, B, X[3], 16, UINT32_C (0xd4ef3085));
  MD5STEP_R3 (B, C, D, A, X[6], 23, UINT32_C (0x04881d05));

  MD5STEP_R3 (A, B, C, D, X[9], 4, UINT32_C (0xd9d4d039));
  MD5STEP_R3 (D, A, B, C, X[12], 11, UINT32_C (0xe6db99e5));
  MD5STEP_R3 (C, D, A, B, X[15], 16, UINT32_C (0x1fa27cf8));
  MD5STEP_R3 (B, C, D, A, X[2], 23, UINT32_C (0xc4ac5665));

  /* One step of round 4 of MD5 computation, see RFC 1321, Clause 3.4 (step 4).
     The original function was modified to use X[k] and T[i] as
     direct inputs. */
#define MD5STEP_R4(va,vb,vc,vd,vX,vs,vT) do {         \
    (va) += (vX) + (vT);                              \
    (va) += I_FUNC((vb),(vc),(vd));                   \
    (va) = _MHD_ROTL32((va),(vs)) + (vb); } while (0)

  /* Round 4. */

  MD5STEP_R4 (A, B, C, D, X[0], 6, UINT32_C (0xf4292244));
  MD5STEP_R4 (D, A, B, C, X[7], 10, UINT32_C (0x432aff97));
  MD5STEP_R4 (C, D, A, B, X[14], 15, UINT32_C (0xab9423a7));
  MD5STEP_R4 (B, C, D, A, X[5], 21, UINT32_C (0xfc93a039));

  MD5STEP_R4 (A, B, C, D, X[12], 6, UINT32_C (0x655b59c3));
  MD5STEP_R4 (D, A, B, C, X[3], 10, UINT32_C (0x8f0ccc92));
  MD5STEP_R4 (C, D, A, B, X[10], 15, UINT32_C (0xffeff47d));
  MD5STEP_R4 (B, C, D, A, X[1], 21, UINT32_C (0x85845dd1));

  MD5STEP_R4 (A, B, C, D, X[8], 6, UINT32_C (0x6fa87e4f));
  MD5STEP_R4 (D, A, B, C, X[15], 10, UINT32_C (0xfe2ce6e0));
  MD5STEP_R4 (C, D, A, B, X[6], 15, UINT32_C (0xa3014314));
  MD5STEP_R4 (B, C, D, A, X[13], 21, UINT32_C (0x4e0811a1));

  MD5STEP_R4 (A, B, C, D, X[4], 6, UINT32_C (0xf7537e82));
  MD5STEP_R4 (D, A, B, C, X[11], 10, UINT32_C (0xbd3af235));
  MD5STEP_R4 (C, D, A, B, X[2], 15, UINT32_C (0x2ad7d2bb));
  MD5STEP_R4 (B, C, D, A, X[9], 21, UINT32_C (0xeb86d391));

  /* Finally increment and store working variables.
     See RFC 1321, end of Clause 3.4 (step 4). */

  H[0] += A;
  H[1] += B;
  H[2] += C;
  H[3] += D;
}


/**
 * Process portion of bytes.
 *
 * @param ctx the calculation context
 * @param data bytes to add to hash
 * @param length number of bytes in @a data
 */
void
MHD_MD5_update (struct Md5Ctx *ctx,
                const uint8_t *data,
                size_t length)
{
  unsigned int bytes_have; /**< Number of bytes in the context buffer */

  mhd_assert ((data != NULL) || (length == 0));

#ifndef MHD_FAVOR_SMALL_CODE
  if (0 == length)
    return; /* Shortcut, do nothing */
#endif /* MHD_FAVOR_SMALL_CODE */

  /* Note: (count & (MD5_BLOCK_SIZE-1))
           equals (count % MD5_BLOCK_SIZE) for this block size. */
  bytes_have = (unsigned int) (ctx->count & (MD5_BLOCK_SIZE - 1));
  ctx->count += length;

  if (0 != bytes_have)
  {
    unsigned int bytes_left = MD5_BLOCK_SIZE - bytes_have;
    if (length >= bytes_left)
    {     /* Combine new data with data in the buffer and
             process the full block. */
      memcpy (((uint8_t *) ctx->buffer) + bytes_have,
              data,
              bytes_left);
      data += bytes_left;
      length -= bytes_left;
      md5_transform (ctx->H, ctx->buffer);
      bytes_have = 0;
    }
  }

  while (MD5_BLOCK_SIZE <= length)
  {   /* Process any full blocks of new data directly,
         without copying to the buffer. */
    md5_transform (ctx->H, data);
    data += MD5_BLOCK_SIZE;
    length -= MD5_BLOCK_SIZE;
  }

  if (0 != length)
  {   /* Copy incomplete block of new data (if any)
         to the buffer. */
    memcpy (((uint8_t *) ctx->buffer) + bytes_have, data, length);
  }
}


/**
 * Size of "length" insertion in bits.
 * See RFC 1321, end of Clause 3.2 (step 2).
 */
#define MD5_SIZE_OF_LEN_ADD_BITS 64

/**
 * Size of "length" insertion in bytes.
 */
#define MD5_SIZE_OF_LEN_ADD (MD5_SIZE_OF_LEN_ADD_BITS / 8)

/**
 * Finalise MD5 calculation, return digest.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #MD5_DIGEST_SIZE bytes
 */
void
MHD_MD5_finish (struct Md5Ctx *ctx,
                uint8_t digest[MD5_DIGEST_SIZE])
{
  uint64_t num_bits;   /**< Number of processed bits */
  unsigned int bytes_have; /**< Number of bytes in the context buffer */

  /* Memorise the number of processed bits.
     The padding and other data added here during the postprocessing must
     not change the amount of hashed data. */
  num_bits = ctx->count << 3;

  /* Note: (count & (MD5_BLOCK_SIZE-1))
           equals (count % MD5_BLOCK_SIZE) for this block size. */
  bytes_have = (unsigned int) (ctx->count & (MD5_BLOCK_SIZE - 1));

  /* Input data must be padded with a single bit "1", then with zeros and
     the finally the length of data in bits must be added as the final bytes
     of the last block.
     See RFC 1321, Clauses 3.1 and 3.2 (steps 1 and 2). */
  /* Data is always processed in form of bytes (not by individual bits),
     therefore position of the first padding bit in byte is always
     predefined (0x80). */
  /* Buffer always have space for one byte at least (as full buffers are
     processed immediately). */
  ((uint8_t *) ctx->buffer)[bytes_have++] = 0x80;

  if (MD5_BLOCK_SIZE - bytes_have < MD5_SIZE_OF_LEN_ADD)
  {   /* No space in the current block to put the total length of message.
         Pad the current block with zeros and process it. */
    if (bytes_have < MD5_BLOCK_SIZE)
      memset (((uint8_t *) ctx->buffer) + bytes_have, 0,
              MD5_BLOCK_SIZE - bytes_have);
    /* Process the full block. */
    md5_transform (ctx->H, ctx->buffer);
    /* Start the new block. */
    bytes_have = 0;
  }

  /* Pad the rest of the buffer with zeros. */
  memset (((uint8_t *) ctx->buffer) + bytes_have, 0,
          MD5_BLOCK_SIZE - MD5_SIZE_OF_LEN_ADD - bytes_have);
  /* Put the number of bits in processed data as little-endian value.
     See RFC 1321, clauses 2 and 3.2 (step 2). */
  _MHD_PUT_64BIT_LE_SAFE (ctx->buffer + MD5_BLOCK_SIZE_WORDS - 2,
                          num_bits);
  /* Process the full final block. */
  md5_transform (ctx->H, ctx->buffer);

  /* Put in LE mode the hash as the final digest.
     See RFC 1321, clauses 2 and 3.5 (step 5). */
#ifndef _MHD_PUT_32BIT_LE_UNALIGNED
  if (0 != ((uintptr_t) digest) % _MHD_UINT32_ALIGN)
  { /* The destination is unaligned */
    uint32_t alig_dgst[MD5_DIGEST_SIZE_WORDS];
    _MHD_PUT_32BIT_LE (alig_dgst + 0, ctx->H[0]);
    _MHD_PUT_32BIT_LE (alig_dgst + 1, ctx->H[1]);
    _MHD_PUT_32BIT_LE (alig_dgst + 2, ctx->H[2]);
    _MHD_PUT_32BIT_LE (alig_dgst + 3, ctx->H[3]);
    /* Copy result to the unaligned destination address. */
    memcpy (digest, alig_dgst, MD5_DIGEST_SIZE);
  }
  else /* Combined with the next 'if' */
#endif /* ! _MHD_PUT_32BIT_LE_UNALIGNED */
  if (1)
  {
    /* Use cast to (void*) here to mute compiler alignment warnings.
     * Compilers are not smart enough to see that alignment has been checked. */
    _MHD_PUT_32BIT_LE ((void *) (digest + 0 * MD5_BYTES_IN_WORD), ctx->H[0]);
    _MHD_PUT_32BIT_LE ((void *) (digest + 1 * MD5_BYTES_IN_WORD), ctx->H[1]);
    _MHD_PUT_32BIT_LE ((void *) (digest + 2 * MD5_BYTES_IN_WORD), ctx->H[2]);
    _MHD_PUT_32BIT_LE ((void *) (digest + 3 * MD5_BYTES_IN_WORD), ctx->H[3]);
  }

  /* Erase potentially sensitive data. */
  memset (ctx, 0, sizeof(struct Md5Ctx));
}
