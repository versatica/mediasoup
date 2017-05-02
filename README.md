[![][mediasoup-banner]][mediasoup-website]

[![][npm-shield-mediasoup]][npm-mediasoup]
[![][travis-ci-shield-mediasoup]][travis-ci-mediasoup]
[![][codacy-grade-shield-mediasoup]][codacy-grade-mediasoup]


## Website and documentation

* [mediasoup.org][mediasoup-website]


## Demo online

![mediasoup-demo-screenshot]

* Try it at [demo.mediasoup.org](https://demo.mediasoup.org) ([source code](https://github.com/versatica/mediasoup-demo)).


## Donate

You can support **mediasoup** by making a donation [here][paypal-url]. Thanks!


## Installation

Within your Node.js application:

```bash
$ npm install mediasoup --save
```

Prior to that, ensure your host satisfies the following **requirements**:

* Node.js >= `v4.8.0`
* POSIX based operating system (Windows not yet supported)
* Python 2 (`python2` or `python` command must point to the Python 2 executable)
* `make`
* `gcc` and `g++`, or `clang`, with C++11 support

*NOTE:* In Debian and Ubuntu install the `build-essential` package. It includes both `make` and `gcc`/`g++`.


## Design goals

* Be a WebRTC [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Be a Node.js module.
* Be minimalist: just handle the media layer.
* Expose a modern ECMAScript 6 [API](https://mediasoup.org/api/) in sync with [WebRTC 1.0](https://w3c.github.io/webrtc-pc/) and [ORTC](http://ortc.org/).
* Work with current WebRTC client implementations.


## Features

* Multiple conference rooms with multiple participants.
* Multi-stream over a single (BUNDLE) transport (Plan-B and Unified-Plan).
* IPv6 ready.
* ICE / DTLS / RTP / RTCP over UDP and TCP.
* Congestion control via [REMB](https://tools.ietf.org/html/draft-alvestrand-rmcat-remb).
* Extremely powerful (media worker subprocess coded in C++ on top of [libuv](http://libuv.org)).
* Can handle RTP packets in JavaScript land.


## Authors

* Iñaki Baz Castillo [[website](https://inakibaz.me)|[github](https://github.com/ibc/)]
* José Luis Millán [[github](https://github.com/jmillan/)]


## License

[ISC](./LICENSE)




[mediasoup-banner]: https://raw.githubusercontent.com/versatica/mediasoup-website/master/_art/mediasoup_banner.png
[mediasoup-demo-screenshot]: https://raw.githubusercontent.com/versatica/mediasoup-website/master/_art/mediasoup-opensips-summit-2017.jpg
[mediasoup-website]: https://mediasoup.org
[travis-ci-shield-mediasoup]: https://img.shields.io/travis/versatica/mediasoup/master.svg
[travis-ci-mediasoup]: http://travis-ci.org/versatica/mediasoup
[npm-shield-mediasoup]: https://img.shields.io/npm/v/mediasoup.svg
[npm-mediasoup]: https://npmjs.org/package/mediasoup
[codacy-grade-shield-mediasoup]: https://img.shields.io/codacy/grade/3c8b9efc83674b6189707ab4188cfb2b.svg
[codacy-grade-mediasoup]: https://www.codacy.com/app/versatica/mediasoup
[paypal-url]: https://paypal.me/inakibazcastillo/100
