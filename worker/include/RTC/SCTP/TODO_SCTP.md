# TODO STCP

Here some notes about our ongoing SCTP implementation.

## Related to dcsctp

- Check all calls to `CreatePacket()` in `Socket.cpp` since many of them must be replaced to `this->tbc->CreatePacket()`.

## Flow

1. A DTLS packet arrives to `WebRtcTransport` where we check `DtlsTransport::IsDtls()`).
2. It calls `OnDtlsDataReceived()` that calls `this->dtlsTransport->ProcessDtlsData()`.
3. It calls `this->listener->OnDtlsTransportApplicationDataReceived()` in `WebRtcTransport`.
4. It calls `Transport::ReceiveSctpData()` that calls `this->sctpAssociation->ProcessSctpData()`.

However, in step 4 `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` should instead call `RTC::SCTP::Packet::parse()` and `Transport::ReceiveSctpPacket()` with a `SCTP::Packet` instance instead than `data` and `len`. In fact it should be named `Transport::ReceiveSctpPacket()` instead of the current `Transport::ReceiveSctpData()`.

Same in `PipeTransport` and `PlainTransport`.
