# TODO STCP

## Related to mediasoup SCTP implementation

- `Association`: When transitioning to CLOSED (due to failure while connecting or closure) we should emit a new event "stcpclosed" in all `DataProducers/Consumers`.

- When receiving SCTP RE-CONFIG, we should emit "streamclosed" in those `DataProducers/DataConsumers` whose stream ID have been closed.

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- `OnAssociationFailed()` and `OnAssociationClosed()` should report an error (if present) to JS.

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- We need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.

- In `Association::FillBuffer()` we should not pass `this->sctpOptions.maxOutboundStreams/maxInboundStreams` but the current values (they may have been modified via "reconfig").

- Probably remove those `MS_DEBUG_TAG(sctp, "xxxx timer has expired")` and make it be `MS_DEBUG_DEV()` instead.

- Test Chrome with I-DATA (message interleaving):

  ```
  open -a "Google Chrome Canary" \
    --args \
    --force-fieldtrials="WebRTC-DataChannelMessageInterleaving/Enabled/"
  ```

- Look for "TODO: SCTP" and `MS_SCTP_STACK` everywhere.

## Related to dcsctp

- Investigate `DcSctpSocket::HandleTimeout()` which is only called from `media/sctp/dcsctp_transport.cc`.
  - Update: This is the entry point when a timer expires. It's the same as our `OnTimer()`.
