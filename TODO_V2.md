# TODO in mediasoup v2 (server-side)

* Redo the compile_commands_template.json.

* worker: All the RTC emthods in Producer and Consumer do a `MS_ASSERT(Enabled();` (meaning that they already have `transport` and `rtpParameters`). Producers/Consumers without them are not present in the Router `mapProducerConsumers`/`mapConsumerProducer` maps. However *THEY DO EXIST* in the Sets of `producers` and `consumers` of the Transport.
  - So: before calling any RTC method on a Producer/Consumer, the Transport must check `producer/consumer->IsEnabled()` and ignore if not.

* Remove all the "close" events in C++. May be leave the `transport.on('close')`.

* Move the RTCP timer from Peer to Transport.

* `then()` of Promises is async so if we create an instance (Peer, Producer, Transport, etc) within the `channel.request().then(....)` and an event happens for that instance immediately, such an event may be lost.
  - The solution is creating the instance before calling `channel.request()`.
  - DOC: https://stackoverflow.com/questions/36726890/why-are-javascript-promises-asynchronous-when-calling-only-synchronous-functions
    + I must avoid unnecesary `Promise.resolve()`.
  - "The callback passed to a Promise constructor is always called synchronously, but the callbacks passed into then are always called asynchronously"

* worker: If a `Producer` is paused and a new `Peer` joins (so a new `Consumer` is generated) such a new `Consumer` does not know that its associated `Producer` is paused. When creating a `Consumer` the `Room` should check `producer->IsPaused()` and call `consumer->setSourcePaused(true)` or similar.
  - NOTE: Won't happen with the new design since that's done in JS land.

* If the transport of a consumer is closed, and the consumer was "enabled", it should become "disabled" and emit event (and send `consumerDisabled` notification? I think yes. Enabled should mean "it has a transport and RTP parameters.
  - NOTE: We don't need `consumerDisabled` since we already receive `transportClosed` which fires `unhandled` in the client's `Consumer`.

* May have to react on DTLS ALERT CLOSE in the server and make it "really" close the Transport and notify the client. Bufff... I don't like this...

* Must be ready for the case in which the client closes a `Transport` on which a `Producer` was running. Isn't it?

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).

* Since a `Producer` may receive RTP (and RTCP) with ssrc, pt and headerExtensionId different that those in the room, we must ready to also map consumer/generated/relayed RTCP.



## TODO per JS class

### Room

* Add `mandatoryCodecPayloadTypes`.
* Use/store `appData` when creating a `Peer`.
