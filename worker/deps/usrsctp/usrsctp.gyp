# NOTE: Inspired by https://chromium.googlesource.com/chromium/src/+/master/third_party/usrsctp/BUILD.gn
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'defines': [
      'SCTP_PROCESS_LEVEL_LOCKS',
      'SCTP_SIMPLE_ALLOCATOR',
      'SCTP_USE_OPENSSL_SHA1',
      '__Userspace__',
      #SCTP_DEBUG', # Uncomment for SCTP debugging.
    ],
    'direct_dependent_settings': {
      'include_dirs': [
      'usrsctplib',
      'usrsctplib/netinet',
      ]
    },
    'include_dirs': [
      'usrsctplib',
      'usrsctplib/netinet',
    ],
    'cflags': [
      '-UINET',
      '-UINET6',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': [
        '-UINET',
        '-UINET6',
      ]
    },
    'conditions': [
      ['OS in "linux android"', {
        'defines': [
          '__Userspace_os_Linux',
          '_GNU_SOURCE',
        ]
      }],
      ['OS in "mac ios"', {
        'defines': [
          'HAVE_SA_LEN',
          'HAVE_SCONN_LEN',
          '__APPLE_USE_RFC_2292',
          '__Userspace_os_Darwin',
        ],
        'xcode_settings': {
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wextra',
            '-Wno-unused-parameter',
            '-Wstrict-prototypes',
          ],
          'OTHER_CFLAGS': [
            '-U__APPLE__',
            '-Wno-deprecated-declarations',
            # atomic_init in user_atomic.h is a static function in a header.
            '-Wno-unused-function',
          ]
        }
      }],
      ['OS=="win"', {
        'defines': [
          '__Userspace_os_Windows'
        ]
      }],
      ['OS!="win"', {
        'defines': [
          'NON_WINDOWS_DEFINE'
        ]
      }]
    ]
  },
  'targets': [
    {
      'target_name': 'usrsctp',
      'type': 'static_library',
      'sources': [
        'usrsctplib/netinet/sctp.h',
        'usrsctplib/netinet/sctp_asconf.c',
        'usrsctplib/netinet/sctp_asconf.h',
        'usrsctplib/netinet/sctp_auth.c',
        'usrsctplib/netinet/sctp_auth.h',
        'usrsctplib/netinet/sctp_bsd_addr.c',
        'usrsctplib/netinet/sctp_bsd_addr.h',
        'usrsctplib/netinet/sctp_callout.c',
        'usrsctplib/netinet/sctp_callout.h',
        'usrsctplib/netinet/sctp_cc_functions.c',
        'usrsctplib/netinet/sctp_constants.h',
        'usrsctplib/netinet/sctp_crc32.c',
        'usrsctplib/netinet/sctp_crc32.h',
        'usrsctplib/netinet/sctp_header.h',
        'usrsctplib/netinet/sctp_indata.c',
        'usrsctplib/netinet/sctp_indata.h',
        'usrsctplib/netinet/sctp_input.c',
        'usrsctplib/netinet/sctp_input.h',
        'usrsctplib/netinet/sctp_lock_userspace.h',
        'usrsctplib/netinet/sctp_os.h',
        'usrsctplib/netinet/sctp_os_userspace.h',
        'usrsctplib/netinet/sctp_output.c',
        'usrsctplib/netinet/sctp_output.h',
        'usrsctplib/netinet/sctp_pcb.c',
        'usrsctplib/netinet/sctp_pcb.h',
        'usrsctplib/netinet/sctp_peeloff.c',
        'usrsctplib/netinet/sctp_peeloff.h',
        'usrsctplib/netinet/sctp_process_lock.h',
        'usrsctplib/netinet/sctp_sha1.c',
        'usrsctplib/netinet/sctp_sha1.h',
        'usrsctplib/netinet/sctp_ss_functions.c',
        'usrsctplib/netinet/sctp_structs.h',
        'usrsctplib/netinet/sctp_sysctl.c',
        'usrsctplib/netinet/sctp_sysctl.h',
        'usrsctplib/netinet/sctp_timer.c',
        'usrsctplib/netinet/sctp_timer.h',
        'usrsctplib/netinet/sctp_uio.h',
        'usrsctplib/netinet/sctp_userspace.c',
        'usrsctplib/netinet/sctp_usrreq.c',
        'usrsctplib/netinet/sctp_var.h',
        'usrsctplib/netinet/sctputil.c',
        'usrsctplib/netinet/sctputil.h',
        'usrsctplib/netinet6/sctp6_usrreq.c',
        'usrsctplib/netinet6/sctp6_var.h',
        'usrsctplib/user_atomic.h',
        'usrsctplib/user_environment.c',
        'usrsctplib/user_environment.h',
        'usrsctplib/user_inpcb.h',
        'usrsctplib/user_ip6_var.h',
        'usrsctplib/user_ip_icmp.h',
        'usrsctplib/user_malloc.h',
        'usrsctplib/user_mbuf.c',
        'usrsctplib/user_mbuf.h',
        'usrsctplib/user_queue.h',
        'usrsctplib/user_recv_thread.c',
        'usrsctplib/user_recv_thread.h',
        'usrsctplib/user_route.h',
        'usrsctplib/user_socket.c',
        'usrsctplib/user_socketvar.h',
        'usrsctplib/user_uma.h',
        'usrsctplib/usrsctp.h',
      ],
    },
  ], # targets
}
