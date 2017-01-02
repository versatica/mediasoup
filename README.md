[![][mediasoup-banner]][mediasoup-website]

[![][npm-shield-mediasoup]][npm-mediasoup]
[![][travis-ci-shield-mediasoup]][travis-ci-mediasoup]


**Status:** v1.0.0 coming soon.


## Website and documentation

* [mediasoup.org][mediasoup-website]


## Donate

You can support **mediasoup** by making a donation [here][paypal-url]. Thanks!



## Installation

```bash
$ npm install mediasoup --save
```


## Design goals

* Be a Node.js module
* Be minimalist: just handle the media layer
* Expose a modern ECMAScript 6 [API](/api/) in sync with [ORTC](http://ortc.org/)
* Work with current [WebRTC](https://webrtc.org) client implementations


## Features

* Multiple conference rooms with multiple participants
* IPv6 ready
* ICE / DTLS / RTP / RTCP over UDP and TCP
* Extremely powerful (media handler subprocess coded in C++ on top of [libuv](http://libuv.org))
* Can handle RTP packets in JavaScript land


## Requirements

* Node.js >= `v4.0.0`
* POSIX based operating system (Windows not yet supported)
* `make`
* `gcc` or `clang` with C++11 support


## Author

Iñaki Baz Castillo ([@ibc](https://github.com/ibc/) at Github)


## License

[ISC](./LICENSE)




[mediasoup-banner]: https://raw.githubusercontent.com/ibc/mediasoup-website/master/_art/mediasoup_banner.png
[mediasoup-website]: https://mediasoup.org
[travis-ci-shield-mediasoup]: https://img.shields.io/travis/ibc/mediasoup/master.svg
[travis-ci-mediasoup]: http://travis-ci.org/ibc/mediasoup
[npm-shield-mediasoup]: https://img.shields.io/npm/v/mediasoup.svg
[npm-mediasoup]: https://npmjs.org/package/mediasoup
[paypal-url]: https://paypal.me/inakibazcastillo/100
