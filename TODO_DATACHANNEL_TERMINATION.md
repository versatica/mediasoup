# TODO DataChannel Termination

- Set proper max size for `netstring` messages in `PayloadChannel` (in JS and C++). It must match `maxSctpMessageSize` in `DataTransport.ts`.

- In `Transport.cpp` how to deal with `this->useRealSctp` and `this->sctpAssociation` for stats/dump and so on?

- En `DataProducerOptions` now `sctpStreamParameters` are optional.

- `sctpStreamParameters` getter can now return `undefined` in `DataProducer/Consumer`.

- Added `dataProducer/Consumer.type`: 'sctp' | 'direct'.

- `DirectTransport.cpp` must set `this->direct = true` and call `Transport::Connected()` in its constructor.

