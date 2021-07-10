project(
  'mediasoup-worker',
  ['c', 'cpp'],
  default_options : [
    'warning_level=1',
    'cpp_std=c++11',
    'default_library=static',
  ],
  meson_version: '>= 0.58',
)

cpp_args = [
  host_machine.endian() == 'little' ? '-DMS_LITTLE_ENDIAN' : '-DMS_BIG_ENDIAN',
  '-DABSL_ALLOCATOR_NOTHROW=1',
]

if host_machine.system() == 'windows'
  cpp_args += [
    '-DNOMINMAX', # Don't define min and max macros (windows.h)
    # Don't bloat namespace with incompatible winsock versions.
    '-DWIN32_LEAN_AND_MEAN',
    # Don't warn about usage of insecure C functions.
    '-D_CRT_SECURE_NO_WARNINGS',
    '-D_SCL_SECURE_NO_WARNINGS',
    # Introduced in VS 2017 15.8, allow overaligned types in aligned_storage.
    '-D_ENABLE_EXTENDED_ALIGNED_STORAGE'
  ]
endif

common_sources = [
  'src/lib.cpp',
  'src/DepLibSRTP.cpp',
  'src/DepLibUV.cpp',
  'src/DepLibWebRTC.cpp',
  'src/DepOpenSSL.cpp',
  'src/DepUsrSCTP.cpp',
  'src/Logger.cpp',
  'src/MediaSoupErrors.cpp',
  'src/Settings.cpp',
  'src/Worker.cpp',
  'src/Utils/Crypto.cpp',
  'src/Utils/File.cpp',
  'src/Utils/IP.cpp',
  'src/Utils/String.cpp',
  'src/handles/SignalsHandler.cpp',
  'src/handles/TcpConnectionHandler.cpp',
  'src/handles/TcpServerHandler.cpp',
  'src/handles/Timer.cpp',
  'src/handles/UdpSocketHandler.cpp',
  'src/handles/UnixStreamSocket.cpp',
  'src/Channel/ChannelNotifier.cpp',
  'src/Channel/ChannelRequest.cpp',
  'src/Channel/ChannelSocket.cpp',
  'src/PayloadChannel/Notification.cpp',
  'src/PayloadChannel/PayloadChannelNotifier.cpp',
  'src/PayloadChannel/PayloadChannelRequest.cpp',
  'src/PayloadChannel/PayloadChannelSocket.cpp',
  'src/RTC/AudioLevelObserver.cpp',
  'src/RTC/Consumer.cpp',
  'src/RTC/DataConsumer.cpp',
  'src/RTC/DataProducer.cpp',
  'src/RTC/DirectTransport.cpp',
  'src/RTC/DtlsTransport.cpp',
  'src/RTC/IceCandidate.cpp',
  'src/RTC/IceServer.cpp',
  'src/RTC/KeyFrameRequestManager.cpp',
  'src/RTC/NackGenerator.cpp',
  'src/RTC/PipeConsumer.cpp',
  'src/RTC/PipeTransport.cpp',
  'src/RTC/PlainTransport.cpp',
  'src/RTC/PortManager.cpp',
  'src/RTC/Producer.cpp',
  'src/RTC/RateCalculator.cpp',
  'src/RTC/Router.cpp',
  'src/RTC/RtpListener.cpp',
  'src/RTC/RtpObserver.cpp',
  'src/RTC/RtpPacket.cpp',
  'src/RTC/RtpProbationGenerator.cpp',
  'src/RTC/RtpStream.cpp',
  'src/RTC/RtpStreamRecv.cpp',
  'src/RTC/RtpStreamSend.cpp',
  'src/RTC/RtxStream.cpp',
  'src/RTC/SctpAssociation.cpp',
  'src/RTC/SctpListener.cpp',
  'src/RTC/SenderBandwidthEstimator.cpp',
  'src/RTC/SeqManager.cpp',
  'src/RTC/SimpleConsumer.cpp',
  'src/RTC/SimulcastConsumer.cpp',
  'src/RTC/SrtpSession.cpp',
  'src/RTC/StunPacket.cpp',
  'src/RTC/SvcConsumer.cpp',
  'src/RTC/TcpConnection.cpp',
  'src/RTC/TcpServer.cpp',
  'src/RTC/Transport.cpp',
  'src/RTC/TransportCongestionControlClient.cpp',
  'src/RTC/TransportCongestionControlServer.cpp',
  'src/RTC/TransportTuple.cpp',
  'src/RTC/TrendCalculator.cpp',
  'src/RTC/UdpSocket.cpp',
  'src/RTC/WebRtcTransport.cpp',
  'src/RTC/Codecs/H264.cpp',
  'src/RTC/Codecs/VP8.cpp',
  'src/RTC/Codecs/VP9.cpp',
  'src/RTC/RtpDictionaries/Media.cpp',
  'src/RTC/RtpDictionaries/Parameters.cpp',
  'src/RTC/RtpDictionaries/RtcpFeedback.cpp',
  'src/RTC/RtpDictionaries/RtcpParameters.cpp',
  'src/RTC/RtpDictionaries/RtpCodecMimeType.cpp',
  'src/RTC/RtpDictionaries/RtpCodecParameters.cpp',
  'src/RTC/RtpDictionaries/RtpEncodingParameters.cpp',
  'src/RTC/RtpDictionaries/RtpHeaderExtensionParameters.cpp',
  'src/RTC/RtpDictionaries/RtpHeaderExtensionUri.cpp',
  'src/RTC/RtpDictionaries/RtpParameters.cpp',
  'src/RTC/RtpDictionaries/RtpRtxParameters.cpp',
  'src/RTC/SctpDictionaries/SctpStreamParameters.cpp',
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
  'src/RTC/RTCP/FeedbackRtpTransport.cpp',
  'src/RTC/RTCP/FeedbackPsPli.cpp',
  'src/RTC/RTCP/FeedbackPsSli.cpp',
  'src/RTC/RTCP/FeedbackPsRpsi.cpp',
  'src/RTC/RTCP/FeedbackPsFir.cpp',
  'src/RTC/RTCP/FeedbackPsTst.cpp',
  'src/RTC/RTCP/FeedbackPsVbcm.cpp',
  'src/RTC/RTCP/FeedbackPsLei.cpp',
  'src/RTC/RTCP/FeedbackPsAfb.cpp',
  'src/RTC/RTCP/FeedbackPsRemb.cpp',
  'src/RTC/RTCP/XR.cpp',
  'src/RTC/RTCP/XrDelaySinceLastRr.cpp',
  'src/RTC/RTCP/XrReceiverReferenceTime.cpp',
]

