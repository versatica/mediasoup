# mediasoup

Powerful WebRTC SFU ("Selective Forwarding Unit") server built on Node and C++.


## IMPORTANT!

* Not yet ready, working on it.


## Features

* Not a boring standalone server, but a Node library exposing a pure JavaScript API.
* Modern design: Designed with [ORTC](http://ortc.org) in mind, but also valid for [WebRTC](http://www.webrtc.org).
* "Selective Forwarding Unit" architecture as defined in [draft-ietf-avtcore-rtp-topologies-update](https://tools.ietf.org/html/draft-ietf-avtcore-rtp-topologies-update).
* ICE/DTLS/RTP/RTCP/DataChannel over UDP and TCP, IPv4 and IPv6.
* Signaling agnostic: **mediasoup** does not deal with SIP or other signaling protocols. In the other hand, it can deal with *any* signaling protocol.
* Extremely powerful: Media handler subprocess (*mediasoup-worker*) is coded in C++ on top of the awesome [libuv](https://github.com/libuv/libuv) asychronous I/O library. **mediasoup** takes full advantage of your CPU capabilities by launching as many workers as needed.


## Usage

```javascript
var mediasoup = require('mediasoup');

// Create a mediasoup server.
var server = mediasoup.Server();

// Create a conference room.
var room = server.Room();

// Our custom signaling server.
var signalingServer = [...];

signalingServer.on('invite', (request) =>
{
  // Add a participant into the room.
  var peer = room.Peer(request.username);

  // Our signaling response.
  var response =
  {
    candidates: []
  };

  // After inspecting `requests.transports` we need to create a single
  // transport for sending/receiving both audio and video.
  peer.createTransport({ udp: true, tcp: true })
    .then((transport) =>
    {
      // Log DTLS state changes.
      transport.on('dtlsstatechange', (data) => {
        console.log('transport "dtlsstatechange" event [data.dtlsState:%s]', data.dtlsState);
      });

      // Append our local ICE candidates into the response.
      transport.iceLocalCandidates.forEach((candidate) =>
      {
        response.candidates.push(candidate);
      });

      // Set remote DTLS parameters.
      // `transport.setRemoteDtlsParameters()` returns a Promise that resolves
      // with `transport` itself so it can be handled in the next `then()`
      // handler.
      return transport.setRemoteDtlsParameters(
        {
          role        : request.dtlsRole,
          fingerprint :
          {
            algorithm : request.remoteFingerprint.type,
            value     : request.remoteFingerprint.hash
          }
        });
    })
    .then((transport) =>
    {
      // Set our DTLS parameters into the response.
      response.dtlsRole = transport.dtlsLocalParameters.role;
      response.fingerprint =
      {
        type : 'sha-512',
        hash : transport.dtlsLocalParameters.fingerprints['sha-512']
      };

      // And send the response.
      signalingServer.send(response);
    });
});
```

Until the API is documented, the output of the included test units may help a bit understanding how the API will look like:

```bash
ibc in ~/src/mediasoup $ gulp test

Using gulpfile ~/src/mediasoup/gulpfile.js
Starting 'test'...

test/test_mediasoup.js
  mediasoup.Server() with no options must succeed
    ✓ server should close cleanly

  mediasoup.Server() with valid options must succeed
    ✓ server should close cleanly

  mediasoup.Server() with wrong options must fail
    ✓ server should close with error

  mediasoup.Server() with non existing `rtcListenIPv4` IP must fail
    ✓ server should close with error

  mediasoup.Server() with too narrow RTC ports range must fail
    ✓ server should close with error

test/test_Server.js
  server.updateSettings() with no options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with valid options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with invalid options must fail
    ✓ server.updateSettings() failed: Error: invalid value 'chicken' for logLevel

  server.updateSettings() in a closed server must fail
    ✓ server.updateSettings() failed: InvalidStateError: Server closed
    ✓ server.updateSettings() error must be InvalidStateError

  server.Room() must succeed
    ✓ room should close cleanly

  server.Room() in a closed server must fail
    ✓ server.Room() should throw InvalidStateError

  server.dump() must succeed
    ✓ server.dump() succeeded
    ✓ server.dump() should retrieve two workers

test/test_Room.js
  room.Peer() with `peerName` must succeed
    ✓ peer should close cleanly

  room.Peer() without `peerName` must fail
    ✓ room.Peer()) should throw TypeError

  room.Peer() with same `peerName` must fail
    ✓ room.Peer() should throw InvalidStateError

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

test/test_Peer.js
  peer.createTransport() with no options must succeed
    ✓ peer.createTransport() succeeded
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve one transport

  peer.createTransport() with no `udp` nor `tcp` must fail
    ✓ peer.createTransport() failed

  peer.RtpReceiver() with valid `transport` must succeed
    ✓ peer.createTransport() succeeded
    ✓ rtpReceiver.transport must retrieve the given `transport`
    ✓ rtpReceiver.rtcpTransport must retrieve the given `transport`
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve one rtpReceiver
    ✓ rtpReceiver should close cleanly

  peer.RtpReceiver() with valid `transport` and `rtcpTransport` must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport.createAssociatedTransport() succeeded
    ✓ rtpReceiver.transport must retrieve the given `transport`
    ✓ rtpReceiver.rtcpTransport must retrieve the given `rtcpTransport`

  peer.RtpReceiver() with a closed `transport` must fail
    ✓ peer.createTransport() succeeded
    ✓ peer.RtpReceiver() should throw InvalidStateError

test/test_Transport.js
  transport.createAssociatedTransport() must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport must have "controlled" `iceRole`
    ✓ transport must have "RTP" `iceComponent`
    ✓ transport must not have `iceSelectedTuple`
    ✓ transport must have "new" `iceState`
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
    ✓ `transport.iceState` must be "closed"
    ✓ `transport.dtlsState` must be "closed"
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve zero transports

  transport.setRemoteDtlsParameters() with "server" `role` must succeed
    ✓ peer.createTransport() succeeded
    ✓ default local DTLS `role` must be "auto"
    ✓ transport.setRemoteDtlsParameters() succeeded
    ✓ new local DTLS `role` must be "client"
    ✓ transport.dump() succeeded
    ✓ local DTLS `role` must be "client"

  transport.setRemoteDtlsParameters() with "auto" `role` must succeed
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

test/test_RtpReceiver.js
  rtpReceiver.receive() must succeed
    ✓ rtpReceiver.receive() succeeded

  rtpReceiver.close() must succeed
    ✓ rtpReceiver should close cleanly
    ✓ peer.dump() succeeded
    ✓ peer.dump() should retrieve zero rtpReceivers

test/test_extra.js
  extra.fingerprintFromSDP()
    ✓ should be equal
    ✓ should be equal

  extra.fingerprintToSDP()
    ✓ should be equal
    ✓ should be equal

  92 passing (7s
```


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)
