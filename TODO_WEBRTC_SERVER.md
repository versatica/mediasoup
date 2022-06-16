# TODO WebRtcServer


### Issue obtainining `usernameFragment:password` from received STUN packets

When we create a ICE server we decide the mediasoup side `usernameFragment` and `password` and pass them to the `IceServer` constructor.

However, when a STUN packet is received from a client, if we check `packet->GetUsername()` it doesn't return the mediasoup side `usernameFragment:password` values but instead `mediasoupUsernameFragment:clientUsernameFragment`.

I've added some logs to show the problem:

```
mediasoup:ERROR:Channel [pid:35897 RTC::IceServer::IceServer() | ----------- IceServer constructor | usernameFragment:bxuwysb0l2hltk2h, password:0275z9opi3uiql3apboece5qrgvffm03 +6ms

mediasoup:ERROR:Channel [pid:35897 RTC::WebRtcTransport::OnStunDataReceived() | ----------- stunPacket->GetUsername(): bxuwysb0l2hltk2h:1dRN +36ms
````

In fact, if we show the local SDP of the client it clearly shows:

```
a=ice-ufrag:1dRN
```

So how to match received STUN packets against the credentials of our `IceServer`? We may just compare the "usernameFragment" component but unclear if this is good. Obviously the `usernameFragmet` is chosen by mediasoup `WebRtcTransport` constructor by doing `Utils::Crypto::GetRandomString(16)` so it could be good enough but still... The thing is that mediasoup-client does NOT signal the client `a=ice-ufrag` to mediasoup because being mediasoup ICE-Lite it doesn't need it.

STUN RFC: See https://datatracker.ietf.org/doc/html/rfc5389

Suggested solution: use a 32 bytes mediasoup side `usernameFragment`.

However, there is no rule in RFC 5389 telling that `USERNAME` attribute should contain `remoteUsernameFragment:localUsernameFragment`. Anyway, in `StunPacket::CheckAuthentication()` we already rely on `USERNAME` starting by our local username fragment followed by ":".

Done.


### What happens with its WebRtcTransports when closing a WebRtcServer?

This is hard. And there must be a parallelism between TS/Rust and C++.

Proposal:

* `webRtcServer.close()` carefully iterates its maps and calls `webRtcTransport.webRtcServerClosed()`.
* `webRtcTransport.webRtcServerClosed()` calls a new parent `Transport::MustClose()` method.
* `Transport::MustClose()` calls a new `this->listener->OnTransportMustClose(this)`.
* `Router.OnTransportMustClose(transport)` does:
	- Call `transport->CloseProducersAndConsumers()` so `Router` is notified about closed `Producers` and `Consumers` and can remove them from its maps.
	- Remove the `transport` from it map.
	- Call `delete transport`.
	
Note that the destructor of `WebRtcTransport` will invoke `delete this->iceServer` which will trigger may callbacks called in `WebRtcServer` that will remove the `transport` from its maps.

NOTE: All the above is done in the branch.

And let's see how to do it in TS/Rust for things to be similar.


### Remove limit of TcpConnections per TcpServer

Also increase `backlog(int)` in `uv_listen`(POSIX argument):


> The backlog argument defines the maximum length to which the
> queue of pending connections for sockfd may grow.  If a connection
> request arrives when the queue is full, the client may receive an
> error with an indication of ECONNREFUSED or, if the underlying 
> protocol supports retransmission, the request may be ignored so 
> that a later reattempt at connection succeeds.

No matter the value is super high, in Linux it's limited by `/proc/sys/net/core/somaxconn`.

Done by just hardcoding it to 256 since it's irrelevant after investigating it.


### Handle limit of UDP/TCP tuples in WebRtcTransport

This is needed no matter `WebRtcServer` is used or not. Each `WebRtcTransport` should manage max number of UDP and TCP tuples and remove old one (non selected one) etc when a new one is created or reject new one, let's see.


### Use the new TransportTuple.id

In `IceServer` we use `tuple.compare()` but now we have `tuple->id` to match things. See `GenerateId()` in `TransportTuple.hpp`.

### New events in `IceServer`

As far as a tuple is added or removed it must call a new callback. Also new events when local ICE usernameFrag is initially set or changed later.
