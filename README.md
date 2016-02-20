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
      transport.on('dtlsstatechange', (data) =>
      {
        console.log('transport "dtlsstatechange" event [data.dtlsState:%s]', data.dtlsState);
      });

      // Append our local ICE candidates into the response.
      transport.iceLocalCandidates.forEach((candidate) =>
      {
        response.candidates.push(candidate);
      });

      // Set remote DTLS parameters.
      return transport.setRemoteDtlsParameters(
        {
          role        : request.dtlsRole,
          fingerprint :
          {
            algorithm : request.remoteFingerprint.type,
            value     : request.remoteFingerprint.hash
          }
        })
        .then(() =>
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
});
```

Until the API is documented, the output of the included test units may help a bit understanding how the API will look like:

```bash
ibc in ~/src/mediasoup $ gulp test

Using gulpfile ~/src/mediasoup/gulpfile.js
Starting 'test'...

test/test_mediasoup.js
  mediasoup.Server() with no options must succeed
    ✓ server must close cleanly

  mediasoup.Server() with valid options must succeed
    ✓ server must close cleanly

  mediasoup.Server() with wrong options must fail
    ✓ server must close with error

  mediasoup.Server() with non existing `rtcListenIPv4` IP must fail
    ✓ server must close with error

  mediasoup.Server() with too narrow RTC ports range must fail
    ✓ server must close with error

test/test_Server.js
  server.updateSettings() with no options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with valid options must succeed
    ✓ server.updateSettings() succeeded

  server.updateSettings() with invalid options must fail
    ✓ server.updateSettings() failed: Error: invalid value 'chicken' for logLevel

  server.updateSettings() in a closed server must fail
    ✓ server.updateSettings() failed: InvalidStateError: Server closed
    ✓ server.updateSettings() must reject with InvalidStateError

  server.Room() must succeed
    ✓ room must close cleanly

  server.Room() in a closed server must fail
    ✓ server.Room() must throw InvalidStateError

  server.dump() must succeed
    ✓ server.dump() succeeded
    ✓ server.dump() must retrieve two workers

test/test_Room.js
  room.Peer() with `peerName` must succeed
    ✓ peer must close cleanly

  room.Peer() without `peerName` must fail
    ✓ room.Peer()) must throw TypeError

  room.Peer() with same `peerName` must fail
    ✓ room.Peer() must throw

  room.Peer() with same `peerName` must succeed if previous peer was closed before
    ✓ room.getPeer() must retrieve the first "alice"
    ✓ room.getPeers() must retrieve one peer
    ✓ room.getPeer() must retrieve nothing
    ✓ room.getPeers() must retrieve zero peers
    ✓ room.getPeer() must retrieve the new "alice"
    ✓ room.getPeers() must retrieve one peer
    ✓ peer must close cleanly

  room.dump() must succeed
    ✓ room.dump() succeeded
    ✓ room.dump() must retrieve two peers

test/test_Peer.js
  peer.createTransport() with no options must succeed
    ✓ peer.createTransport() succeeded
    ✓ peer.dump() succeeded
    ✓ peer.dump() must retrieve one transport

  peer.createTransport() with no `udp` nor `tcp` must fail
    ✓ peer.createTransport() failed: Error: could not open any IP:port

  peer.RtpReceiver() with valid `transport` must succeed
    ✓ peer.createTransport() succeeded
    ✓ rtpReceiver.transport must retrieve the given `transport`
    ✓ peer.dump() succeeded
    ✓ peer.dump() must retrieve one rtpReceiver
    ✓ rtpReceiver must close cleanly

  peer.RtpReceiver() with a closed `transport` must fail
    ✓ peer.createTransport() succeeded
    ✓ peer.RtpReceiver() must throw InvalidStateError

test/test_Transport.js
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
    ✓ transport.setRemoteDtlsParameters() failed: Error: invalid `data.role`
    ✓ local DTLS `role` must be "auto"

  transport.setRemoteDtlsParameters() without `fingerprint` must fail
    ✓ peer.createTransport() succeeded
    ✓ transport.setRemoteDtlsParameters() failed: Error: missing `data.fingerprint`
    ✓ local DTLS `role` must be "auto"

  transport.close() must succeed
    ✓ peer.createTransport() succeeded
    ✓ transport must close cleanly
    ✓ `transport.iceState` must be "closed"
    ✓ `transport.dtlsState` must be "closed"
    ✓ peer.dump() succeeded
    ✓ peer.dump() must retrieve zero transports

test/test_RtpReceiver.js
  rtpReceiver.receive() with valid `rtpParameters` must succeed
    ✓ rtpReceiver.receive() succeeded
    ✓ rtpReceiver.dump() succeeded
    ✓ rtpReceiver.dump() must provide the expected `rtpParameters`

  rtpReceiver.receive() with no `rtpParameters` must fail
    ✓ rtpReceiver.receive() failed: Error: missing `codecs`

  rtpReceiver.close() must succeed
    ✓ rtpReceiver must close cleanly
    ✓ peer.dump() succeeded
    ✓ peer.dump() must retrieve zero rtpReceivers

test/test_RtpListener.js
  lalala
    ✓ first rtpReceiver.receive() succeeded
    ✓ second rtpReceiver.receive() succeeded
    ✓ both rtpReceiver.receive() succeeded
    ✓ transport.dump() succeeded
    ✓ transport.dump() must provide the given payload types
    ✓ transport.dump() must provide the given ssrc values

test/test_extra.js
  extra.fingerprintFromSDP()
    ✓ should be equal
    ✓ should be equal

  extra.fingerprintToSDP()
    ✓ should be equal
    ✓ should be equal

  82 passing (7s)
```


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)
