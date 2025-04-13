# STCP

Here some notes about our future SCTP implementation.

## Flow

1. A DTLS packet arrives to `WebRtcTransport` where we check `DtlsTransport::IsDtls()`).
2. It calls `OnDtlsDataReceived()` that calls `this->dtlsTransport->ProcessDtlsData()`.
3. It calls `this->listener->OnDtlsTransportApplicationDataReceived()` in `WebRtcTransport`.
4. It calls `Transport::ReceiveSctpData()` that calls `this->sctpAssociation->ProcessSctpData()`.

However, in step 4 `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` should instead check `RTC::SCTP::Packet.isSctp()` and then `RTC::SCTP::Packet::parse()` and call `Transport::ReceiveSctpData()` with a `SCTP::Packet` instance instead than `data` and `len`. In fact it should be named `Transport::ReceiveSctpPacket()` instead.

Same in `PipeTransport` and `PlainTransport`.

## TODO

- In `DataChunk::SetUserData()` and `HeartbeatInfoChunkParameter::SetInfo()` we should assert that given length is not greater than max of uint16_t minus the length of the Chunk or Parameter header. This is because Length field is uint16_t but it also includes the header length.

- In tests make `Serialize()` and `Clone()` fail due to small buffer to see if ASAN shows some leak due to some pointer not released.
