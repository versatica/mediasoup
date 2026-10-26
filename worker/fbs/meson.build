flatbuffers_schemas = [
  'activeSpeakerObserver.fbs',
  'audioLevelObserver.fbs',
  'common.fbs',
  'consumer.fbs',
  'dataConsumer.fbs',
  'dataProducer.fbs',
  'directTransport.fbs',
  'log.fbs',
  'message.fbs',
  'notification.fbs',
  'pipeTransport.fbs',
  'plainTransport.fbs',
  'producer.fbs',
  'request.fbs',
  'response.fbs',
  'router.fbs',
  'rtpObserver.fbs',
  'rtpPacket.fbs',
  'rtpParameters.fbs',
  'rtpStream.fbs',
  'rtxStream.fbs',
  'sctpAssociation.fbs',
  'sctpParameters.fbs',
  'srtpParameters.fbs',
  'transport.fbs',
  'webRtcServer.fbs',
  'webRtcTransport.fbs',
  'worker.fbs',
]

# The schemas are turned into file objects here because the target that
# consumes them is declared in the FBS subdirectory, where plain relative names
# would no longer resolve.
flatbuffers_schema_files = files(flatbuffers_schemas)

# FBS is the directory from which worker code includes the generated header
# files. The generator target is declared from within that very subdirectory
# because Meson does not allow path segments in the outputs of a custom target,
# and the outputs must be the individual headers rather than the directory that
# holds them. Declaring the directory as the output makes the target dirty
# forever: running the generator rewrites the headers in place, which never
# updates the modification time of the directory containing them, so the target
# never becomes up to date and every build regenerates the headers and
# recompiles every source file that includes them.
subdir('FBS')

flatbuffers_generator_dep = declare_dependency(
  include_directories: '.',
  sources: flatbuffers_generator,
)
