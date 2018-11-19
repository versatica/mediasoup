# TODO in mediasoup v2 (server-side)

* `RtpStreamer` should have its own RTP capabilities rather than using the ones from the room.

* Add `gulp lint/format/tidy` also for `worker/test/*`.

* Rethink the auto close of the worker `Transport` when receiving a DTLS CLOSE ALERT (or when it's just TCP and the connection fails...). Yes/no? BTW it's generating some issues because the JS closes the Producer/Transport and gets `request failed [id:84026423, reason:"Consumer/Transport does not exist"]` (because it was already closed due to DTLS ALERT).

* Implement `mandatoryCodecPayloadTypes`.

* Use encrypted emails in `.travis.yml` to avoid notifications produced by mediasoup forks:
  - https://travis-ci.community/t/use-travis-encrypt-to-set-multiple-notifications-email-recipients/976
