# Changelog

### NEXT

### 0.6.0

- Remove support for the "urn:ietf:params:rtp-hdrext:toffset" RTP extension (PR #1942).
- Fix `ScalabilityMode::ksvc()` returning `false` for `L2T1_KEY`.

### 0.5.0

- Worker: Use `int64_t` for bitrate everywhere (PR #1919).

### 0.4.0

- New built-in SCTP stack (PR #1806):
  - Remove `NumSctpStreams` type.
  - Add `SctpNegotiatedCapabilities` type.

### 0.3.0

- `RtpHeaderExtensionUri`: Add `SsrcAudioLevel`, `AbsSendTime`, `TransportWideCcDraft01`, `DependencyDescriptor`, `AbsCaptureTime`, `PlayoutDelay` and `MediasoupPacketId` variants. Rename `AudioLevel` to `SsrcAudioLevel` (PR #1631).
- `RtpParameters`: Add optional `msid` field (WebRTC MediaStream Identification, RFC 8830) (PR #1634).

### 0.2.1

- Initial release as a standalone crate extracted from `mediasoup` (PR #1572).
