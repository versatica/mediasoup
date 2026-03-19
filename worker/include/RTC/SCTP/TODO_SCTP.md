# TODO STCP

## Related to mediasoup SCTP implementation

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- `OnAssociationFailed()` and `OnAssociationClosed()` should report an error (if present) to JS.

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- We need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.

- In `Association::FillBuffer()` we should not pass `this->sctpOptions.maxOutboundStreams/maxInboundStreams` but the current values (they may have been modified via "reconfig").

- We must call `association->Connect()` somewhere when appropriate! Probably same as the `MayConnect()` in former `SctpAssociation`.

- Test Chrome with I-DATA (message interleaving):

  ```
  open -a "Google Chrome Canary" \
    --args \
    --force-fieldtrials="WebRTC-DataChannelMessageInterleaving/Enabled/"
  ```

- Look for "TODO: SCTP" and `MS_SCTP_STACK`.

## Related to dcsctp

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.
