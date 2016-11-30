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
        'src/RTC/RtpBuffer.cpp',
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
        'src/RTC/RTCP/Packet.cpp',
        'src/RTC/RTCP/CompoundPacket.cpp',
        'src/RTC/RTCP/SenderReport.cpp',
        'src/RTC/RTCP/ReceiverReport.cpp',
        'src/RTC/RTCP/Sdes.cpp',
        'src/RTC/RTCP/Bye.cpp',
        'src/RTC/RTCP/Feedback.cpp',
        'src/RTC/RTCP/FeedbackPs.cpp',
        'src/RTC/RTCP/FeedbackRtp.cpp',
        'src/RTC/RTCP/FeedbackRtpNack.cpp',
        'src/RTC/RTCP/FeedbackRtpTmmb.cpp',
        'src/RTC/RTCP/FeedbackRtpSrReq.cpp',
        'src/RTC/RTCP/FeedbackRtpTllei.cpp',
        'src/RTC/RTCP/FeedbackRtpEcn.cpp',
        'src/RTC/RTCP/FeedbackPsPli.cpp',
        'src/RTC/RTCP/FeedbackPsSli.cpp',
        'src/RTC/RTCP/FeedbackPsRpsi.cpp',
        'src/RTC/RTCP/FeedbackPsFir.cpp',
        'src/RTC/RTCP/FeedbackPsTst.cpp',
        'src/RTC/RTCP/FeedbackPsVbcm.cpp',
        'src/RTC/RTCP/FeedbackPsLei.cpp',
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
        'include/RTC/RtpBuffer.h',
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
        'include/RTC/RTCP/Packet.h',
        'include/RTC/RTCP/CompoundPacket.h',
        'include/RTC/RTCP/SenderReport.h',
        'include/RTC/RTCP/ReceiverReport.h',
        'include/RTC/RTCP/Sdes.h',
        'include/RTC/RTCP/Bye.h',
        'include/RTC/RTCP/Feedback.h',
        'include/RTC/RTCP/FeedbackItem.h',
        'include/RTC/RTCP/FeedbackPs.h',
        'include/RTC/RTCP/FeedbackRtp.h',
        'include/RTC/RTCP/FeedbackRtpNack.h',
        'include/RTC/RTCP/FeedbackRtpTmmb.h',
        'include/RTC/RTCP/FeedbackRtpSrReq.h',
        'include/RTC/RTCP/FeedbackRtpTllei.h',
        'include/RTC/RTCP/FeedbackRtpEcn.h',
        'include/RTC/RTCP/FeedbackPsPli.h',
        'include/RTC/RTCP/FeedbackPsSli.h',
        'include/RTC/RTCP/FeedbackPsRpsi.h',
        'include/RTC/RTCP/FeedbackPsFir.h',
        'include/RTC/RTCP/FeedbackPsTst.h',
        'include/RTC/RTCP/FeedbackPsVbcm.h',
        'include/RTC/RTCP/FeedbackPsLei.h',
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

        # Endianness
        [ 'node_byteorder=="big"', {
            # Define Big Endian
            'defines': ['MS_BIG_ENDIAN']
          }, {
            # Define Little Endian
            'defines': ['MS_LITTLE_ENDIAN']
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
