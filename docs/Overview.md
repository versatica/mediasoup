# Overview


## Definitions


### MediaSoup

The media server itself is a single process internally comprised of two main components: the *Dispatcher* and *Workers*.


### MediaSoup ControlProtocol

The MediaSoup [ControlProtocol](./ControlProtocol.md) is a bidirectional protocol to manage MediaSoup and multimedia sessions.


### Controller

The Controller is a separate application (not provided by MediaSoup) that can run in the same or a different host. The Controller manages the signaling protocol (for example SIP in case the Controller is a SIP proxy/server) and communicates with MediaSoup by using the MediaSoup [ControlProtocol](./ControlProtocol.md) over a TCP connection.

By using the MediaSoup ControlProtocol, the Controller requests MediaSoup to create multimedia sessions by providing MediaSoup with the required media information (this is, the media information of the remote peer) and receiving from MediaSoup its media information to be transmitted to the remote peer.

The Controller can generate multiparticipant sessions (let's say multi-conferences) or single sessions between two remote participants that communicate through MediaSoup (for example having MediaSoup behaving as a media gateway between a WebRTC endpoint and a legacy SIP device).

Multiple Controllers can communicate with the same instance of MediaSoup.


### MediaSoup Dispatcher

The Dispatcher is the MediaSoup component that listens for messages from Controllers and sends messages back to them when appropriate. The messages exchanged between both a Controller and the Dispatcher conform to the MediaSoup [ControlProtocol](./ControlProtocol.md).

The Dispatcher also uses the MediaSoup ControlProtocol to communicate with the MediaSoup Workers.


### MediaSoup Worker

An instance of MediaSoup runs N Workers instances (N is typically the number of cores detected in the host). A Worker manages multimedia sessions by handling the RTP, RTCP, ICE, DTLS and so on. A MediaSoup session (let it be a single session of two participants or a multiparticipant session) is handled by a single Worker, but a Worker can handle more than one session.

The MediaSoup Dispatcher communicates with the MediaSoup Workers (and vice-versa) by sending MediaSoup [ControlProtocol](./ControlProtocol.md) messages over UNIX sockets. The Dispatcher runs a separate communication channel with each Worker.

