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

- Write `TestReConfigChunk.cpp` using related Chunk Parameters.

- Instead of `chunk->GetXxxxxAt(idx)` we could return a vector using `std::move()`:

  ```c++
  std::vector<Chunk::ChunkType> chunkTypes;

  // Fill it.

  return std::move(chunk_types);
  ```

- Try to remove all `friend class`.

- In `HeartbeatChunk` the `HeartbeatInfoChunkParameter` should be mandatory when parsing. Or should we add some `Validate()` method?
