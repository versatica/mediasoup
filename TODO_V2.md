# TODO in mediasoup v2 (server-side)

* `RtpStreamer` should have its own RTP capabilities rather than using the ones from the room.

* Instead of using `static_cast<size_t>(number)` use `size_t{ number }`. NOTE: Investigate it.

* Set a proper value for `Producer::FullFrameRequestBlockTimeout` (currently 1 second).

* Check `appData` everywhere.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Implement `mandatoryCodecPayloadTypes`.

* JS: Remove tons of useless debugs (for example in `Peer._createProducer()`).

* Update `compile_commands_template.json`.
