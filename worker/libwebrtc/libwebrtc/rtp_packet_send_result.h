// TODO: Replace it with the real original libwebrtc file.

#ifndef RTP_PACKET_SEND_RESULT_H_
#define RTP_PACKET_SEND_RESULT_H_

// Status returned from TimeToSendPacket() family of callbacks.
enum class RtpPacketSendResult {
  kSuccess,               // Packet sent OK.
  kTransportUnavailable,  // Network unavailable, try again later.
  kPacketNotFound  // SSRC/sequence number does not map to an available packet.
};

#endif

