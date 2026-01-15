# TODO RTP

- `RtpProbationGenerator.cpp`: Remove the `packet->Dump()` and `MS_DUMP()` (added to see if the new padding variable mechanism works as pexpected).

- `Transport.cpp`: Remove `packet->Dump()` in `OnTransportCongestionControlClientSendRtpPacket()`.

- We must add `const` in many `RTP::Packet* packet` arguments to be sure that those methods cannot modify the packet.

- Tema throw a saco en Packet. Hacer try/catch.

- Mirar el tamaño del RTP::Packet al meterlo en el retransmission buffer porque podría petar al hacer le clone. O sea, `RTP/SharedPacket` stuff y try/catch.

- `RTP/TestPacket.cpp`: Write tests for the new `Parse()` method with 3 args.

- `TestRtpStreamSend.cpp`: Fallan tests, ` rtxPacket->GetSequenceNumber()` devuelve valores aleatorios!. He comentado algunos tests y añadido `printf()` para debugging. Además hay cosas muy raras, ¿por qué los paquetes que devuelve `OnRtpStreamRetransmitRtpPacket()` tienen `header extension: id:0, value length:0` (o sea, que tienen Header Extension pero en plan guarro y sin value:

  ```
  --- packet1 orig:
  <RTP::Packet>
    length: 12 (buffer length: 12)
    sequence number: 21006
    timestamp: 1533790901
    marker: false
    payload type: 123
    ssrc: 2
    csrcs: false
    payload length: 0
    padding length: 0
  </RTP::Packet>

  ------ OnRtpStreamRetransmitRtpPacket:
  <RTP::Packet>
    length: 12 (buffer length: 112)
    sequence number: 21006
    timestamp: 1533790901
    marker: false
    payload type: 9
    ssrc: 1111
    csrcs: false
    header extension: id:0, value length:0
    payload length: 18446744073709551525
    padding length: 87
  </RTP::Packet>
  ```
