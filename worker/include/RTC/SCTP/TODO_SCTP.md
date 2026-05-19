# TODO STCP

## Related to mediasoup SCTP implementation

- dcsctp uses µs (webrtc::Timestamp::Micros()) internally, while mediasoup uses ms (`DepLibUV::GetTimeMs()`). When porting dcsctp timeout/duration logic, make sure to convert accordingly. Do not mix units in the same field.

- `Association`: When transitioning to CLOSED (due to failure while connecting or closure) we should emit a new event "stcpclosed" in all `DataProducers/Consumers`.

- When receiving SCTP RE-CONFIG, we should emit "streamclosed" in those `DataProducers/DataConsumers` whose stream ID have been closed.

- `OnAssociationFailed()` and `OnAssociationClosed()` should report an error (if present) to JS.

- Rename all "Packet", "Chunk", "Parameter", "Error Cause", "Association", etc to lowcase everywhere (in code and comments).

- Rename all "I_DATA" etc to `I-DATA" everywhere (in code and comments).

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- When running `test-PipeTransport.ts` and `test-werift-sctp.ts` with `useBuiltInSctpStack: true`, tests pass but those errors show up:

  ```
  mediasoup:ERROR:Worker (stderr) UnixStreamSocketHandle::Write() | uv_try_write() failed, trying uv_write(): broken pipe
  ```

- We must remove `numSctpStreams` option given to `router.createXxxTransport()` and `NumSctpStreams` type. `OS` and `MIS` in `numSctpStreams` are just the max announced number of outbound and incoming SCTP streams, but in the new SCTP stack those should always be 65535. The max number of incoming and outgoing streams will be negotiated later with the SCTP INIT and INIT_ACK and will be the minimum of our values (65535) and the OS and MIS that the peer announces in its INIT or INIT_ACK.
  - This is a breaking change.
  - Remove it from `sctpParameters.fbs` and other FBS types (look for `MIS` or `mis`, etc).
  - Remove it in Rust layer.
  - We must also remove `device.sctpCapabilities` getter from mediasoup-client because anyway we are making up those values!
  - Also must update the website documentation.

- When we invoke `close()` on a `DataProducer/Consumer` in server, we must end calling `sctpAssociation->ResetStream([streamId])` so it sends `ReConfig` to peer.

- In `transport.dump()` (maybe also in `getStats()`) we must properly obtain `OS` and `MIS` according to the number of SCTP streams negotiated via INIT + INIT_ACK. And if SCTP is not yet established, then... not sure.
  - In `Association::FillBuffer()` we should not pass `this->sctpOptions.negotiatedMaxOutboundStreams/negotiatedMaxInboundStreams` but the current values.

- We need to pass `isDataChannel` to `SCTP::Association` constructor as we do in former `SctpAssociation`. Also use it in `Association::FillBuffer()`.
  - Well, let's see. If it's only for when changing number of OS/MIS... then the new SCTP stack doesn't support it so...

- Fix `dataConsumer.getBufferedAmount()` which in usrsctp returns the data buffered for all data consumers in the transport but now it will be per `DataConsumer` (SCTP stream).
  - In `DataConsumer` class rename `SetAssociationBufferedAmount()` to `SetBufferedAmount()`.
  - In `DataConsumer` class revisit `SctpAssociationSendBufferFull()` method.
  - Fix the documentation in the website which says: "The underlaying SCTP association uses a common send buffer for all data consumers, hence the value given by this method indicates the data buffered for all data consumers in the transport."

- Look for "TODO: SCTP" everywhere (also in `worker/test/`).

- Test Chrome/Canary with I-DATA (message interleaving):

  ```bash
  /Applications/Google\ Chrome\ Canary.app/Contents/MacOS/Google\ Chrome\ Canary \
    --force-fieldtrials="WebRTC-DataChannelMessageInterleaving/Enabled/" \
    --enable-logging=stderr \
    --v=1 \
  ```

  ### Problem in ReassemblyQueue

  In dsctp there is an `absl::AnyInvocable`, which is a move-only callable, unlike `std::function` which requires the callable to be copyable. The standard equivalent is `std::move_only_function`, introduced in C++23.

  If you use C++23:

  ```cpp
  std::vector<std::move_only_function<void()>> deferredActions;
  ```

  In C++20 there is no `std::move_only_function` (that is C++23). The problem is that `absl::AnyInvocable` accepts move-only callables, while `std::function` requires them to be copyable.

  This is relevant because in dcsctp the lambda captures `data = std::move(data)`, and `UserData` has its copy constructor deleted, so the resulting lambda is not copyable and `std::function` will reject it.

  The solution for C++20 is to move the `UserData` into a `shared_ptr` so that the lambda becomes copyable:

  ```cpp
  std::vector<std::function<void()>> deferredActions;
  ```

  And when adding the action, instead of:

  ```cpp
  // dcsctp - It works because AnyInvocable accepts move-only callables
  deferred_actions.push_back(
      [this, tsn, data = std::move(data)]() mutable {
          queued_bytes_ -= data.size();
          Add(tsn, std::move(data));
      });
  ```

  In your code:

  ```cpp
  // C++20 - UserData is not copyable, so it is wrapped in shared_ptr
  auto sharedData = std::make_shared<UserData>(std::move(data));

  this->deferredResetStreams->deferredActions.push_back(
    [this, tsn, sharedData]() mutable
    {
        this->queuedBytes -= sharedData->GetPayloadLength();
        this->Add(tsn, std::move(*sharedData));
    });
  ```
