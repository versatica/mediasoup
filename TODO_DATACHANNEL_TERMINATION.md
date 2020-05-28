# TODO DataChannel Termination

- Set proper max size for `netstring` messages in `PayloadChannel` (in JS and C++). It must match `maxSctpMessageSize` in `DataTransport.ts`.

- `sctpStreamParameters` are now optional in `DataProducerOptions`.

- `sctpStreamParameters` getter can now return `undefined` in `DataProducer/Consumer`.

- Added `dataProducer/Consumer.type`: 'sctp' | 'direct'.

- `DirectTransport.cpp` must call `Transport::Connected()` in its constructor.

- `transport.consumeData()` (when in SCTP) now accepts optional arguments:

```ts
ordered?: boolean;
maxPacketLifeTime?: number;
maxRetransmits?: number;
```

- How to increment bytes counter in `Transport` when `DATA_PRODUCER_SEND`? Ok, it must be done in `Transport::HandleNotification()`. It must verify the notification fields and call `DataReceived()`.
  + DONE

- Same for `DataSent()` when notifying a `message` to a direct `DataConsumer`.

