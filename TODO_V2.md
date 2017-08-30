# TODO in mediasoup v2 (server-side)

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.

* Rethink the auto close of the worker `Transport`. Yes/no?

* Move the RTCP timer from Peer to Transport.

* `then()` of Promises is async so if we create an instance (Peer, Producer, Transport, etc) within the `channel.request().then(....)` and an event happens for that instance immediately, such an event may be lost.
  - The solution is creating the instance before calling `channel.request()`.
  - DOC: https://stackoverflow.com/questions/36726890/why-are-javascript-promises-asynchronous-when-calling-only-synchronous-functions
    + I must avoid unnecesary `Promise.resolve()`.
  - "The callback passed to a Promise constructor is always called synchronously, but the callbacks passed into then are always called asynchronously"

* worker: If a `Producer` is paused and a new `Peer` joins (so a new `Consumer` is generated) such a new `Consumer` does not know that its associated `Producer` is paused. When creating a `Consumer` the `Room` should check `producer->IsPaused()` and call `consumer->setSourcePaused(true)` or similar.
  - NOTE: Won't happen with the new design since that's done in JS land.

* May have to react on DTLS ALERT CLOSE in the server and make it "really" close the Transport and notify the client. Bufff... I don't like this...

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).


## TODO per JS class

### Room

* Add `mandatoryCodecPayloadTypes`.
* Use/store `appData` when creating a `Peer`.
