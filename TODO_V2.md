# TODO in mediasoup v2 (server-side)

* When a Peer disconnect, others receive `peerClosed` and later `consumerClosed` for each Consumer associated to the closed Peer, so the client side complains ("peer not found"). We should avoid sending `consumerClosed` if its Peer left.

* Revisar que si un Producer JS empieza pausado, esté tb pausado en el worker y en el Consumer JS asociado. Y que si un Consumer JS se habilita pausado, que se entere tb el Consumer del worker.
  - Y que si un Producer se pausa (y, por lo tanto, sus Consumers están "sourcePaused") al hacer `enable()` en esos Consumers se le diga al cliente que están pausados.

* Revisar appData en todos los mensajes.

* Remove DEPTH stuff.

* Redo the compile_commands_template.json.

* Rethink the auto close of the worker `Transport`. Yes/no?

* Move the RTCP timer from Peer to Transport.

* `then()` of Promises is async so if we create an instance (Peer, Producer, Transport, etc) within the `channel.request().then(....)` and an event happens for that instance immediately, such an event may be lost.
  - The solution is creating the instance before calling `channel.request()`.
  - DOC: https://stackoverflow.com/questions/36726890/why-are-javascript-promises-asynchronous-when-calling-only-synchronous-functions
    + I must avoid unnecesary `Promise.resolve()`.
  - "The callback passed to a Promise constructor is always called synchronously, but the callbacks passed into then are always called asynchronously"

* May have to react on DTLS ALERT CLOSE in the server and make it "really" close the Transport and notify the client. Bufff... I don't like this...

* Upon a `updateProducer` notification, map the new SSRCs of the `Producer` and send a PLI back to the browser (otherwise, consumers will get frozen video).


## TODO per JS class

### Room

* Add `mandatoryCodecPayloadTypes`.
* Use/store `appData` when creating a `Peer`.
