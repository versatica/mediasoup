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

- In every test, memset(buffer) to cero after `xxx->Serialize()`.

- Improve docs of `Serialize()`, `Clone()`, `CloneInto()` in Serializable.hpp.

- Everywhere: Replace "parent class" with "base class".

- Check all pending "TODO" comments.

- in `Packet::Parse()` we call `Chunk::IsChunk()` for each possible Chunk in the Packet and then each `XxxxChunk::Parse()` calls it again. Improve it.

- In `HeartbeatChunk` the `HeartbeatInfoChunkParameter` should be mandatory when parsing. Or should we add some `Validate()` method?

- What happens if I call `chunk->BuildParameterInPlace()` in a Chunk class that is not supposed to have chunks??? It should not be allowed and should throw.

- Keep `Packet::AddChunk()` and `Chunk::AddParameter()`? Do we really need them having the `BuildXxxxInPlace()` methods?
