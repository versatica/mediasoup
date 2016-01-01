# mediasoup

Powerful [WebRTC](http://www.webrtc.org/) SFU ("Selective Forwarding Unit") server built on Node and C++.


## IMPORTANT!

* Not yet ready, working on it.


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


## Features

* Not a boring standalone server, but a Node library exposing a pure JavaScript API.
* Supports *WebRTC* media requirements ([ICE-Lite](http://tools.ietf.org/html/rfc5245), [DTLS-SRTP](http://tools.ietf.org/html/rfc5764), [rtcp-mux](http://tools.ietf.org/html/rfc5761), [bundle](http://tools.ietf.org/html/draft-ietf-mmusic-sdp-bundle-negotiation)) along with classic VoIP technologies ([SDES-SRTP](http://tools.ietf.org/html/rfc4568), plain [RTP/RTCP](http://tools.ietf.org/html/rfc3550)).
* ICE/DTLS/RTP/RTCP over UDP and TCP.
* Multi participant real-time sessions.
* IPv6 ready.
* Signaling agnostic. **mediasoup** does not deal with SIP or other signaling protocols. In the other side, it can deal with *any* signaling protocol.
* SDP unaware. Really, **mediasoup** will not parse a SDP for you.
* Extremely powerful. Media handler subprocess (*mediasoup-worker*) is coded in C++ on top of the awesome [libuv](https://github.com/libuv/libuv) asychronous I/O library. **mediasoup** takes full advantage of your CPU capabilities by launching as many workers as needed.


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)
