#include "common.hpp"

extern "C" int run_worker(
  int argc,
  char* argv[],
  const char* version,
  int consumerChannelFd,
  int producerChannelFd,
  int payloadConsumeChannelFd,
  int payloadProduceChannelFd,
  ChannelWriteFn channelWriteFn,
  ChannelWriteCtx channelWriteCtx,
  PayloadChannelWriteFn payloadChannelWriteFn,
  PayloadChannelWriteCtx payloadChannelWriteCtx);
