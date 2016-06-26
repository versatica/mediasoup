{
  'targets':
  [
    {
      'target_name': 'mediasoup-worker',
      'type': 'executable',
      'dependencies':
      [
        'deps/jsoncpp/jsoncpp.gyp:jsoncpp',
        'deps/netstring/netstring.gyp:netstring',
        'deps/libuv/uv.gyp:libuv',
        'deps/openssl/openssl.gyp:openssl',
        'deps/libsrtp/libsrtp.gyp:libsrtp'
      ],
      'sources':
      [
        # C++ source files
        'src/DepLibSRTP.cpp',
        'src/DepLibUV.cpp',
        'src/DepOpenSSL.cpp',
        'src/DepUsrSCTP.cpp',
        'src/Logger.cpp',
        'src/Loop.cpp',
        'src/Settings.cpp',
        'src/main.cpp',
        'src/Channel/Notifier.cpp',
        'src/Channel/Request.cpp',
        'src/Channel/UnixStreamSocket.cpp',
        'src/RTC/DtlsTransport.cpp',
        'src/RTC/IceCandidate.cpp',
        'src/RTC/IceServer.cpp',
        'src/RTC/Peer.cpp',
        'src/RTC/Room.cpp',
        'src/RTC/RtcpPacket.cpp',
        'src/RTC/RtpListener.cpp',
        'src/RTC/RtpPacket.cpp',
        'src/RTC/RtpReceiver.cpp',
        'src/RTC/RtpSender.cpp',
        'src/RTC/SrtpSession.cpp',
        'src/RTC/StunMessage.cpp',
        'src/RTC/TcpConnection.cpp',
        'src/RTC/TcpServer.cpp',
        'src/RTC/Transport.cpp',
        'src/RTC/TransportTuple.cpp',
        'src/RTC/UdpSocket.cpp',
        'src/RTC/RtpDictionaries/Media.cpp',
        'src/RTC/RtpDictionaries/Parameters.cpp',
        'src/RTC/RtpDictionaries/RTCRtpCodecRtxParameters.cpp',
        'src/RTC/RtpDictionaries/RtcpFeedback.cpp',
        'src/RTC/RtpDictionaries/RtcpParameters.cpp',
        'src/RTC/RtpDictionaries/RtpCapabilities.cpp',
        'src/RTC/RtpDictionaries/RtpCodecMime.cpp',
        'src/RTC/RtpDictionaries/RtpCodecParameters.cpp',
        'src/RTC/RtpDictionaries/RtpEncodingParameters.cpp',
        'src/RTC/RtpDictionaries/RtpFecParameters.cpp',
        'src/RTC/RtpDictionaries/RtpHeaderExtension.cpp',
        'src/RTC/RtpDictionaries/RtpHeaderExtensionParameters.cpp',
        'src/RTC/RtpDictionaries/RtpParameters.cpp',
        'src/RTC/RtpDictionaries/RtpRtxParameters.cpp',
        'src/Utils/Crypto.cpp',
        'src/Utils/File.cpp',
        'src/Utils/IP.cpp',
        'src/handles/SignalsHandler.cpp',
        'src/handles/TcpConnection.cpp',
        'src/handles/TcpServer.cpp',
        'src/handles/Timer.cpp',
        'src/handles/UdpSocket.cpp',
        'src/handles/UnixStreamSocket.cpp',
        # C++ include files
        'include/DepLibSRTP.h',
        'include/DepLibUV.h',
        'include/DepOpenSSL.h',
        'include/DepUsrSCTP.h',
        'include/LogLevel.h',
        'include/Logger.h',
        'include/Loop.h',
        'include/MediaSoupError.h',
        'include/Settings.h',
        'include/Utils.h',
        'include/common.h',
        'include/Channel/Notifier.h',
        'include/Channel/Request.h',
        'include/Channel/UnixStreamSocket.h',
        'include/RTC/DtlsTransport.h',
        'include/RTC/IceCandidate.h',
        'include/RTC/IceServer.h',
        'include/RTC/Parameters.h',
        'include/RTC/Peer.h',
        'include/RTC/Room.h',
        'include/RTC/RtcpPacket.h',
        'include/RTC/RtpDictionaries.h',
        'include/RTC/RtpListener.h',
        'include/RTC/RtpPacket.h',
        'include/RTC/RtpReceiver.h',
        'include/RTC/RtpSender.h',
        'include/RTC/SrtpSession.h',
        'include/RTC/StunMessage.h',
        'include/RTC/TcpConnection.h',
        'include/RTC/TcpServer.h',
        'include/RTC/Transport.h',
        'include/RTC/TransportTuple.h',
        'include/RTC/UdpSocket.h',
        'include/handles/SignalsHandler.h',
        'include/handles/TcpConnection.h',
        'include/handles/TcpServer.h',
        'include/handles/Timer.h',
        'include/handles/UdpSocket.h',
        'include/handles/UnixStreamSocket.h'
      ],
      'include_dirs':
      [
        'include'
      ],
      'conditions':
      [
        # FIPS
        [ 'openssl_fips != ""', {
          'defines': [ 'BUD_FIPS_ENABLED=1' ]
        }],

        # Platform-specifics

        [ 'OS == "mac" and mediasoup_asan == "true"', {
          'xcode_settings':
          {
            'OTHER_CFLAGS': [ '-fsanitize=address' ],
            'OTHER_LDFLAGS': [ '-fsanitize=address' ]
          }
        }],

        [ 'OS == "linux"', {
          'defines':
          [
            '_POSIX_C_SOURCE=200112',
            '_GNU_SOURCE'
          ]
        }],

        [ 'OS == "linux" and mediasoup_asan == "true"', {
          'cflags': [ '-fsanitize=address' ],
          'ldflags': [ '-fsanitize=address' ]
        }],

        [ 'OS in "linux freebsd"', {
          'ldflags':
          [
            '-Wl,--whole-archive <(libopenssl) -Wl,--no-whole-archive'
          ]
        }],

        [ 'OS in "freebsd"', {
          'ldflags': [ '-Wl,--export-dynamic' ]
        }],

        [ 'OS != "win"', {
          'cflags': [ '-std=c++11', '-Wall', '-Wextra', '-Wno-unused-parameter' ]
        }],

        [ 'OS == "mac"', {
          'xcode_settings':
          {
            'WARNING_CFLAGS': [ '-Wall', '-Wextra', '-Wno-unused-parameter' ],
            'OTHER_CPLUSPLUSFLAGS' : [ '-std=c++11' ]
          }
        }]
      ]
    }
  ]
}
