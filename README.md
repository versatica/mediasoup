![mediasoup](https://raw.githubusercontent.com/ibc/mediasoup/master/art/MediaSoup-logo.png)

# MediaSoup

Flexible and powerful Node.js library handling a  C++ multi-stream media server ready for [WebRTC](http://www.webrtc.org/) and other RTP based technologies.


## IMPORTANT!

* Working on it!


## Features

* Supports *WebRTC* media requirements ([ICE-Lite](http://tools.ietf.org/html/rfc5245), [DTLS-SRTP](http://tools.ietf.org/html/rfc5764), [rtcp-mux](http://tools.ietf.org/html/rfc5761), [bundle](http://tools.ietf.org/html/draft-ietf-mmusic-sdp-bundle-negotiation)) along with classic VoIP technologies ([SDES-SRTP](http://tools.ietf.org/html/rfc4568), plain [RTP/RTCP](http://tools.ietf.org/html/rfc3550)).
* ICE/DTLS/RTP/RTCP over UDP and TCP.
* Multi participant real-time sessions.
* IPv6 ready.
* Signaling agnostic. MediaSoup does not deal with SIP or other signaling protocols. In the other side, it can deal with *any* signaling protocol.
* SDP unaware. Really, MediaSoup will not parse a SDP for you.
* Extremely powerful. MediaSoup is coded in C++ on top of the awesome [libuv](https://github.com/libuv/libuv) asychronous I/O library. MediaSoup is a single process application with multiple *Worker* threads for media handling. MediaSoup takes full advantage of your CPU capabilities.


## Worker Building

```bash
$ cd worker
$ make submodules
$ make
```


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)
