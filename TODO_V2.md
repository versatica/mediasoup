# TODO in mediasoup v2 (server-side)

* Set a proper value for Producer::FullFrameRequestBlockTimeout (currently 1 second).

* Should RtpStream::InitSeq() reset `received` to 0? We do it but it may break RTCP reports in both RtpStreamRecv and RtpStreamSend.
  - NOTE: We have removed `received = 0` from `InitSeq()`, so we still send RTCP SenderReports on paused Consumers.
  - Also check whether expectedPrior and receivedPrior should just belong to RtpStreamRecv rather than RtpStream (yes).

* Add a `fullFrameRequestBlockTimer` and a `isFullFrameRequested` flag in `Producer`, so `Producer::RequestFullFrame()` check it and decides whether ask for a full frame to its streams or not.

* If a worker Consumer is paused (or sourcePaused), should the RTCP checks or report generations behave differently?

* Check `appData` everywhere.

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Move the RTCP timer from Peer to Transport.

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).

* Implement `mandatoryCodecPayloadTypes`.

* Check H246 parameters.

* JS: Remove tons of useless debugs (for example in `Peer._createProducer()`).
