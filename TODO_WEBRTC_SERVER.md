# TODO WebRtcServer


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
