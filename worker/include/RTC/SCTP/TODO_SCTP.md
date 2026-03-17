# TODO STCP

## Related to mediasoup SCTP implementation

- Missing `Association::IsSctp()`. Use it in `iceCommon.hpp` (tests). Should/may check `IsSctp()` in `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` same as we do in `PlainTransport::OnPacketReceived()`.
  - _NOTE:_ Maybe we don't need it at all since `association->ReceiveSctpData()` already checks it. And we can only check it if we force ports 5000.

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- Do we need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`?

## Related to dcsctp

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.
