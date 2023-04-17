#include "common.hpp"

extern "C" int mediasoup_worker_run(
  int argc,
  char* argv[],
  const char* version,
  int consumerChannelFd,
  int producerChannelFd,
  int payloadConsumeChannelFd,
  int payloadProduceChannelFd,
  ChannelReadFn channelReadFn,
  ChannelReadCtx channelReadCtx,
  ChannelWriteFn channelWriteFn,
  ChannelWriteCtx channelWriteCtx);
