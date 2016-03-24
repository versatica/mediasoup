/*
 * null_cipher.c
 *
 * A null cipher implementation.  This cipher leaves the plaintext
 * unchanged.
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

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "datatypes.h"
#include "null_cipher.h"
#include "alloc.h"

/* the null_cipher uses the cipher debug module  */

extern srtp_debug_module_t srtp_mod_cipher;

static srtp_err_status_t srtp_null_cipher_alloc (srtp_cipher_t **c, int key_len, int tlen)
{
    extern const srtp_cipher_type_t srtp_null_cipher;

    debug_print(srtp_mod_cipher,
                "allocating cipher with key length %d", key_len);

    /* allocate memory a cipher of type null_cipher */
    *c = (srtp_cipher_t *)srtp_crypto_alloc(sizeof(srtp_cipher_t));
    if (*c == NULL) {
        return srtp_err_status_alloc_fail;
    }
    memset(*c, 0x0, sizeof(srtp_cipher_t));

    /* set pointers */
    (*c)->algorithm = SRTP_NULL_CIPHER;
    (*c)->type = &srtp_null_cipher;
    (*c)->state = (void *) 0x1; /* The null cipher does not maintain state */

    /* set key size */
    (*c)->key_len = key_len;

    return srtp_err_status_ok;

}

static srtp_err_status_t srtp_null_cipher_dealloc (srtp_cipher_t *c)
{
    extern const srtp_cipher_type_t srtp_null_cipher;

    /* zeroize entire state*/
    octet_string_set_to_zero((uint8_t*)c, sizeof(srtp_cipher_t));

    /* free memory of type null_cipher */
    srtp_crypto_free(c);

    return srtp_err_status_ok;

}

static srtp_err_status_t srtp_null_cipher_init (srtp_null_cipher_ctx_t *ctx, const uint8_t *key)
{

    debug_print(srtp_mod_cipher, "initializing null cipher", NULL);

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_null_cipher_set_iv (srtp_null_cipher_ctx_t *c, uint8_t *iv, int dir)
{
    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_null_cipher_encrypt (srtp_null_cipher_ctx_t *c,
                                            unsigned char *buf, unsigned int *bytes_to_encr)
{
    return srtp_err_status_ok;
}

static const char srtp_null_cipher_description[] = "null cipher";

static const srtp_cipher_test_case_t srtp_null_cipher_test_0 = {
    0,    /* octets in key            */
    NULL, /* key                      */
    0,    /* packet index             */
    0,    /* octets in plaintext      */
    NULL, /* plaintext                */
    0,    /* octets in plaintext      */
    NULL, /* ciphertext               */
    0,
    NULL,
    0,
    NULL             /* pointer to next testcase */
};


/*
 * note: the decrypt function is idential to the encrypt function
 */

const srtp_cipher_type_t srtp_null_cipher = {
    (cipher_alloc_func_t)srtp_null_cipher_alloc,
    (cipher_dealloc_func_t)srtp_null_cipher_dealloc,
    (cipher_init_func_t)srtp_null_cipher_init,
    (cipher_set_aad_func_t)0,
    (cipher_encrypt_func_t)srtp_null_cipher_encrypt,
    (cipher_decrypt_func_t)srtp_null_cipher_encrypt,
    (cipher_set_iv_func_t)srtp_null_cipher_set_iv,
    (cipher_get_tag_func_t)0,
    (const char*)srtp_null_cipher_description,
    (const srtp_cipher_test_case_t*)&srtp_null_cipher_test_0,
    (srtp_debug_module_t*)NULL,
    (srtp_cipher_type_id_t)SRTP_NULL_CIPHER
};