cpp = meson.get_compiler('cpp')

openssl_dep = dependency(
  'openssl',
  static: get_option('default_library') == 'static',
  version: '>= 1.1.1'
)

nlohmann_json_dep = dependency('nlohmann_json')

libuv_dep = dependency(
  'libuv',
  default_options: [
    'warning_level=0',
    'build_tests=false',
    'build_benchmarks=false',
  ],
  version: '>= 1.40'
)
libsrtp2_dep = dependency(
  'libsrtp2',
  default_options: [
    'warning_level=0',
    'crypto-library=openssl',
    'tests=disabled',
  ],
  version: '>= 2.3'
)
usrsctp_dep = dependency(
  'usrsctp',
  default_options: [
    'warning_level=0',
    'sctp_build_programs=false',
  ],
)
abseil_cpp_proj = subproject(
  'abseil-cpp',
  default_options: [
    'warning_level=0',
  ],
)
subdir('deps/netstring')
libwebrtc_include_directories = include_directories('include')
subdir('deps/libwebrtc')

dependencies = [
  openssl_dep,
  nlohmann_json_dep,
  libuv_dep,
  libsrtp2_dep,
  usrsctp_dep,
  netstring_dep,
  libwebrtc_dep,
]

if host_machine.system() == 'windows'
  subdir('dep/getopt')
  dependencies += [
    getopt_dep
  ]
endif

libmediasoup_worker = library(
  'libmediasoup-worker',
  name_prefix: '',
  build_by_default: false,
  dependencies: dependencies,
  sources: common_sources,
  include_directories: include_directories('include'),
  cpp_args: cpp_args,
)

executable(
  'mediasoup-worker',
  build_by_default: true,
  dependencies: dependencies,
  sources: common_sources + ['src/main.cpp'],
  include_directories: include_directories('include'),
  cpp_args: cpp_args + ['-DMS_EXECUTABLE'],
)

mediasoup_worker_test = executable(
  'mediasoup-worker-test',
  build_by_default: false,
  dependencies: dependencies + [
    dependency('catch2'),
  ],
  sources: common_sources + [
    'test/src/tests.cpp',
    'test/src/RTC/TestKeyFrameRequestManager.cpp',
    'test/src/RTC/TestNackGenerator.cpp',
    'test/src/RTC/TestRateCalculator.cpp',
    'test/src/RTC/TestRtpPacket.cpp',
    'test/src/RTC/TestRtpStreamSend.cpp',
    'test/src/RTC/TestRtpStreamRecv.cpp',
    'test/src/RTC/TestSeqManager.cpp',
    'test/src/RTC/TestTrendCalculator.cpp',
    'test/src/RTC/TestRtpEncodingParameters.cpp',
    'test/src/RTC/Codecs/TestVP8.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsAfb.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsFir.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsLei.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsPli.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsRemb.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsRpsi.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsSli.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsTst.cpp',
    'test/src/RTC/RTCP/TestFeedbackPsVbcm.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpEcn.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpNack.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpSrReq.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpTllei.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpTmmb.cpp',
    'test/src/RTC/RTCP/TestFeedbackRtpTransport.cpp',
    'test/src/RTC/RTCP/TestBye.cpp',
    'test/src/RTC/RTCP/TestReceiverReport.cpp',
    'test/src/RTC/RTCP/TestSdes.cpp',
    'test/src/RTC/RTCP/TestSenderReport.cpp',
    'test/src/RTC/RTCP/TestPacket.cpp',
    'test/src/RTC/RTCP/TestXr.cpp',
    'test/src/Utils/TestBits.cpp',
    'test/src/Utils/TestIP.cpp',
    'test/src/Utils/TestJson.cpp',
    'test/src/Utils/TestString.cpp',
    'test/src/Utils/TestTime.cpp',
  ],
  include_directories: include_directories(
    'include',
    'test/include',
  ),
  cpp_args: cpp_args + [
    '-DMS_LOG_STD',
    '-DMS_TEST',
  ],
)

test(
   'mediasoup-worker-test',
   mediasoup_worker_test,
   workdir: meson.source_root(),
)
