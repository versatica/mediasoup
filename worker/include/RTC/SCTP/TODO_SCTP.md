# TODO STCP

## Related to mediasoup SCTP implementation

- Rename all "Packet", "Chunk", "Parameter", "Error Cause", "Association", etc to lowcase everywhere (in code and comments).

- Rename all "I_DATA" etc to `I-DATA" everywhere (in code and comments).

- Probably add many more fields in `SctpOptions` given to the `Association` in `Transport.cpp`.

- When running `test-PipeTransport.ts` and `test-werift-sctp.ts` tests pass but those errors show up (must build worker with `-DMS_LOG_STD`):

  ```
  mediasoup:ERROR:Worker (stderr) UnixStreamSocketHandle::Write() | uv_try_write() failed, trying uv_write(): broken pipe
  ```

- In the doc add all new options such as `maxSendMessageSize`, `sctpXxx`, etc, deprecate (or remove) `maxMessageSize` and `maxSctpMessageSize` and document also the new default values in `pipeToRouter()`.

- In `transport.dump()` (maybe also in `getStats()`) we must properly obtain current `OS` and `MIS` according to the number of SCTP streams negotiated via INIT + INIT_ACK. And if SCTP is not yet established, then... not sure.

- Use the `AssociationMetrics`. Expose them in transport stats, etc.

- Fix the documentation in the website which says: "The underlaying SCTP association uses a common send buffer for all data consumers, hence the value given by this method indicates the data buffered for all data consumers in the transport."

- Need Node/Rust tests for `dataConsumer->setBufferedAmountLowThreshold()` and so on. In Rust there is `smoke.rs`.

- In `sctpParametersTypes.ts` and `sctp_parameters.rs` I've added back previous fields for backwards compatibility. Must be removed in the future.

- Look for "TODO: SCTP" everywhere (also in `worker/test/` and `node/src/` and `rust/`).

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
