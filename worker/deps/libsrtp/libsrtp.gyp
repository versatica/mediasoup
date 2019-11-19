# NOTE: Inspired by https://chromium.googlesource.com/chromium/deps/libsrtp/+/master/libsrtp.gyp
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'use_openssl%': 1,
  },
  'target_defaults': {
    'defines': [
      'HAVE_CONFIG_H',
      'HAVE_STDLIB_H',
      'HAVE_STRING_H',
      'TESTAPP_SOURCE',
    ],
    'include_dirs': [
      './config',
      'srtp/include',
      'srtp/crypto/include',
    ],
    'conditions': [
      ['use_openssl==1', {
        'defines': [
          'OPENSSL',
        ],
      }],
      # NOTE: Avoid having to set os_posix
      # ['os_posix==1', {
      ['OS!="win"', {
        'defines': [
          'HAVE_INT16_T',
          'HAVE_INT32_T',
          'HAVE_INT8_T',
          'HAVE_UINT16_T',
          'HAVE_UINT32_T',
          'HAVE_UINT64_T',
          'HAVE_UINT8_T',
          'HAVE_STDINT_H',
          'HAVE_INTTYPES_H',
          'HAVE_NETINET_IN_H',
          'HAVE_ARPA_INET_H',
          'HAVE_UNISTD_H',
        ],
        'cflags': [
          '-Wno-unused-variable',
        ],
      }],
      ['OS=="win"', {
        'defines': [
          'HAVE_BYTESWAP_METHODS_H',
          # All Windows architectures are this way.
          'SIZEOF_UNSIGNED_LONG=4',
          'SIZEOF_UNSIGNED_LONG_LONG=8',
          'HAVE_WINSOCK2_H',
         ],
      }],
      ['target_arch=="x64" or target_arch=="ia32"', {
        'defines': [
          'CPU_CISC',
        ],
      }],
      ['target_arch=="arm" or target_arch=="arm64" \
       or target_arch=="mipsel" or target_arch=="mips64el"', {
        'defines': [
          # TODO(leozwang): CPU_RISC doesn't work properly on android/arm and
          # mips platforms for unknown reasons, need to investigate the root
          # cause of it. CPU_RISC is used for optimization only, and CPU_CISC
          # should just work just fine, it has been tested on android/arm with
          # srtp test applications and libjingle.
          'CPU_CISC',
        ],
      }],
      ['target_arch=="mipsel" or target_arch=="arm" or target_arch=="ia32"', {
        'defines': [
          # Define FORCE_64BIT_ALIGN to avoid alignment-related-crashes like
          # crbug/414919. Without this, aes_cbc_alloc will allocate an
          # aes_cbc_ctx_t not 64-bit aligned and the v128_t members of
          # aes_cbc_ctx_t will not be 64-bit aligned, which breaks the
          # compiler optimizations that assume 64-bit alignment of v128_t.
          'FORCE_64BIT_ALIGN',
        ],
      }],
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        # NOTE: I don't think we must export this.
        # './config',
        'srtp/include',
        # NOTE: I don't think we must export this.
        # 'srtp/crypto/include',
      ],
      'conditions': [
        # NOTE: Avoid having to set os_posix
        # ['os_posix==1', {
        ['OS!="win"', {
          'defines': [
            'HAVE_INT16_T',
            'HAVE_INT32_T',
            'HAVE_INT8_T',
            'HAVE_UINT16_T',
            'HAVE_UINT32_T',
            'HAVE_UINT64_T',
            'HAVE_UINT8_T',
            'HAVE_STDINT_H',
            'HAVE_INTTYPES_H',
            'HAVE_NETINET_IN_H',
          ],
        }],
        ['OS=="win"', {
          'defines': [
            'HAVE_BYTESWAP_METHODS_H',
            # All Windows architectures are this way.
            'SIZEOF_UNSIGNED_LONG=4',
            'SIZEOF_UNSIGNED_LONG_LONG=8',
           ],
        }],
        ['target_arch=="x64" or target_arch=="ia32"', {
          'defines': [
            'CPU_CISC',
          ],
        }],
      ],
    },
  },
  'targets': [
    {
      'target_name': 'libsrtp',
      'type': 'static_library',
      'sources': [
        # includes
        'srtp/include/ekt.h',
        'srtp/include/getopt_s.h',
        'srtp/include/rtp.h',
        'srtp/include/rtp_priv.h',
        'srtp/include/srtp.h',
        'srtp/include/srtp_priv.h',
        'srtp/include/ut_sim.h',
        # headers
        'srtp/crypto/include/aes.h',
        'srtp/crypto/include/aes_icm.h',
        'srtp/crypto/include/alloc.h',
        'srtp/crypto/include/auth.h',
        'srtp/crypto/include/cipher.h',
        'srtp/crypto/include/crypto_kernel.h',
        'srtp/crypto/include/crypto_math.h',
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
        # sources
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
        ['use_openssl==1', {
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
            'srtp/crypto/include/aes_gcm_ossl.h',
            'srtp/crypto/include/aes_icm_ossl.h',
          ],
        }],
      ],
    }, # target libsrtp
    {
      'target_name': 'rdbx_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/include/getopt_s.h',
        'srtp/test/getopt_s.c',
        'srtp/test/rdbx_driver.c',
      ],
    },
    {
      'target_name': 'srtp_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/include/getopt_s.h',
        'srtp/include/srtp_priv.h',
        'srtp/test/getopt_s.c',
        'srtp/test/srtp_driver.c',
      ],
    },
    {
      'target_name': 'roc_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/include/rdbx.h',
        'srtp/include/ut_sim.h',
        'srtp/test/roc_driver.c',
      ],
    },
    {
      'target_name': 'replay_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/include/rdbx.h',
        'srtp/include/ut_sim.h',
        'srtp/test/replay_driver.c',
      ],
    },
    {
      'target_name': 'rtpw',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/include/getopt_s.h',
        'srtp/include/rtp.h',
        'srtp/include/srtp.h',
        'srtp/crypto/include/datatypes.h',
        'srtp/test/getopt_s.c',
        'srtp/test/rtp.c',
        'srtp/test/rtpw.c',
      ],
      'conditions': [
        ['OS=="android"', {
          'defines': [
            'HAVE_SYS_SOCKET_H',
          ],
        }],
      ],
    },
    {
      'target_name': 'srtp_test_cipher_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/cipher_driver.c',
        'srtp/include/getopt_s.h',
        'srtp/test/getopt_s.c',
      ],
      'conditions': [
        ['use_openssl==1', {
          'dependencies': [
            '<(DEPTH)/deps/openssl/openssl.gyp:openssl',
          ],
        }],
      ],
    },
    {
      'target_name': 'srtp_test_datatypes_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/datatypes_driver.c',
      ],
    },
    {
      'target_name': 'srtp_test_stat_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/stat_driver.c',
      ],
    },
    {
      'target_name': 'srtp_test_sha1_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/sha1_driver.c',
      ],
    },
    {
      'target_name': 'srtp_test_kernel_driver',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/kernel_driver.c',
        'srtp/include/getopt_s.h',
        'srtp/test/getopt_s.c',
      ],
    },
    {
      'target_name': 'srtp_test_aes_calc',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/aes_calc.c',
      ],
    },
    {
      'target_name': 'srtp_test_rand_gen',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/rand_gen.c',
        'srtp/include/getopt_s.h',
        'srtp/test/getopt_s.c',
      ],
    },
    {
      'target_name': 'srtp_test_rand_gen_soak',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/rand_gen_soak.c',
        'srtp/include/getopt_s.h',
        'srtp/test/getopt_s.c',
      ],
    },
    {
      'target_name': 'srtp_test_env',
      'type': 'executable',
      'dependencies': [
        'libsrtp',
      ],
      'sources': [
        'srtp/crypto/test/env.c',
      ],
    },
    {
      'target_name': 'srtp_runtest',
      'type': 'none',
      'dependencies': [
        'rdbx_driver',
        'srtp_driver',
        'roc_driver',
        'replay_driver',
        'rtpw',
        'srtp_test_cipher_driver',
        'srtp_test_datatypes_driver',
        'srtp_test_stat_driver',
        'srtp_test_sha1_driver',
        'srtp_test_kernel_driver',
        'srtp_test_aes_calc',
        'srtp_test_rand_gen',
        'srtp_test_rand_gen_soak',
        'srtp_test_env',
      ],
    },
  ], # targets
}
