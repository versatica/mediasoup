# TODO STCP

## Related to mediasoup SCTP implementation

- `Association`: When transitioning to CLOSED (due to failure while connecting or closure) we should emit a new event "stcpclosed" in all `DataProducers/Consumers`.

- When receiving SCTP RE-CONFIG, we should emit "streamclosed" in those `DataProducers/DataConsumers` whose stream ID have been closed.

- Why the hell does `DataConsumer` have a `RTC::SctpAssociation* sctpAssociation` member?

- `OnAssociationFailed()` and `OnAssociationClosed()` should report an error (if present) to JS.

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- We must remove `numSctpStreams` option given to `router.createXxxTransport()` and `NumSctpStreams` type. `OS` and `MIS` in `numSctpStreams` are just the max announced number of outbound and incoming SCTP streams, but in the new SCTP stack those should always be 65535. The max number of incoming and outgoing streams will be negotiated later with the SCTP INIT and INIT_ACK and will be the minimum of our values (65535) and the OS and MIS that the peer announces in its INIT or INIT_ACK.
  - This is a breaking change.
  - Remove it from `sctpParameters.fbs` and other FBS types (look for `MIS` or `mis`, etc).
  - Remove it in Rust layer.
  - We must also remove `device.sctpCapabilities` getter from mediasoup-client because anyway we are making up those values!
  - Also must update the website documentation.

- In `transport.dump()` (maybe also in `getStats()`) we must properly obtain `OS` and `MIS` according to the number of SCTP streams negotiated via INIT + INIT_ACK. And if SCTP is not yet established, then... not sure.
  - In `Association::FillBuffer()` we should not pass `this->sctpOptions.negotiatedMaxOutboundStreams/negotiatedMaxInboundStreams` but the current values.

- We need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.
  - Well, let's see. If it's only for when changing number of OS/MIS... then the new SCTP stack doesn't support it so...

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
