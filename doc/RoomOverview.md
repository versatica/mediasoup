# Room Overview

This documentation details how *mediasoup* rooms are created, how peers are added to the room, how `RtpReceiver` and `RtpSender` instances are created/generated, and how RTP parameters are handled.


## Creating a Room

A `Room` is created by calling `server.createRoom(roomOptions)` where `roomOptions` is a mandatory object that must include a `mediaCodecs` array of media codecs supported by the room.

Each media codec must define mandatory attributes:

* `kind`: "audio"/"video"/"depth" or empty string which means all the kinds
* `name`: "audio/opus", etc
* `clockRate`: 48000, etc

Media codecs can also define some optional fields such as:

* `payloadType`: if not set, a random one is assigned
* `numChannels`: defaults to 1 except for Opus in which it defaults to 2
* `parameters`: needed for defining the `packetizationMode` for H264, etc
* `maxptime`
* `ptime`

Available media codec names are:

* "audio/opus"
* "audio/PCMA"
* "audio/PCMU"
* "audio/ISAC"
* "audio/G722"
* "video/VP8"
* "video/VP9"
* "video/H264"
* "video/H265"

Complementary codecs are also considered media codecs. These are:

* "audio/CN"
* "audio/telephone-event"

Feature codecs ("video/rtx", "video/ulpfec", "video/flexfec" and "video/red") are **NOT** media codecs and hence MUST NOT be placed into `roomOptions.mediaCodecs` (they will be ignored if present).

