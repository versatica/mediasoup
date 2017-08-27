# TODO in mediasoup v2 (server-side)


* Before the Producer dispacthes a RTP packet to the room (via `onRtpPacket()`, we call `ApplyRtpMapping(packet)` to mangle RTP payload type and header extension ids. However, we are breaking the previously established Id mapping in the `RtpPacket` (generated via `packet->AddExtensionMapping()`). The fact is that, most probably, we should set the mapping once the `RtpStreamRecv` processes the packet. The problem is that, currently, the information to set the mapping is hold into the `RtpStreamRecv`...

* May be problems with `RtpPacket` RTX stuff due to RTP mapping in Producers? Sure, the decoded RTX packet must also be mapped (once decoded).

* Use "@xxxxx" for private events and `safeEmit("xxxx")` for public events (already done except for Producer and Consumer).

* If the transport of a consumer is closed, and the consumer was "enabled", it should become "disabled" and emit event? I think yes. Enabled should mean "it has a transport". Period.

* May have to react on DTLS ALERT CLOSE in the server and make it "really" close the Transport and notify the client. Bufff... I don't like this...

* Must be ready for the case in which the client closes a `Transport` on which a `Producer` was running. Isn't it?

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).

* Since a `Producer` may receive RTP (and RTCP) with ssrc, pt and headerExtensionId different that those in the room, we must ready to also map consumer/generated/relayed RTCP.



## TODO per JS class

### Room

* Add `mandatoryCodecPayloadTypes`.
* Use/store `appData` when creating a `Peer`.
