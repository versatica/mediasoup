# mediasoup

Powerful [WebRTC](http://www.webrtc.org/) SFU ("Selective Forwarding Unit") server built on Node and C++.


## IMPORTANT!

* Not yet ready, working on it.


## Features

* Not a boring standalone server, but a Node library exposing a pure JavaScript API.
* Supports *WebRTC* media requirements ([ICE-Lite](http://tools.ietf.org/html/rfc5245), [DTLS-SRTP](http://tools.ietf.org/html/rfc5764), [rtcp-mux](http://tools.ietf.org/html/rfc5761), [bundle](http://tools.ietf.org/html/draft-ietf-mmusic-sdp-bundle-negotiation)) along with classic VoIP technologies ([SDES-SRTP](http://tools.ietf.org/html/rfc4568), plain [RTP/RTCP](http://tools.ietf.org/html/rfc3550)).
* ICE/DTLS/RTP/RTCP over UDP and TCP.
* Multi participant real-time sessions.
* IPv6 ready.
* Signaling agnostic. **mediasoup** does not deal with SIP or other signaling protocols. In the other side, it can deal with *any* signaling protocol.
* SDP unaware. Really, **mediasoup** will not parse a SDP for you.
* Extremely powerful. Media handler subprocess (*mediasoup-worker*) is coded in C++ on top of the awesome [libuv](https://github.com/libuv/libuv) asychronous I/O library. **mediasoup** takes full advantage of your CPU capabilities by launching as many workers as needed.


## Usage

```javascript
var mediasoup = require('mediasoup');

// Create a server with 4 worker subprocesses:
var server = mediasoup.Server({ numWorkers: 4 });

// Create a conference room:
var room = server.Room();

// Add a participant into the room:
var peer1 = room.Peer();

// Create a single transport ("bundle") for sending/receiving both audio and video:
peer1.createTransport({ udp: true, tcp: true })
    .then((transport) =>
    {
        transport.iceLocalCandidates.forEach((candidate) =>
        {
            console.log('local ICE candidate: %o', candidate);
        });
    });

// TODO: more stuff coming soon
```

Until the API is documented, the output of the included test units may help a bit understanding how the API will look like:

```bash
ibc in ~/src/mediasoup $ gulp test

[15:10:29] Using gulpfile ~/src/mediasoup/gulpfile.js
[15:10:29] Starting 'test'...

test/test_extra.js
  extra.dtlsFingerprintFromSDP()
    ✓ should be equal
    ✓ should be equal

  extra.dtlsFingerprintToSDP()
    ✓ should be equal
    ✓ should be equal

test/test_mediasoup.js
  mediasoup.Server() with no options must succeed
    ✓ server should close cleanly

  mediasoup.Server() with valid options must succeed
    ✓ server should close cleanly

  mediasoup.Server() with wrong options must fail
    ✓ server should close with error

  mediasoup.Server() with non existing `rtcListenIPv4` must fail
    ✓ server should close with error

  mediasoup.Server() with too narrow RTC ports range must fail
    ✓ server should close with error

test/test_server.js
  server.updateSettings() with no options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with valid options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with invalid options must fail
    ✓ server.updateSettings() failed: Error: invalid value 'wrong_log_level' for logLevel

  server.updateSettings() in a closed server must fail
    ✓ server.updateSettings() failed: Error: server closed

  server.Room() must succeed
    ✓ room should close cleanly

  server.Room() in a closed server must fail
    ✓ server.Room() should throw error

  server.dump() must succeed
    ✓ server.dump() succeeded
    ✓ server.dump() should retrieve two workers

test/test_room.js
  room.Peer() with `peerName` must succeed
    ✓ peer should close cleanly

  room.Peer() without `peerName` must fail
    ✓ room.Peer() should throw error

  room.Peer() with same `peerName` must fail
    ✓ room.Peer() should throw error

  room.Peer() with same `peerName` must succeed if previous peer was closed before
    ✓ room.getPeer() retrieves the first "alice"
    ✓ room.getPeers() returns one peer
    ✓ room.getPeer() retrieves nothing
    ✓ room.getPeers() returns zero peers
    ✓ room.getPeer() retrieves the new "alice"
    ✓ room.getPeers() returns one peer
    ✓ peer should close cleanly

  room.dump() must succeed
    ✓ room.dump() succeeded
    ✓ room.dump() should retrieve two peers

test/test_peer.js
  peer.createTransport() with no options must succeed
    ✓ peer.createTransport() succeeded
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve one transport

  peer.createTransport() with no `udp` nor `tcp` must fail
    ✓ peer.createTransport() failed

test/test_transport.js
  transport.createAssociatedTransport() must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport must have "RTP" `iceComponent`
    ✓ transport must have "new" `dtlsState`
    ✓ transport just contains "udp" candidates
    ✓ transport.createAssociatedTransport() succeeded
    ✓ associated transport must have "RTCP" `iceComponent`
    ✓ associated transport must have "new" `dtlsState`
    ✓ associated transport just contains "udp" candidates
    ✓ associatedTransport.createAssociatedTransport() failed: Error: cannot call CreateAssociatedTransport() on a RTCP Transport
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve two transports

  transport.close() must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport should close cleanly
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve zero transports

  transport.setRemoteDtlsParameters() with "server" `role` must succeed
    ✓ peer.createTransport() succeeded
    ✓ default local DTLS `role` must be "auto"
    ✓ transport.setRemoteDtlsParameters() succeeded
    ✓ new local DTLS `role` must be "client"
    ✓ transport.dump() succeeded
    ✓ local DTLS `role` must be "client"

  transport.setRemoteDtlsParameters() with "server" `auto` must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport.setRemoteDtlsParameters() succeeded
    ✓ new local DTLS `role` must be "client"
    ✓ transport.dump() succeeded
    ✓ local DTLS `role` must be "client"

  transport.setRemoteDtlsParameters() with no `role` must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport.setRemoteDtlsParameters() succeeded
    ✓ new local DTLS `role` must be "client"
    ✓ transport.dump() succeeded
    ✓ local DTLS `role` must be "client"

  transport.setRemoteDtlsParameters() with invalid `role` must fail
    ✓ peer.createTransport() succeeded
    ✓ transport.createAssociatedTransport() failed: Error: invalid `data.role`
    ✓ local DTLS `role` must be "auto"

  transport.setRemoteDtlsParameters() without `fingerprint` must fail
    ✓ peer.createTransport() succeeded
    ✓ transport.createAssociatedTransport() failed: Error: missing `data.fingerprint`
    ✓ local DTLS `role` must be "auto"


  70 passing (7s)
[15:10:36] Finished 'test' after 7.01 s
```


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)
