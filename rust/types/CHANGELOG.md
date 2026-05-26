# Changelog

### NEXT

- New built-in SCTP stack (PR #1806):
  - Remove `NumSctpStreams` type.
  - Add `SctpNegotiatedCapabilities` type.

### 0.3.0

- `RtpHeaderExtensionUri`: Add `SsrcAudioLevel`, `AbsSendTime`, `TransportWideCcDraft01`, `DependencyDescriptor`, `AbsCaptureTime`, `PlayoutDelay` and `MediasoupPacketId` variants. Rename `AudioLevel` to `SsrcAudioLevel` (PR #1631).
- `RtpParameters`: Add optional `msid` field (WebRTC MediaStream Identification, RFC 8830) (PR #1634).

### 0.2.1

- Initial release as a standalone crate extracted from `mediasoup` (PR #1572).
