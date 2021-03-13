extern "C" int run_worker(
    int argc,
    char* argv[],
    const char* version,
    bool processMode,
    int consumerChannelFd,
    int producerChannelFd,
    int payloadConsumeChannelFd,
    int payloadProduceChannelFd
);
