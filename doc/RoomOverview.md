# Room Overview

This documentation details how *mediasoup* rooms are created, how peers are added to the room, how `RtpReceiver` and `RtpSender` instances are created/generated, and how RTP parameters are handled.


## Creating a Room

A `Room` is created by calling `server.createRoom(roomOptions)` where `roomOptions` is a mandatory object that must include a `mediaCodecs` array of media codecs supported by the room.

Each media codec must define some mandatory fields:

* `kind`: "audio"/"video"/"depth" or empty string which means all the kinds
* `name`: "audio/opus", etc
* `clockRate`: 48000, etc

Media codecs can also define some optional fields such as:

* `payloadType`: if not set, a random one is assigned
* `numChannels`: defaults to 1 except for Opus in which it defaults to 2
* `parameters`: needed for defining the `packetizationMode` for H264, etc
* `maxptime`
* `ptime`

Feature codecs ("video/rtx", "video/ulpfec", "video/flexfec" and "video/red") are **NOT** media codecs and hence MUST NOT be placed into `roomOptions.mediaCodecs` (they will be ignored if present).

Given media codecs are matched to the media codec configurations supported by mediasoup (check them at [lib/supportedRtpCapabilities.js](../lib/supportedRtpCapabilities.js)). No matching codecs are ignored.

At the end, the room `RtpCapabilities` object is a clone of the mediasoup's supported capabilities with the matching subset of given room media codecs.

And the room is done.


## Creating a Peer

A `Peer` is created by calling `room.Peer(name)`.

Once created, peer's capabilities must be set (otherwise no `RtpReceiver` can be created) by calling `peer.setCapabilities(capabilities)`. Those capabilities include codecs, RTP header extensions and FEC mechanisms.

These codecs have the same fields as the room codecs, with some variations:

* `payloadType` is ignored (it will be mapped to the same room codec).
* `rtcpFeedback` is optional.

After that, C++ `Room::onPeerCapabilities(capabilities)` is called. This method removes peer's codecs not supported by the room, removes the unsupported RTCP feedback mechanisms from the supported codecs, and removes RTP header extensions and FEC mechanisms not supported by mediasoup.

This is, peer's capabilities are a subset of the mediasoup capabilities and the room capabilities.


## Creating a RtpReceiver

A `RtpReceiver` is created by calling `peer.RtpReceiver(kind, transport)` where `kind` can be "audio", "video" or "depth", and `transport` must be a `Transport` instance previously created by the peer.

Once created, `rtpReceiver.receive(parameters)` must be called (otherwise the `RtpReceiver` is ignored by the room. `parameters` is a `RtpParameters` object which the following fields:

* `muxId` is optional.
* `codecs` is mandatory.
* `encodings` is optional.
* `rtcp` is optional.
* `userParameters` is an optional object.

These codecs have the same fields as the peer's capabilities codecs above, with some variations:

* `kind` is ignored.
* `payloadType` MUST match the same value as the one set by the room (otherwise it will fail).

If `receive()` is called with a codec that is not supported by the room or not present in the peer's given capabilities, it fails.

Unsupported RTCP feedback mechanisms and RTP header extensions not supported by mediasoup or by the peer itself are removed.

Effective RTP parameters are returned to the `receive()` promise.

`Room::onPeerRtpReceiverParameters()` creates `RtpSenders` associated to this `RtpReceiver` (for every other `Peer` with its capabilities already set) by calling `Peer::AddRtpSender()` on them.

`Peer::AddRtpSender()` takes the associated `RtpParameters` of the `RtpReceiver` as parameter and call `RtpSender::Send(parameters)`.

`RtpSender::Send(parameters)` clones the given parameters and removes non supported codecs and non supported RTP header extensions.

*TODO:* This must be analyzed.
