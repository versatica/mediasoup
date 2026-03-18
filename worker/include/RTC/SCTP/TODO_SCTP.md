# TODO STCP

## Related to mediasoup SCTP implementation

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- We to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.

- In `Association::FillBuffer()` we should not pass `this->sctpOptions.maxOutboundStreams/maxInboundStreams` but the current values (they may have been modified via "reconfig").

- We must call `association->Connect()` somewhere!

- Look for "TODO: SCTP" and `MS_SCTP_STACK`.

## Related to dcsctp

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.
