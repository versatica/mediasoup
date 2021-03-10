{
  'variables': {
    'conditions': [
      ['OS=="win"', {
        'shared_unix_defines': [ ],
      }, {
        'shared_unix_defines': [
          '_LARGEFILE_SOURCE',
          '_FILE_OFFSET_BITS=64',
        ],
      }],
      ['OS in "mac ios"', {
        'shared_mac_defines': [ '_DARWIN_USE_64_BIT_INODE=1' ],
      }, {
        'shared_mac_defines': [ ],
      }],
      ['OS=="zos"', {
        'shared_zos_defines': [
          '_UNIX03_THREADS',
          '_UNIX03_SOURCE',
          '_UNIX03_WITHDRAWN',
          '_OPEN_SYS_IF_EXT',
          '_OPEN_SYS_SOCK_EXT3',
          '_OPEN_SYS_SOCK_IPV6',
          '_OPEN_MSGQ_EXT',
          '_XOPEN_SOURCE_EXTENDED',
          '_ALL_SOURCE',
          '_LARGE_TIME_API',
          '_OPEN_SYS_FILE_EXT',
          '_AE_BIMODAL',
          'PATH_MAX=255'
        ],
      }, {
        'shared_zos_defines': [ ],
      }],
    ],
  },

  'targets': [
    {
      'target_name': 'libuv',
      'type': '<(uv_library)',
      'include_dirs': [
        'libuv/include',
        'libuv/src/',
      ],
      'defines': [
        '<@(shared_mac_defines)',
        '<@(shared_unix_defines)',
        '<@(shared_zos_defines)',
      ],
      'direct_dependent_settings': {
        'defines': [
          '<@(shared_mac_defines)',
          '<@(shared_unix_defines)',
          '<@(shared_zos_defines)',
        ],
        'include_dirs': [ 'libuv/include' ],
        'conditions': [
          ['OS == "linux"', {
            'defines': [ '_POSIX_C_SOURCE=200112' ],
          }],
        ],
      },
      'sources': [
        'libuv/common.gypi',
        'libuv/include/uv.h',
        'libuv/include/uv/tree.h',
        'libuv/include/uv/errno.h',
        'libuv/include/uv/threadpool.h',
        'libuv/include/uv/version.h',
        'libuv/src/fs-poll.c',
        'libuv/src/heap-inl.h',
        'libuv/src/idna.c',
        'libuv/src/idna.h',
        'libuv/src/inet.c',
        'libuv/src/queue.h',
        'libuv/src/random.c',
        'libuv/src/strscpy.c',
        'libuv/src/strscpy.h',
        'libuv/src/threadpool.c',
        'libuv/src/timer.c',
        'libuv/src/uv-data-getter-setters.c',
        'libuv/src/uv-common.c',
        'libuv/src/uv-common.h',
        'libuv/src/version.c'
      ],
      'xcode_settings': {
        'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',  # -fvisibility=hidden
        'WARNING_CFLAGS': [
          '-Wall',
          '-Wextra',
          '-Wno-unused-parameter',
          '-Wstrict-prototypes',
        ],
        'OTHER_CFLAGS': [ '-g', '--std=gnu89' ],
      },
      'conditions': [
        [ 'OS=="win"', {
          'defines': [
            '_WIN32_WINNT=0x0600',
            '_GNU_SOURCE',
          ],
          'sources': [
            'libuv/include/uv/win.h',
            'libuv/src/win/async.c',
            'libuv/src/win/atomicops-inl.h',
            'libuv/src/win/core.c',
            'libuv/src/win/detect-wakeup.c',
            'libuv/src/win/dl.c',
            'libuv/src/win/error.c',
            'libuv/src/win/fs.c',
            'libuv/src/win/fs-event.c',
            'libuv/src/win/getaddrinfo.c',
            'libuv/src/win/getnameinfo.c',
            'libuv/src/win/handle.c',
            'libuv/src/win/handle-inl.h',
            'libuv/src/win/internal.h',
            'libuv/src/win/loop-watcher.c',
            'libuv/src/win/pipe.c',
            'libuv/src/win/thread.c',
            'libuv/src/win/poll.c',
            'libuv/src/win/process.c',
            'libuv/src/win/process-stdio.c',
            'libuv/src/win/req-inl.h',
            'libuv/src/win/signal.c',
            'libuv/src/win/snprintf.c',
            'libuv/src/win/stream.c',
            'libuv/src/win/stream-inl.h',
            'libuv/src/win/tcp.c',
            'libuv/src/win/tty.c',
            'libuv/src/win/udp.c',
            'libuv/src/win/util.c',
            'libuv/src/win/winapi.c',
            'libuv/src/win/winapi.h',
            'libuv/src/win/winsock.c',
            'libuv/src/win/winsock.h',
          ],
          'link_settings': {
            'libraries': [
              '-ladvapi32',
              '-liphlpapi',
              '-lpsapi',
              '-lshell32',
              '-luser32',
              '-luserenv',
              '-lws2_32'
            ],
          },
        }, { # Not Windows i.e. POSIX
          'sources': [
            'libuv/include/uv/unix.h',
            'libuv/include/uv/linux.h',
            'libuv/include/uv/sunos.h',
            'libuv/include/uv/darwin.h',
            'libuv/include/uv/bsd.h',
            'libuv/include/uv/aix.h',
            'libuv/src/unix/async.c',
            'libuv/src/unix/atomic-ops.h',
            'libuv/src/unix/core.c',
            'libuv/src/unix/dl.c',
            'libuv/src/unix/fs.c',
            'libuv/src/unix/getaddrinfo.c',
            'libuv/src/unix/getnameinfo.c',
            'libuv/src/unix/internal.h',
            'libuv/src/unix/loop.c',
            'libuv/src/unix/loop-watcher.c',
            'libuv/src/unix/pipe.c',
            'libuv/src/unix/poll.c',
            'libuv/src/unix/process.c',
            'libuv/src/unix/random-devurandom.c',
            'libuv/src/unix/signal.c',
            'libuv/src/unix/spinlock.h',
            'libuv/src/unix/stream.c',
            'libuv/src/unix/tcp.c',
            'libuv/src/unix/thread.c',
            'libuv/src/unix/tty.c',
            'libuv/src/unix/udp.c',
          ],
          'link_settings': {
            'libraries': [ '-lm' ],
            'conditions': [
              ['OS=="solaris"', {
                'ldflags': [ '-pthreads' ],
              }],
              [ 'OS=="zos" and uv_library=="shared_library"', {
                'ldflags': [ '-Wl,DLL' ],
              }],
              ['OS != "solaris" and OS != "android" and OS != "zos"', {
                'ldflags': [ '-pthread' ],
              }],
            ],
          },
          'conditions': [
            ['uv_library=="shared_library"', {
              'conditions': [
                ['OS=="zos"', {
                  'cflags': [ '-qexportall' ],
                }, {
                  'cflags': [ '-fPIC' ],
                }],
              ],
            }],
            ['uv_library=="shared_library" and OS!="mac" and OS!="zos"', {
              # This will cause gyp to set soname
              # Must correspond with UV_VERSION_MAJOR
              # in include/uv/version.h
              'product_extension': 'so.1',
            }],
          ],
        }],
        [ 'OS in "linux mac ios android zos"', {
          'sources': [ 'libuv/src/unix/proctitle.c' ],
        }],
        [ 'OS != "zos"', {
          'cflags': [
            '-fvisibility=hidden',
            '-g',
            '--std=gnu89',
            '-Wall',
            '-Wextra',
            '-Wno-unused-parameter',
            '-Wstrict-prototypes',
          ],
        }],
        [ 'OS in "mac ios"', {
          'sources': [
            'libuv/src/unix/darwin.c',
            'libuv/src/unix/fsevents.c',
            'libuv/src/unix/darwin-proctitle.c',
            'libuv/src/unix/random-getentropy.c',
          ],
          'defines': [
            '_DARWIN_USE_64_BIT_INODE=1',
            '_DARWIN_UNLIMITED_SELECT=1',
          ]
        }],
        [ 'OS=="linux"', {
          'defines': [ '_GNU_SOURCE' ],
          'sources': [
            'libuv/src/unix/linux-core.c',
            'libuv/src/unix/linux-inotify.c',
            'libuv/src/unix/linux-syscalls.c',
            'libuv/src/unix/linux-syscalls.h',
            'libuv/src/unix/procfs-exepath.c',
            'libuv/src/unix/random-getrandom.c',
            'libuv/src/unix/random-sysctl-linux.c',
          ],
          'link_settings': {
            'libraries': [ '-ldl', '-lrt' ],
          },
        }],
        [ 'OS=="android"', {
          'sources': [
            'libuv/src/unix/linux-core.c',
            'libuv/src/unix/linux-inotify.c',
            'libuv/src/unix/linux-syscalls.c',
            'libuv/src/unix/linux-syscalls.h',
            'libuv/src/unix/pthread-fixes.c',
            'libuv/src/unix/android-ifaddrs.c',
            'libuv/src/unix/procfs-exepath.c',
            'libuv/src/unix/random-getrandom.c',
            'libuv/src/unix/random-sysctl-linux.c',
          ],
          'link_settings': {
            'libraries': [ '-ldl' ],
          },
        }],
        [ 'OS=="solaris"', {
          'sources': [
            'libuv/src/unix/no-proctitle.c',
            'libuv/src/unix/sunos.c',
          ],
          'defines': [
            '__EXTENSIONS__',
            '_XOPEN_SOURCE=500',
          ],
          'link_settings': {
            'libraries': [
              '-lkstat',
              '-lnsl',
              '-lsendfile',
              '-lsocket',
            ],
          },
        }],
        [ 'OS=="aix"', {
          'variables': {
            'os_name': '<!(uname -s)',
          },
          'sources': [
            'libuv/src/unix/aix-common.c',
          ],
          'defines': [
            '_ALL_SOURCE',
            '_XOPEN_SOURCE=500',
            '_LINUX_SOURCE_COMPAT',
            '_THREAD_SAFE',
          ],
          'conditions': [
            [ '"<(os_name)"=="OS400"', {
              'sources': [
                'libuv/src/unix/ibmi.c',
                'libuv/src/unix/posix-poll.c',
                'libuv/src/unix/no-fsevents.c',
                'libuv/src/unix/no-proctitle.c',
              ],
            }, {
              'sources': [
                'libuv/src/unix/aix.c'
              ],
              'defines': [
                'HAVE_SYS_AHAFS_EVPRODS_H'
              ],
              'link_settings': {
                'libraries': [
                  '-lperfstat',
                ],
              },
            }],
          ]
        }],
        [ 'OS=="freebsd" or OS=="dragonflybsd"', {
          'sources': [ 'libuv/src/unix/freebsd.c' ],
        }],
        [ 'OS=="freebsd"', {
          'sources': [ 'libuv/src/unix/random-getrandom.c' ],
        }],
        [ 'OS=="openbsd"', {
          'sources': [
            'libuv/src/unix/openbsd.c',
            'libuv/src/unix/random-getentropy.c',
          ],
        }],
        [ 'OS=="netbsd"', {
          'link_settings': {
            'libraries': [ '-lkvm' ],
          },
          'sources': [ 'libuv/src/unix/netbsd.c' ],
        }],
        [ 'OS in "freebsd dragonflybsd openbsd netbsd".split()', {
          'sources': [
            'libuv/src/unix/posix-hrtime.c',
            'libuv/src/unix/bsd-proctitle.c'
          ],
        }],
        [ 'OS in "ios mac freebsd dragonflybsd openbsd netbsd".split()', {
          'sources': [
            'libuv/src/unix/bsd-ifaddrs.c',
            'libuv/src/unix/kqueue.c',
          ],
        }],
        ['uv_library=="shared_library"', {
          'defines': [ 'BUILDING_UV_SHARED=1' ]
        }],
        ['OS=="zos"', {
          'sources': [
            'libuv/src/unix/pthread-fixes.c',
            'libuv/src/unix/os390.c',
            'libuv/src/unix/os390-syscalls.c'
          ]
        }],
      ]
    },
  ]
}
