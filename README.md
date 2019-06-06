# mediasoup v3

[![][mediasoup-banner]][mediasoup-website]

[![][npm-shield-mediasoup]][npm-mediasoup]
[![][travis-ci-shield-mediasoup]][travis-ci-mediasoup]
[![][codacy-grade-shield-mediasoup]][codacy-grade-mediasoup]
[![][opencollective-shield-mediasoup]][opencollective-mediasoup]


## Website and Documentation

* [mediasoup.org][mediasoup-website]


## Support Forum

* [mediasoup.discourse.group][mediasoup-discourse]


## Design Goals

mediasoup and its client side libraries are designed to accomplish with the following goals:

* Be a [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Support both WebRTC and plain RTP input and output.
* Be a Node.js module in server side.
* Be a tiny JavaScript and C++ libraries in client side.
* Be minimalist: just handle the media layer.
* Be signaling agnostic: do not mandate any signaling protocol.
* Be super low level API.
* Support all existing WebRTC endpoints.
* Enable integration with well known multimedia libraries/tools.


## Architecture

![][mediasoup-architecture]


## Use Cases

mediasoup and its client side libraries provide a super low level API. They are intended to enable different use cases and scenarios, without any constraint or assumption. Some of these use cases are:

* Group video chat applications.
* One-to-many (or few-to-many) broadcasting applications in real-time.
* RTP streaming.


## Features

* ECMAScript 6 low level API.
* Multi-stream: multiple audio/video streams over a single ICE + DTLS transport.
* IPv6 ready.
* ICE / DTLS / RTP / RTCP over UDP and TCP.
* Simulcast and SVC support.
* Congestion control.
* Sender and receiver bandwidth estimation with spatial/temporal layers distribution algorithm.
* Extremely powerful (media worker subprocess coded in C++ on top of [libuv](https://libuv.org)).


## Demo Online

[![][mediasoup-demo-screenshot]][mediasoup-demo]

Try it at [v3demo.mediasoup.org](https://v3demo.mediasoup.org) ([source code](https://github.com/versatica/mediasoup-demo)).


## Authors

* Iñaki Baz Castillo [[website](https://inakibaz.me)|[github](https://github.com/ibc/)]
* José Luis Millán [[github](https://github.com/jmillan/)]


## Sponsors

* [46 Labs LLC](https://46labs.com): Special thanks to 46 Labs where we both, José Luis and Iñaki, develop a mediasoup based multiparty video conferencing app focused on the enterprise market for more than a year.


## Donate

You can support mediasoup by making a donation in [open collective][opencollective-mediasoup]. Thanks!


## License

[ISC](./LICENSE)




[mediasoup-banner]: /art/mediasoup-banner.png
[mediasoup-website]: https://mediasoup.org
[mediasoup-discourse]: https://mediasoup.discourse.group
[npm-shield-mediasoup]: https://img.shields.io/npm/v/mediasoup.svg
[npm-mediasoup]: https://npmjs.org/package/mediasoup
[travis-ci-shield-mediasoup]: https://travis-ci.com/versatica/mediasoup.svg?branch=master
[travis-ci-mediasoup]: https://travis-ci.com/versatica/mediasoup
[codacy-grade-shield-mediasoup]: https://img.shields.io/codacy/grade/3c8b9efc83674b6189707ab4188cfb2b.svg
[codacy-grade-mediasoup]: https://www.codacy.com/app/versatica/mediasoup
[opencollective-shield-mediasoup]: https://img.shields.io/opencollective/all/mediasoup.svg
[opencollective-mediasoup]: https://opencollective.com/mediasoup/donate
[mediasoup-architecture]: /art/mediasoup-v3-architecture-01.svg
[mediasoup-demo-screenshot]: /art/mediasoup-v3.png
[mediasoup-demo]: https://v3demo.mediasoup.org





