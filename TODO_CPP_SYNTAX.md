## Peer.cpp

uint8_t Peer::rtcpBuffer[MS_RTCP_BUFFER_SIZE];

this->timer->Start(uint64_t(RTC::RTCP::MAX_VIDEO_INTERVAL_MS / 2));

if (packet->GetSize() > MS_RTCP_BUFFER_SIZE)

uint64_t interval = RTC::RTCP::MAX_VIDEO_INTERVAL_MS;

if (interval > RTC::RTCP::MAX_VIDEO_INTERVAL_MS)


## RtpReceiver.cpp

uint8_t RtpReceiver::rtcpBuffer[MS_RTCP_BUFFER_SIZE];

if (packet.GetSize() > MS_RTCP_BUFFER_SIZE)

packet.Serialize(RtpReceiver::rtcpBuffer);


##RtpSender.cpp

this->maxRtcpInterval = RTC::RTCP::MAX_AUDIO_INTERVAL_MS;
