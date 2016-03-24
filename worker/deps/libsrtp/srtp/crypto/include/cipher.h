/*
 * cipher.h
 *
 * common interface to ciphers
 *
 * David A. McGrew
 * Cisco Systems, Inc.
 */
/*
 *
 * Copyright (c) 2001-2006,2013 Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 *
 *   Neither the name of the Cisco Systems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef CIPHER_H
#define CIPHER_H

#include "srtp.h"
#include "datatypes.h"
#include "rdbx.h"               /* for srtp_xtd_seq_num_t */
#include "err.h"                /* for error codes  */
#include "crypto_types.h"       /* for values of cipher_type_id_t */


/*
 * cipher_direction_t defines a particular cipher operation.
 *
 * A cipher_direction_t is an enum that describes a particular cipher
 * operation, i.e. encryption or decryption.  For some ciphers, this
 * distinction does not matter, but for others, it is essential.
 */
typedef enum {
    direction_encrypt, /**< encryption (convert plaintext to ciphertext) */
    direction_decrypt, /**< decryption (convert ciphertext to plaintext) */
    direction_any      /**< encryption or decryption                     */
} srtp_cipher_direction_t;

/*
 * the cipher_pointer and cipher_type_pointer definitions are needed
 * as cipher_t and cipher_type_t are not yet defined
 */
typedef struct srtp_cipher_t      *srtp_cipher_pointer_t;

/*
 *  a cipher_alloc_func_t allocates (but does not initialize) a cipher_t
 */
typedef srtp_err_status_t (*cipher_alloc_func_t)
    (srtp_cipher_pointer_t *cp, int key_len, int tag_len);

/*
 * a cipher_init_func_t [re-]initializes a cipher_t with a given key
 */
typedef srtp_err_status_t (*cipher_init_func_t)
    (void *state, const uint8_t *key);

/* a cipher_dealloc_func_t de-allocates a cipher_t */
typedef srtp_err_status_t (*cipher_dealloc_func_t)(srtp_cipher_pointer_t cp);

/* a cipher_set_segment_func_t sets the segment index of a cipher_t */
typedef srtp_err_status_t (*cipher_set_segment_func_t)
    (void *state, srtp_xtd_seq_num_t idx);

/*
 * a cipher_set_aad_func_t processes the AAD data for AEAD ciphers
 */
typedef srtp_err_status_t (*cipher_set_aad_func_t)
    (void *state, const uint8_t *aad, uint32_t aad_len);


/* a cipher_encrypt_func_t encrypts data in-place */
typedef srtp_err_status_t (*cipher_encrypt_func_t)
    (void *state, uint8_t *buffer, unsigned int *octets_to_encrypt);

/* a cipher_decrypt_func_t decrypts data in-place */
typedef srtp_err_status_t (*cipher_decrypt_func_t)
    (void *state, uint8_t *buffer, unsigned int *octets_to_decrypt);

/*
 * a cipher_set_iv_func_t function sets the current initialization vector
 */
typedef srtp_err_status_t (*cipher_set_iv_func_t)
    (srtp_cipher_pointer_t cp, uint8_t *iv, srtp_cipher_direction_t direction);

/*
 * a cipher_get_tag_funct_t function is used to get the authentication
 * tag that was calculated by an AEAD cipher.
 */
typedef srtp_err_status_t (*cipher_get_tag_func_t)
    (void *state, uint8_t *tag, uint32_t *len);


/*
 * cipher_test_case_t is a (list of) key, salt, srtp_xtd_seq_num_t,
 * plaintext, and ciphertext values that are known to be correct for a
 * particular cipher.  this data can be used to test an implementation
 * in an on-the-fly self test of the correcness of the implementation.
 * (see the cipher_type_self_test() function below)
 */
typedef struct srtp_cipher_test_case_t {
    int key_length_octets;                          /* octets in key            */
    const uint8_t *key;                                   /* key                      */
    uint8_t *idx;                                   /* packet index             */
    int plaintext_length_octets;                    /* octets in plaintext      */
    const uint8_t *plaintext;                             /* plaintext                */
    int ciphertext_length_octets;                   /* octets in plaintext      */
    const uint8_t *ciphertext;                            /* ciphertext               */
    int aad_length_octets;                          /* octets in AAD            */
    const uint8_t *aad;                                   /* AAD                      */
    int tag_length_octets;                          /* Length of AEAD tag       */
    const struct srtp_cipher_test_case_t *next_test_case; /* pointer to next testcase */
} srtp_cipher_test_case_t;

/* cipher_type_t defines the 'metadata' for a particular cipher type */
typedef struct srtp_cipher_type_t {
    cipher_alloc_func_t alloc;
    cipher_dealloc_func_t dealloc;
    cipher_init_func_t init;
    cipher_set_aad_func_t set_aad;
    cipher_encrypt_func_t encrypt;
    cipher_encrypt_func_t decrypt;
    cipher_set_iv_func_t set_iv;
    cipher_get_tag_func_t get_tag;
    const char                       *description;
    const srtp_cipher_test_case_t         *test_data;
    srtp_debug_module_t             *debug;
    srtp_cipher_type_id_t id;
} srtp_cipher_type_t;

/*
 * cipher_t defines an instantiation of a particular cipher, with fixed
 * key length, key and salt values
 */
typedef struct srtp_cipher_t {
    const srtp_cipher_type_t *type;
    void          *state;
    int key_len;
    int algorithm;
} srtp_cipher_t;

/* some bookkeeping functions */
int srtp_cipher_get_key_length(const srtp_cipher_t *c);


/*
 * cipher_type_self_test() tests a cipher against test cases provided in
 * an array of values of key/srtp_xtd_seq_num_t/plaintext/ciphertext
 * that is known to be good
 */
srtp_err_status_t srtp_cipher_type_self_test(const srtp_cipher_type_t *ct);


/*
 * cipher_type_test() tests a cipher against external test cases provided in
 * an array of values of key/srtp_xtd_seq_num_t/plaintext/ciphertext
 * that is known to be good
 */
srtp_err_status_t srtp_cipher_type_test(const srtp_cipher_type_t *ct, const srtp_cipher_test_case_t *test_data);


/*
 * cipher_bits_per_second(c, l, t) computes (and estimate of) the
 * number of bits that a cipher implementation can encrypt in a second
 *
 * c is a cipher (which MUST be allocated and initialized already), l
 * is the length in octets of the test data to be encrypted, and t is
 * the number of trials
 *
 * if an error is encountered, then the value 0 is returned
 */
uint64_t srtp_cipher_bits_per_second(srtp_cipher_t *c, int octets_in_buffer, int num_trials);

srtp_err_status_t srtp_cipher_type_alloc(const srtp_cipher_type_t *ct, srtp_cipher_t **c, int key_len, int tlen);
srtp_err_status_t srtp_cipher_dealloc(srtp_cipher_t *c);
srtp_err_status_t srtp_cipher_init(srtp_cipher_t *c, const uint8_t *key);
srtp_err_status_t srtp_cipher_set_iv(srtp_cipher_t *c, uint8_t *iv, int direction);
srtp_err_status_t srtp_cipher_output(srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output); 
srtp_err_status_t srtp_cipher_encrypt(srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output); 
srtp_err_status_t srtp_cipher_decrypt(srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output); 
srtp_err_status_t srtp_cipher_get_tag(srtp_cipher_t *c, uint8_t *buffer, uint32_t *tag_len);
srtp_err_status_t srtp_cipher_set_aad(srtp_cipher_t *c, const uint8_t *aad, uint32_t aad_len);

#endif /* CIPHER_H */
