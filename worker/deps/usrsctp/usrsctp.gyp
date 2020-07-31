# NOTE: Inspired by
#   https://chromium.googlesource.com/chromium/src/+/master/third_party/usrsctp/BUILD.gn
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'dependencies': [
      # We need our own openssl dependency (required if SCTP_USE_OPENSSL_SHA1
      # is defined).
      '../openssl/openssl.gyp:openssl',
    ],
    'defines': [
      'SCTP_PROCESS_LEVEL_LOCKS',
      'SCTP_SIMPLE_ALLOCATOR',
      'SCTP_USE_OPENSSL_SHA1',
      '__Userspace__',
      # 'SCTP_DEBUG', # Uncomment for SCTP debugging.
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        'usrsctp/usrsctplib'
      ],
    },
    'include_dirs': [
      'usrsctp/usrsctplib',
      # 'usrsctp/usrsctplib/netinet', # Not needed (it seems).
    ],
    'cflags': [
      '-UINET',
      '-UINET6',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': [
        '-UINET',
        '-UINET6',
      ],
    },
    'conditions': [
      ['OS in "linux android"', {
        'defines': [
          '_GNU_SOURCE',
        ],
      }],
      ['OS in "mac ios"', {
        'defines': [
          'HAVE_SA_LEN',
          'HAVE_SCONN_LEN',
          '__APPLE_USE_RFC_2292',
        ],
        'xcode_settings': {
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wextra',
            '-Wno-unused-parameter',
            '-Wstrict-prototypes',
          ],
          'OTHER_CFLAGS': [
            '-Wno-deprecated-declarations',
            # atomic_init in user_atomic.h is a static function in a header.
            '-Wno-unused-function',
          ],
        }
      }],
      ['OS!="win"', {
        'defines': [
          'NON_WINDOWS_DEFINE'
        ],
      }],
      [ 'sctp_debug == "true"', {
        'defines': [ 'SCTP_DEBUG' ]
      }]
    ],
  },
  'targets': [
    {
      'target_name': 'usrsctp',
      'type': 'static_library',
      'sources': [
        'usrsctp/usrsctplib/netinet/sctp.h',
        'usrsctp/usrsctplib/netinet/sctp_asconf.c',
        'usrsctp/usrsctplib/netinet/sctp_asconf.h',
        'usrsctp/usrsctplib/netinet/sctp_auth.c',
        'usrsctp/usrsctplib/netinet/sctp_auth.h',
        'usrsctp/usrsctplib/netinet/sctp_bsd_addr.c',
        'usrsctp/usrsctplib/netinet/sctp_bsd_addr.h',
        'usrsctp/usrsctplib/netinet/sctp_callout.c',
        'usrsctp/usrsctplib/netinet/sctp_callout.h',
        'usrsctp/usrsctplib/netinet/sctp_cc_functions.c',
        'usrsctp/usrsctplib/netinet/sctp_constants.h',
        'usrsctp/usrsctplib/netinet/sctp_crc32.c',
        'usrsctp/usrsctplib/netinet/sctp_crc32.h',
        'usrsctp/usrsctplib/netinet/sctp_header.h',
        'usrsctp/usrsctplib/netinet/sctp_indata.c',
        'usrsctp/usrsctplib/netinet/sctp_indata.h',
        'usrsctp/usrsctplib/netinet/sctp_input.c',
        'usrsctp/usrsctplib/netinet/sctp_input.h',
        'usrsctp/usrsctplib/netinet/sctp_lock_userspace.h',
        'usrsctp/usrsctplib/netinet/sctp_os.h',
        'usrsctp/usrsctplib/netinet/sctp_os_userspace.h',
        'usrsctp/usrsctplib/netinet/sctp_output.c',
        'usrsctp/usrsctplib/netinet/sctp_output.h',
        'usrsctp/usrsctplib/netinet/sctp_pcb.c',
        'usrsctp/usrsctplib/netinet/sctp_pcb.h',
        'usrsctp/usrsctplib/netinet/sctp_peeloff.c',
        'usrsctp/usrsctplib/netinet/sctp_peeloff.h',
        'usrsctp/usrsctplib/netinet/sctp_process_lock.h',
        'usrsctp/usrsctplib/netinet/sctp_sha1.c',
        'usrsctp/usrsctplib/netinet/sctp_sha1.h',
        'usrsctp/usrsctplib/netinet/sctp_ss_functions.c',
        'usrsctp/usrsctplib/netinet/sctp_structs.h',
        'usrsctp/usrsctplib/netinet/sctp_sysctl.c',
        'usrsctp/usrsctplib/netinet/sctp_sysctl.h',
        'usrsctp/usrsctplib/netinet/sctp_timer.c',
        'usrsctp/usrsctplib/netinet/sctp_timer.h',
        'usrsctp/usrsctplib/netinet/sctp_uio.h',
        'usrsctp/usrsctplib/netinet/sctp_userspace.c',
        'usrsctp/usrsctplib/netinet/sctp_usrreq.c',
        'usrsctp/usrsctplib/netinet/sctp_var.h',
        'usrsctp/usrsctplib/netinet/sctputil.c',
        'usrsctp/usrsctplib/netinet/sctputil.h',
        'usrsctp/usrsctplib/netinet6/sctp6_usrreq.c',
        'usrsctp/usrsctplib/netinet6/sctp6_var.h',
        'usrsctp/usrsctplib/user_atomic.h',
        'usrsctp/usrsctplib/user_environment.c',
        'usrsctp/usrsctplib/user_environment.h',
        'usrsctp/usrsctplib/user_inpcb.h',
        'usrsctp/usrsctplib/user_ip6_var.h',
        'usrsctp/usrsctplib/user_ip_icmp.h',
        'usrsctp/usrsctplib/user_malloc.h',
        'usrsctp/usrsctplib/user_mbuf.c',
        'usrsctp/usrsctplib/user_mbuf.h',
        'usrsctp/usrsctplib/user_queue.h',
        'usrsctp/usrsctplib/user_recv_thread.c',
        'usrsctp/usrsctplib/user_recv_thread.h',
        'usrsctp/usrsctplib/user_route.h',
        'usrsctp/usrsctplib/user_socket.c',
        'usrsctp/usrsctplib/user_socketvar.h',
        'usrsctp/usrsctplib/user_uma.h',
        'usrsctp/usrsctplib/usrsctp.h',
      ],
    },
  ], # targets
}
