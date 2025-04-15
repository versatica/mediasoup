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

- In `HeartbeatChunk` the `HeartbeatInfoChunkParameter` should be mandatory when parsing. Or should we add some `Validate()` method?

- Check all pending "TODO" comments.

- Maybe change this:

  ```c++
  auto* parameter = reinterpret_cast<HeartbeatInfoChunkParameter*>(
    chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO)
  );
  ```

  to this?:

  ```c++
  auto* parameter = chunk->BuildParameterInPlace<HeartbeatInfoChunkParameter>();
  ```

- Same in `Packet::BuildChunkInPlace()`.

- What happens if I call `chunk->BuildParameterInPlace()` in a Chunk class that is not supposed to have chunks??? It should not be allowed and should throw.
