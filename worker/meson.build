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
    '-D_ENABLE_EXTENDED_ALIGNED_STORAGE',
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
  'src/RTC/ActiveSpeakerObserver.cpp',
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

openssl_proj = subproject(
  'openssl',
  default_options: [
    'warning_level=0',
  ],
)
nlohmann_json_proj = subproject(
  'nlohmann_json',
  default_options: [
    'warning_level=0',
  ],
)
libuv_proj = subproject(
  'libuv',
  default_options: [
    'warning_level=0',
    'build_tests=false',
    'build_benchmarks=false',
  ],
)
libsrtp2_proj = subproject(
  'libsrtp2',
  default_options: [
    'warning_level=0',
    'crypto-library=openssl',
    'crypto-library-kdf=disabled',
    'tests=disabled',
  ],
)
usrsctp_proj = subproject(
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
catch2_proj = subproject(
  'catch2',
  default_options: [
    'warning_level=0',
  ],
)
libwebrtc_include_directories = include_directories('include')
subdir('deps/libwebrtc')

dependencies = [
  abseil_cpp_proj.get_variable('absl_container_dep'),
  openssl_proj.get_variable('openssl_dep'),
  nlohmann_json_proj.get_variable('nlohmann_json_dep'),
  libuv_proj.get_variable('libuv_dep'),
  libsrtp2_proj.get_variable('libsrtp2_dep'),
  usrsctp_proj.get_variable('usrsctp_dep'),
  libwebrtc_dep,
]

link_whole = [
  abseil_cpp_proj.get_variable('absl_container_lib'),
  openssl_proj.get_variable('libcrypto_lib'),
  openssl_proj.get_variable('libssl_lib'),
  libuv_proj.get_variable('libuv'),
  libsrtp2_proj.get_variable('libsrtp2'),
  usrsctp_proj.get_variable('usrsctp'),
  libwebrtc,
]

if host_machine.system() == 'windows'
  wingetopt_proj = subproject(
    'wingetopt',
    default_options: [
      'warning_level=0',
    ],
  )

  dependencies += [
    wingetopt_proj.get_variable('wingetopt_dep'),
  ]
  link_whole += [
    wingetopt_proj.get_variable('wingetopt_lib'),
  ]
endif

libmediasoup_worker = library(
  'libmediasoup-worker',
  name_prefix: '',
  build_by_default: false,
  install: true,
  install_tag: 'libmediasoup-worker',
  dependencies: dependencies,
  sources: common_sources,
  include_directories: include_directories('include'),
  cpp_args: cpp_args,
  link_whole: link_whole,
)

executable(
  'mediasoup-worker',
  build_by_default: true,
  install: true,
  install_tag: 'mediasoup-worker',
  dependencies: dependencies,
  sources: common_sources + ['src/main.cpp'],
  include_directories: include_directories('include'),
  cpp_args: cpp_args + ['-DMS_EXECUTABLE'],
)

mediasoup_worker_test = executable(
  'mediasoup-worker-test',
  build_by_default: false,
  install: true,
  install_tag: 'mediasoup-worker-test',
  dependencies: dependencies + [
    catch2_proj.get_variable('catch2_dep'),
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
  workdir: meson.project_source_root(),
)

if host_machine.system() == 'linux'
  executable(
    'mediasoup-worker-fuzzer',
    build_by_default: false,
    install: true,
    install_tag: 'mediasoup-worker-fuzzer',
    dependencies: dependencies,
    sources: common_sources + [
      'fuzzer/src/fuzzer.cpp',
      'fuzzer/src/FuzzerUtils.cpp',
      'fuzzer/src/RTC/FuzzerStunPacket.cpp',
      'fuzzer/src/RTC/FuzzerRtpPacket.cpp',
      'fuzzer/src/RTC/FuzzerTrendCalculator.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerBye.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPs.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsAfb.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsFir.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsLei.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsPli.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsRemb.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsRpsi.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsSli.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsTst.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackPsVbcm.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtp.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpEcn.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpNack.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpSrReq.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpTllei.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpTmmb.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerFeedbackRtpTransport.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerPacket.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerReceiverReport.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerSdes.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerSenderReport.cpp',
      'fuzzer/src/RTC/RTCP/FuzzerXr.cpp',
    ],
    include_directories: include_directories(
      'include',
      'fuzzer/include',
    ),
    cpp_args: cpp_args + [
      '-g',
      '-O0',
      '-fsanitize=address,fuzzer',
      '-DMS_LOG_STD',
    ],
    link_args: [
      '-fsanitize=address,fuzzer',
    ],
  )
endif
