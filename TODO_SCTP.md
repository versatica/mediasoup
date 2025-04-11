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

- Remove `Factory()` in item classes and add `AppendChunkInPlace()` that creates a chunk at the end of the packet and returns the pointer so the app can modify it (and make larger) and then call chunk->Consolidate() that should freeze the chunk and emit event on Packet that will recompute its length and so on.
- So probably make `Freeze()` protected or private. And `Unfreeze()`?

## Fuzzer crashes
