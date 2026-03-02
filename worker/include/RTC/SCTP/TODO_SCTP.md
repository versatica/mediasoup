# TODO STCP

## Related to mediasoup SCTP implementation

- `SocketListener` callbacks cannpt include `Socket* socket` as first argument because the `listener` is given to other subclasses and those cannot invoke listener callbacks with `this` (because they are not `Socket` instances). So we may have to remove `Socket* socket` from the signatures of `SocketListener` callbacks. However if we do that, how can the parent class correlate them? Should we assume that we will have a `SCTP::Association` parent class that handles a **single** `SCTP::Socket` instance and also `DataProducers/DataConsumers`?

- We don't have `packet_sender_` so neither `OnSentPacket()` callback so we must manually increase `this->privateMetrics.txPacketsCount`, but we cannot do that in `TransmissionControlBlock::SendPacket()` because there are no metrics in there, so how to do it?

## Related to dcsctp

- Check all calls to `CreatePacket()` in `Socket.cpp` since many of them must be replaced to `this->tbc->CreatePacket()`.

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.

## Flow

1. A DTLS packet arrives to `WebRtcTransport` where we check `DtlsTransport::IsDtls()`).
2. It calls `OnDtlsDataReceived()` that calls `this->dtlsTransport->ProcessDtlsData()`.
3. It calls `this->listener->OnDtlsTransportApplicationDataReceived()` in `WebRtcTransport`.
4. It calls `Transport::ReceiveSctpData()` that calls `this->sctpAssociation->ProcessSctpData()`.

However, in step 4 `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` should instead call `RTC::SCTP::Packet::parse()` and `Transport::ReceiveSctpPacket()` with a `SCTP::Packet` instance instead than `data` and `len`. In fact it should be named `Transport::ReceiveSctpPacket()` instead of the current `Transport::ReceiveSctpData()`.

Same in `PipeTransport` and `PlainTransport`.
