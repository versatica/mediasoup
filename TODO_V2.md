# TODO in mediasoup v2 (server-side)

* In `RtpStreamSend::RequestRtpRetransmission()` we may have to request a key frame (new listener needed for that) if we couldn't resend all the requested packets.

* In `RtpStreamSend::StorePacket()` we should remove old packets (as `NackGenerator` does).

* Remove `producer.on('rtprawpacket')` and, instead, create a special `RtpConsumer` or something like that.

* Set a proper value for Producer::FullFrameRequestBlockTimeout (currently 1 second).

* Check `appData` everywhere.

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.
  - NOTE: It's updated (manually). Should document in BUILDING.md how to automate it.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Implement `mandatoryCodecPayloadTypes`.

* JS: Remove tons of useless debugs (for example in `Peer._createProducer()`).

* Producer JS API to request fullframe.
