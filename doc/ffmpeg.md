# ffmpeg

Some stuff to make `ffmpeg` work with mediasoup v3.

Resources:

* [FFmpeg Protocols Documentation](https://ffmpeg.org/ffmpeg-protocols.html)
* [libopus encoding options in ffmpeg](http://ffmpeg.org/ffmpeg-codecs.html#libopus-1)
* [node-fluent-ffmpeg](https://github.com/fluent-ffmpeg/node-fluent-ffmpeg) - Unmaintained, but interesting.


## Sending a MP3 file encoded in Opus

Something like this:

```bash
$ ffmpeg \
  -re -f mp3 -i audio.mp3 -acodec libopus -ab 128k -ac 2 -ar 48000 \
  -ssrc 666 -payload_type 111 \
  -f rtp "rtp://1.2.3.4:1234?rtcpport=1234&localrtpport=10000&localrtcpport=10001"
```

The command produces this SDP output, which represents the **remote** SDP:

```
v=0
o=- 0 0 IN IP4 127.0.0.1
s=No Name
c=IN IP4 1.2.3.4
t=0 0
a=tool:libavformat 58.20.100
m=audio 1234 RTP/AVP 111
b=AS:128
a=rtpmap:111 opus/48000/2
a=fmtp:111 sprop-stereo=1
```

Commandline options and URL query parameters mean (some of them):

* `-ssrc 666`: Send RTP packets with SSRC 666 (great!).
* `-payload_type 111`: Use 111 as payload type (great!).
* `rtp://1.2.3.4:1234`:  Send RTP to IP 1.2.3.4 and port 1234.
* `?rtcpport=1234`: Send RTCP to port 1.2.3.4 (it works, great!).
* `?localrtpport=10000`: Use 10000 as RTP source port (great!).
* `?localrtcpport=10001`: Use 10001 as RTCP source port (it cannot be 10000, sad).

Notes:

* `ffmpeg` can send RTP and RTCP to the same port, which is great, but it cannot listen for RTP and RTCP in the same local port. So we must disable `rtcp-mux` in the `PlainRtpTransport` and send RTP and RTCP to different destination ports.


## Using ffmpeg to send audio to mediasoup

Let's assume that we want to read an mp3 file (located in the server) and send it to the SFU. In mediasoup v3 it would be something as follows:

* Decide which IP, RTP port and RTCP port we are gonna use in `ffmpeg`:
  - IP: 127.0.0.1
  - RTP port: 2000
  - RTCP port: 2001

* Decide which codec, SSRC and PT (payload type) we are gonna send:
  - Codec: Opus
  - SSRC: 12345678
  - PT: 100

* Create a `PlainRtpTransport` in mediasoup:

```js
const transport = await router.createPlainTransport(
  { 
    ip       : '127.0.0.1',
    rtcpMux  : false
  });

// Read transport local RTP and RTCP ports.
const rtpPort = transport.tuple.localPort; // => For example 3301.
const rtcpPort = transport.rtcpTuple.localPort; // => For example 4502.

// Provide ffmpeg IP/ports.
await transport.connect(
  {
    ip       : '127.0.0.1',
    port     : 2000, 
    rtcpPort : 2001
  });
```

* Create a `Producer` in such a transport:

```js
const producer = await transport.produce(
  {
    rtpParameters :
    {
      codecs :
      [
        {
          name         : 'opus',
          mimeType     : 'audio/opus',
          clockRate    : 48000,
          payloadType  : 100,
          channels     : 2,
          rtcpFeedback : [],
          parameters   :
          {
            useinbandfec : 1 // NOTE: Not sure if ffmpeg allows this for Opus.
          }
        }
      ],
      encodings :
      [
        { ssrc: 12345678 }
      ]
    }
  });
```

* Tell `ffmpeg` to send the audio file with the parameters defined above:

```bash
$ ffmpeg \
  -re -f mp3 -i audio.mp3 -acodec libopus -ab 128k -ac 2 -ar 48000 \
  -ssrc 12345678 -payload_type 100 \
  -f rtp "rtp://127.0.0.1:3301?rtcpport=4502&localrtpport=2000&localrtcpport=2001"
```

**NOTE:** The above JS code (`router.createPlainTransport()` and `transport.produce()`) could be included into an HTTP API server running in the same Node.js in which mediasoup runs. So the ffmpeg "client" could be easily wrapped into a Shell script that calls some `curl` commands followed by the `ffmpeg` command with the retrieved parameters.


## TODO

* What about sending a mp4 file with both audio and video? Can `ffmpeg` be told to send the audio track and the video track to different IPs and ports with separate SSRC and PT values in the commandline, etc?
