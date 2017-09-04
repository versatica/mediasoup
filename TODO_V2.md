# TODO in mediasoup v2 (server-side)

* Must avoid passing JSON data to Transport/Producer/Consumer instance methods. Instead, create API methods that receive "nice" C++ params and do all the JSON decode work in `Router::HandleRequest()`.

* Must mangle RTP sequence numbers in Consumers:
  - After paused.
  - After the Producer gets `updateRtpParameters()` called (so a `OnProducerRtpParametersUpdated` listener is required in Producer for the Router to call `SourceRtpParametersUpdated()` on all its Concumers).
  - NOTE: We may also need to mangle RTP timestamps!

* After `updateProducer` (which changes receiving SSRCs), should we do something in Transport between `producer->ReceiveRtpPacket(packet)` and `this->remoteBitrateEstimator->IncomingPacket`?

* Set a proper value for Producer::FullFrameRequestBlockTimeout (currently 1 second).

* Should RtpStream::InitSeq() reset `received` to 0? We do it but it may break RTCP reports in both RtpStreamRecv and RtpStreamSend.
  - NOTE: We have removed `received = 0` from `InitSeq()`, so we still send RTCP SenderReports on paused Consumers.
  - Also check whether expectedPrior and receivedPrior should just belong to RtpStreamRecv rather than RtpStream (yes).

* Check `appData` everywhere.

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.
  - NOTE: It's updated (manually). Should document in BUILDING.md how to automate it.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Move the RTCP timer from Peer to Transport.

* Implement `mandatoryCodecPayloadTypes`.

* JS: Remove tons of useless debugs (for example in `Peer._createProducer()`).

* Producer JS API to request fullframe.
