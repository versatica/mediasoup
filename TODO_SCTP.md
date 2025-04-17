# STCP

Here some notes about our future SCTP implementation.

## Flow

1. A DTLS packet arrives to `WebRtcTransport` where we check `DtlsTransport::IsDtls()`).
2. It calls `OnDtlsDataReceived()` that calls `this->dtlsTransport->ProcessDtlsData()`.
3. It calls `this->listener->OnDtlsTransportApplicationDataReceived()` in `WebRtcTransport`.
4. It calls `Transport::ReceiveSctpData()` that calls `this->sctpAssociation->ProcessSctpData()`.

However, in step 4 `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` should instead call `RTC::SCTP::Packet::parse()` and `Transport::ReceiveSctpPacket()` with a `SCTP::Packet` instance instead than `data` and `len`. In fact it should be named `Transport::ReceiveSctpPacket()` instead of the current `Transport::ReceiveSctpData()`.

Same in `PipeTransport` and `PlainTransport`.

## TODO

- In `HeartbeatChunk` the `HeartbeatInfoChunkParameter` should be mandatory when parsing. Or should we add some `Validate()` method?

- What happens if I call `chunk->BuildParameterInPlace()` in a Chunk class that is not supposed to have chunks??? It should not be allowed and should throw.

- Keep `Packet::AddChunk()` and `Chunk::AddParameter()`? Do we really need them having the `BuildXxxxInPlace()` methods?
