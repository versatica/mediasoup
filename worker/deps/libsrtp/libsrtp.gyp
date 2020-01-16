# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'use_openssl%': 1,
    'use_nss%': 0, # Not used here.
  },
  'target_defaults': {
    'defines': [
      # Config.
      'HAVE_CONFIG_H',
      # Platform properties.
      'HAVE_STDLIB_H',
      'HAVE_STRING_H',
      'HAVE_STDINT_H',
      'HAVE_INTTYPES_H',
      'HAVE_INT8_T',
      'HAVE_INT16_T',
      'HAVE_INT32_T',
      'HAVE_UINT8_T',
      'HAVE_UINT16_T',
      'HAVE_UINT32_T',
      'HAVE_UINT64_T',
    ],
    'conditions': [
      [ 'use_openssl == 1', {
        'defines': [
          'OPENSSL',
          'GCM',
        ],
      }],
      [ 'OS != "win"', {
        'defines': [
          'HAVE_ARPA_INET_H',
          'HAVE_NETINET_IN_H',
          'HAVE_SYS_TYPES_H',
          'HAVE_UNISTD_H',
        ],
        'cflags': [ '-Wall', '-Wno-unused-variable', '-Wno-sign-compare' ],
      }],
      [ 'OS == "mac"', {
        'xcode_settings':
        {
          'WARNING_CFLAGS': [ '-Wall', '-Wno-unused-variable', '-Wno-sign-compare' ],
        }
      }],
      [ 'OS == "win"', {
        'defines': [
          'HAVE_WINSOCK2_H',
         ],
      }],
      [ 'target_arch == "x64" or target_arch == "ia32"', {
        'defines': [
          'CPU_CISC',
        ],
      }],
      [ 'target_arch == "arm" or target_arch == "arm64" \
        or target_arch == "mipsel" or target_arch == "mips64el"', {
        'defines': [
          # TODO(leozwang): CPU_RISC doesn't work properly on android/arm and
          # mips platforms for unknown reasons, need to investigate the root
          # cause of it. CPU_RISC is used for optimization only, and CPU_CISC
          # should just work just fine, it has been tested on android/arm with
          # srtp test applications and libjingle.
          'CPU_CISC',
        ],
      }],
    ],
    'include_dirs': [
      './config',
      'srtp/include',
      'srtp/crypto/include',
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        'srtp/include',
        # NOTE: I don't think we must export this.
        # 'srtp/crypto/include',
      ],
    },
  },
  'targets': [
    {
      'target_name': 'libsrtp',
      'type': 'static_library',
      'sources': [
        # Includes.
        'srtp/include/ekt.h',
        'srtp/include/srtp.h',
        # Headers.
        'srtp/include/srtp_priv.h',
        'srtp/include/ut_sim.h',
        'srtp/crypto/include/aes.h',
        'srtp/crypto/include/aes_gcm.h',
        'srtp/crypto/include/aes_icm.h',
        'srtp/crypto/include/aes_icm_ext.h',
        'srtp/crypto/include/alloc.h',
        'srtp/crypto/include/auth.h',
        'srtp/crypto/include/cipher.h',
        'srtp/crypto/include/cipher_priv.h',
        'srtp/crypto/include/cipher_types.h',
        'srtp/crypto/include/crypto_kernel.h',
        'srtp/crypto/include/crypto_types.h',
        'srtp/crypto/include/datatypes.h',
        'srtp/crypto/include/err.h',
        'srtp/crypto/include/hmac.h',
        'srtp/crypto/include/integers.h',
        'srtp/crypto/include/key.h',
        'srtp/crypto/include/null_auth.h',
        'srtp/crypto/include/null_cipher.h',
        'srtp/crypto/include/rdb.h',
        'srtp/crypto/include/rdbx.h',
        'srtp/crypto/include/sha1.h',
        'srtp/crypto/include/stat.h',
        # Sources.
        'srtp/srtp/ekt.c',
        'srtp/srtp/srtp.c',
        'srtp/crypto/cipher/aes.c',
        'srtp/crypto/cipher/aes_icm.c',
        'srtp/crypto/cipher/cipher.c',
        'srtp/crypto/cipher/null_cipher.c',
        'srtp/crypto/hash/auth.c',
        'srtp/crypto/hash/hmac.c',
        'srtp/crypto/hash/null_auth.c',
        'srtp/crypto/hash/sha1.c',
        'srtp/crypto/kernel/alloc.c',
        'srtp/crypto/kernel/crypto_kernel.c',
        'srtp/crypto/kernel/err.c',
        'srtp/crypto/kernel/key.c',
        'srtp/crypto/math/datatypes.c',
        'srtp/crypto/math/stat.c',
        'srtp/crypto/replay/rdb.c',
        'srtp/crypto/replay/rdbx.c',
        'srtp/crypto/replay/ut_sim.c',
      ],
      'conditions': [
        [ 'use_openssl == 1', {
          'dependencies': [
            '<(DEPTH)/deps/openssl/openssl.gyp:openssl',
          ],
          'sources!': [
            'srtp/crypto/cipher/aes_icm.c',
            'srtp/crypto/hash/hmac.c',
            'srtp/crypto/hash/sha1.c',
          ],
          'sources': [
            'srtp/crypto/cipher/aes_gcm_ossl.c',
            'srtp/crypto/cipher/aes_icm_ossl.c',
            'srtp/crypto/hash/hmac_ossl.c',
          ],
        }],
      ],
    },
  ], # targets
}
