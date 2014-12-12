# MediaSoup

Flexible and powerful multi-stream media server ready for [WebRTC](http://www.webrtc.org/) and other RTP based technologies.


## IMPORTANT!

* Not ready yet. Work must be done on the *ControlProtocol* and the high level API in order to make MediaSoup usable. Working on it.


## Features

* Supports *WebRTC* media requirements ([ICE-Lite](http://tools.ietf.org/html/rfc5245), [DTLS-SRTP](http://tools.ietf.org/html/rfc5764), [rtcp-mux](http://tools.ietf.org/html/rfc5761), [bundle](http://tools.ietf.org/html/draft-ietf-mmusic-sdp-bundle-negotiation)) along with classic VoIP technologies ([SDES-SRTP](http://tools.ietf.org/html/rfc4568), plain [RTP/RTCP](http://tools.ietf.org/html/rfc3550)).
* ICE/DTLS/RTP/RTCP over UDP and TCP.
* Multi participant real-time sessions.
* IPv6 ready.
* Signaling agnostic. MediaSoup does not deal with SIP or other signaling protocols. In the other side, it can deal with *any* signaling protocol.
* SDP unaware. Really, MediaSoup will not parse a SDP for you.
* Just media. MediaSoup cares about media processing so it relies on an external application which deals with your preferred signaling protocol and manages MediaSoup by using the *MediaSoup ControlProtocol*.
* Extremely powerful. MediaSoup is coded in C++ on top of the awesome [libuv](https://github.com/libuv/libuv) asychronous I/O library. MediaSoup is a single process application with multiple *Worker* threads for media handling. MediaSoup takes full advantage of your CPU capabilities.


## Dependencies

* [libuv](https://github.com/libuv/libuv): Cross-platform asychronous I/O C library. Version ~1.0.2 required.
* [libconfig](http://www.hyperrealm.com/libconfig/): C/C++ library for processing configuration files. Version ~1.4.8 required.
* [openssl](https://www.openssl.org). Version >=1.0.1 required.
* [libsrtp](https://github.com/cisco/libsrtp). Version >=1.5.0 required.


## Requirements

MediaSoup runs in any Posix system (tested in Linux Debian 7 "Wheezy" and OSX 10.9 "Mavericks") in both 32 or 64 bits architectures (Big or Little Endian).

MediaSoup requires *C++11* and other features available in *gcc* >= 4.7 or *clang* >= 5.


## Installation

* Install dependencies (see above).
* Compile MediaSoup:
```
$ make
```

*TODO:* `make install`, `make test`, etc still missing.


## Running MediaSoup

The `mediasoup` binary is placed at the `bin/` directory. For help on its usage run it with `-h` or `--help`:

```
$ ./bin/mediasoup --help

MediaSoup v0.0.1
Copyright (c) 2014 Iñaki Baz Castillo | https://github.com/ibc

Usage: mediasoup [options]
Options:
  -c, --configfile FILE     Path to the configuration file
  -d, --daemonize           Run in daemon mode
  -p, --pidfile FILE        Create a PID file (requires daemon mode)
  -u, --user USER           Run with the given system user
  -g, --group GROUP         Run with the given system group
  -v, --version             Show version
  -h, --help                Show this message
```

An example configuration file is provided at `etc/mediasoup.default.cfg`. Rename it to `mediasoup.cfg` and edit it as per your needs.


### Reloading MediaSoup

Once it is running MediaSoup configuration can be reload by sending a *SIGUSR1* signal to the process:
```
$ kill -USR1 $(pidof mediasoup)
```

NOTE: Just certain options in the configuration file are reloaded (currently just *logLevel*).


## Documentation

[MediaSoup Documentation](docs/index.md) is placed in the `docs/` folder.


## License

Not decided yet. For now "All Rights Reserved" (will be changed when ready).
