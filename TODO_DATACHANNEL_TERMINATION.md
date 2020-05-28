# TODO DataChannel Termination


## Implementation

- Should we store somewhere a message received via `dataProducer.send()` when we deliver it to a `dataConsumer` (of type 'sctp' or 'direct')?


## Documentation

- Set proper max size for `netstring` messages in `PayloadChannel` (in JS and C++). It must match `maxSctpMessageSize` in `DataTransport.ts`.

- `sctpStreamParameters` are now optional in `DataProducerOptions`.

- `sctpStreamParameters` getter can now return `undefined` in `DataProducer/Consumer`.

- Added `dataProducer/Consumer.type`: 'sctp' | 'direct'.

- `transport.consumeData()` (when in SCTP) now accepts optional arguments:

```ts
ordered?: boolean;
maxPacketLifeTime?: number;
maxRetransmits?: number;
```

- Added a new `direct` log tag.
