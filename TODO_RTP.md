# TODO RTP

- `RtpProbationGenerator.cpp`: Remove the `packet->Dump()` and `MS_DUMP()` (added to see if the new padding variable mechanism works as pexpected).

- `Transport.cpp`: Remove `packet->Dump()` in `OnTransportCongestionControlClientSendRtpPacket()`.

- We must add `const` in many `RTP::Packet* packet` arguments to be sure that those methods cannot modify the packet.

- Tema throw a saco en Packet. Hacer try/catch.

- Mirar el tamaño del RTP::Packet al meterlo en el retransmission buffer porque podría petar al hacer le clone. O sea, `RTP/SharedPacket` stuff y try/catch.

- When in the demo, I see tons of calls to `Packet::SetSequenceNumber()` with same seq value!
