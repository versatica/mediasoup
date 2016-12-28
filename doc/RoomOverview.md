# Room Overview

This documentation details how *mediasoup* rooms are created, how peers are added to the room, how `RtpReceiver` and `RtpSender` instances are created/generated, and how RTP parameters are managed.


## Creating a Room

A `Room` is created by calling `server.createRoom(roomOptions)` where `roomOptions` is a mandatory object that must include a `mediaCodecs` array of media codecs supported by the room.

Each media codec must define a `kind` ("audio"/"video"/"depth" or empty string which means all the kinds), a `name` ("audio/opus"), a `clockRate` (48000) and can define some optional fields such as `numChannels` (which defaults to 1 except for Opus in which it defaults to 2) and `parameters` (needed for defining the `packetizationMode` for H264).

Available media codecs are "opus", "PCMA", "PCMU", "ISAC", "G722", "VP8", "VP9", "H264" and "H265". Complementary codecs are also considered media codecs. These are "CN" and "telephone-event".

Feature codecs ("rtx", "ulpfec", "flexfec" and "red") are **NOT** media codecs.

