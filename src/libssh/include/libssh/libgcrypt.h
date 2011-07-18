/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef LIBGCRYPT_H_
#define LIBGCRYPT_H_

#include "libssh_config.h"

#ifdef HAVE_LIBGCRYPT

#include <gcrypt.h>
typedef gcry_md_hd_t SHACTX;
typedef gcry_md_hd_t MD5CTX;
typedef gcry_md_hd_t HMACCTX;
#define SHA_DIGEST_LEN 20
#define MD5_DIGEST_LEN 16
#define EVP_MAX_MD_SIZE 36

typedef gcry_mpi_t bignum;

#define bignum_new() gcry_mpi_new(0)
#define bignum_free(num) gcry_mpi_release(num)
#define bignum_set_word(bn,n) gcry_mpi_set_ui(bn,n)
#define bignum_bin2bn(bn,datalen,data) gcry_mpi_scan(data,GCRYMPI_FMT_USG,bn,datalen,NULL)
#define bignum_bn2dec(num) my_gcry_bn2dec(num)
#define bignum_dec2bn(num, data) my_gcry_dec2bn(data, num)
#define bignum_bn2hex(num,data) gcry_mpi_aprint(GCRYMPI_FMT_HEX,data,NULL,num)
#define bignum_hex2bn(num,datalen,data) gcry_mpi_scan(num,GCRYMPI_FMT_HEX,data,datalen,NULL)
#define bignum_rand(num,bits) gcry_mpi_randomize(num,bits,GCRY_STRONG_RANDOM),gcry_mpi_set_bit(num,bits-1),gcry_mpi_set_bit(num,0)
#define bignum_mod_exp(dest,generator,exp,modulo) gcry_mpi_powm(dest,generator,exp,modulo)
#define bignum_num_bits(num) gcry_mpi_get_nbits(num)
#define bignum_num_bytes(num) ((gcry_mpi_get_nbits(num)+7)/8)
#define bignum_is_bit_set(num,bit) gcry_mpi_test_bit(num,bit)
#define bignum_bn2bin(num,datalen,data) gcry_mpi_print(GCRYMPI_FMT_USG,data,datalen,NULL,num)
#define bignum_cmp(num1,num2) gcry_mpi_cmp(num1,num2)


#endif /* HAVE_LIBGCRYPT */

struct crypto_struct *ssh_get_ciphertab(void);

#endif /* LIBGCRYPT_H_ */
