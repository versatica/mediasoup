# TODO in mediasoup v2 (server-side)

* If the transport of a consumer is closed, and the consumer was "enabled", it should become "disabled" and emit event? I think yes. Enabled should mean "it has a transport". Period.

* Should accept `peer.createTransport()` by passing a `transportId` number. The same for `Producer`.

* May have to react on DTLS ALERT CLOSE in the server and make it "really" close the Transport and notify the client. Bufff... I don't like this...

* Must be ready for the case in which the client closes a `Transport` on which a `Producer` was running. Isn't it?

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).



## TODO per JS class

### Room

* Add `mandatoryCodecPayloadTypes`.
* Use/store `appData` when creating a `Peer`.
