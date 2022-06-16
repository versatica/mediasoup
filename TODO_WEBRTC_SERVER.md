# TODO WebRtcServer


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


### Handle limit of UDP/TCP tuples in WebRtcTransport

This is needed no matter `WebRtcServer` is used or not. Each `WebRtcTransport` should manage max number of UDP and TCP tuples and remove old one (non selected one) etc when a new one is created or reject new one, let's see.


### Use the new TransportTuple.id

In `IceServer` we use `tuple.compare()` but now we have `tuple->id` to match things. See `GenerateId()` in `TransportTuple.hpp`.

### New events in `IceServer`

As far as a tuple is added or removed it must call a new callback.
