# TODO DataChannel Termination

- Set proper max size for `netstring` messages in `PayloadChannel` (in JS and C++). It must match `maxSctpMessageSize` in `DataTransport.ts`.

- In `Transport.cpp` how to deal with `this->useRealSctp` and `this->sctpAssociation` for stats/dump and so on?

