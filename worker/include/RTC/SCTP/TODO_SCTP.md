# TODO STCP

## Related to mediasoup SCTP implementation

- Add `AssociationListener::OnAssociationFailed()`.

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- Do we need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`?

- In `Transport::FillBuffer()` we need `this->sctpAssociation->FillBuffer(builder)`.

- There is no association state "NEW" or "FAILED" anymore, and this affects `Transport.cpp` and specially FBS types.

- Look for "TODO: SCTP" and `MS_SCTP_STACK`.

## Related to dcsctp

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.
