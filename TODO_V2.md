# TODO in mediasoup v2 (server-side)

* If a Producer is closed, those Peers non having a compatible Consumer (due to codecs) won't receive any notification. This is because the "close" event comes from the worker Consumer (which just exists upon succesful `enableConsumer`).
  - So it seems that we should create the Consumer in C++ always, being disabled at the beginning.

* When a video Consumer is paused and later resumed, it requests a Full Frame to the Producer. The sending browser does receive the PLI and (obviously) sends a full frame. The problem is that the `RtpStreamSend` of the Consumer ignores that full frame due to "bad sequence number":

```
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- paused !!! +2ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- paused !!! +1ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- paused !!! +0ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::HandleRequest() | ---- CONSUMER REQUESTING FULL FRAME ---- +0ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::HandleRequest() | ---- CONSUMER REQUESTING FULL FRAME ---- +0ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +18ms
mediasoup:WARN:mediasoup-worker [id:vdcuccoq#1] RTC::RtpStream::UpdateSeq() | bad sequence number, ignoring packet [ssrc:1945391646, seq:13041] +19s
mediasoup:WARN:mediasoup-worker [id:vdcuccoq#1] RTC::RtpStream::ReceivePacket() | invalid packet [ssrc:1945391646, seq:13041] +0ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +1ms
mediasoup:WARN:mediasoup-worker [id:vdcuccoq#1] RTC::RtpStream::UpdateSeq() | too bad sequence number, re-syncing RTP [ssrc:1945391646, seq:13042] +1ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +11ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +1ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +0ms
mediasoup:ERROR:mediasoup-worker [id:vdcuccoq#1] RTC::Consumer::SendRtpPacket() | ---- ok go go RTP :) +0ms
```

In fact, if the `RtpStreamSend` does not return false, the video automatically works, so we must refactor the `RtpStream::UpdateSeq()` method.

* Add a `fullFrrameRequestBlockTimer` and a `isFullFrameRequested` flag in `Producer`, so `Producer::RequestFullFrame()` check it and decides whether ask for a full frame to its streams or not.

* If a worker Consumer is paused (or sourcePaused), should the RTCP checks or report generations behave differently?

* Check `appData` everywhere.

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Move the RTCP timer from Peer to Transport.

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).

* Implement `mandatoryCodecPayloadTypes`.

* Check H246 parameters.
