# TODO STCP

## Related to mediasoup SCTP implementation

- `Association`: When transitioning to CLOSED (due to failure while connecting or closure) we should emit a new event "stcpclosed" in all `DataProducers/Consumers`.

- When receiving SCTP RE-CONFIG, we should emit "streamclosed" in those `DataProducers/DataConsumers` whose stream ID have been closed.

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- `OnAssociationFailed()` and `OnAssociationClosed()` should report an error (if present) to JS.

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- We need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.

- In `Association::FillBuffer()` we should not pass `this->sctpOptions.maxOutboundStreams/maxInboundStreams` but the current values (they may have been modified via "reconfig").

- Instead of having a protected `sctpAssociation` member in `Transport`, let's make `Transport` subclasses invoke a new method `Transport::SendSctpMessage()` or `Transport::SendMessage()` instead of directly calling `this->sctpAssociation->SendSctpMessage()`.

- Fix `dataConsumer.getBufferedAmount()` which in usrsctp returns the data buffered for all data consumers in the transport but now it will be per `DataConsumer` (SCTP stream).
  - In `DataConsumer` class rename `SetAssociationBufferedAmount()` to `SetBufferedAmount()`.
  - In `DataConsumer` class revisit `SctpAssociationSendBufferFull()` method.
  - Fix the documentation in the website which says: "The underlaying SCTP association uses a common send buffer for all data consumers, hence the value given by this method indicates the data buffered for all data consumers in the transport."

- Look for "TODO: SCTP" everywhere.

- Test Chrome with I-DATA (message interleaving):

  ```
  open -a "Google Chrome Canary" \
    --args \
    --force-fieldtrials="WebRTC-DataChannelMessageInterleaving/Enabled/"
  ```
