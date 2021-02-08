
# Sender side bandwidth estimator

As its name inidicates, this bandwidth estimation is done at the RTP sender side, which has
the information of the RTP packets sent and retrieves information on how those packets
are received via RTCP feedback.

The aim of this work is to predict congestion before it happens by observing how packets
are being received and to make an estimation of how much traffic a receiver can handle without
reaching congestion.

When network queues start to fill up, the received inter-packet delay increases due to the
buffering, and once the queues are filled, packet loss[1] starts to happen.

[1]: Packet loss happens due to filled network queues but it can also happen due to the network
nature.

## Notes from previous works

[Reference](https://c3lab.poliba.it/images/6/65/Gcc-analysis.pdf).

- Loss based congestion control is not suitable because, when caused by full queues,
	it is already late, we should react before this happens.
- When the RTT is used as a congestion metric a low channel utilization may be obtained
in the presence of reverse traffic or when competing with loss-based flows.
- One way delay congestion control has shown that when two flows share the same
bottleneck the second flow typically starves the first one.

## Glossary

- *Sender*: The enpoint sending traffic.
- *Receiver*: The enpoint receving traffic and sending feedback to the sender.
- *BWE* (Bandwidth Estimation): An estimation of the bandwidth a received can handle.
- *RTT* (Round Trip Time): time a packet requires to go from client to server and return.
- *jitter* or packet delay variation: variation in latency as measured in the variability over time of the end-to-end delay across a network. A network with constant delay has no packet jitter. Packet jitter is expressed as an average of the deviation from the network mean delay.

## Per RTP packet information

### Sent RTP packet

#### Availabe information

- Sequence number, incremented by one for each sent packet: `wideSeq`.
- Timestamp[2] indicating when it was sent, represented in milliseconds: `sentAtMs`.
- Total packet size represented in bytes: `size`.
- Whether it is a probation packet: `probation`.
- Whether the feedback for this packet has been received: `received`.

#### Computable information

- Given the timestamps and the size, the sending bitrate can be calculated for a given
	time window.
- Given the time difference between two consecutive sent packets, the delta.

### Sent RTP packet feedback

#### Available information

- Sequence number, pointing the sent RTP packet this feedbak refers to.
- Timestamp[2] when it was received, represented in milliseconds: `receivedAtMs`.

#### Computable information

- Given the timestamps and the size, the receiving bitrate can be calculated for a given
	time window.
- Given the time difference between two consecutive received packets, the delta.


[2]: Clocks for sent and received packets are independent.


### Example of raw representation for certain amount of packets

| wideSeq | size | sentAtMs | probation | received | receivedAtMs |
|---------|------|----------|-----------|----------|--------------|
| 2112 | 680 | 1612631207408 | false | true  | 2119531504 |
| 2113 | 340 | 1612631207409 | false | false | 2119531505 |
| 2114 | 680 | 1612631207410 | false | true  | 2119531505 |
| 2115 | 468 | 1612631207410 | false | true  | 2119531506 |
| 2116 | 340 | 1612631207413 | false | true  | 2119531509 |


## Realm diagram

```
[:Sender] ---> [:Network queue/s] ---> [:Receiver] ---> [:Decoder buffer/s]
    |                                       |
    ----------      feedback        <--------
```


## Different networking scenarios

### No delay (not a real scenario)

The receiver receives the traffic as it is sent.

* sent bitrate == received bitrate
* sent delta == received delta

| wideSeq |  sentAtMs | receivedAtMs |
|---------|-----------|--------------|
| 1 | 1612631207408 | 2119531503 |
| 2 | 1612631207409 | 2119531504 |

### No jitter and delay smaller than the sent delta (not a real scenario)

The receiver receives the traffic as it is sent.

* sent bitrate == received bitrate
* sent delta == received delta

| wideSeq |  sentAtMs | receivedAtMs |
|---------|-----------|--------------|
| 1 | 1612631207408 | 2119531503 |
| 2 | 1612631207409 | 2119531504 |


### No jitter and delay higher than the sent delta

The receiver receives the traffic with a higher delta between packets than it is sent.

* sent bitrate > received bitrate
* sent delta < received delta

| wideSeq |  sentAtMs | receivedAtMs |
|---------|-----------|--------------|
| 1 | 1612631207408 | 2119531503 |
| 2 | 1612631207409 | 2119531505 |

### Network buffers being filled

The receiver receives the traffic with a higher delta between packets increasingly.

* sent bitrate > received bitrate
* sent delta < received delta

| wideSeq |  sentAtMs | receivedAtMs |
|---------|-----------|--------------|
| 1 | 1612631207408 | 2119531503 |
| 2 | 1612631207408 | 2119531504 |
| 3 | 1612631207409 | 2119531505 |
| 4 | 1612631207409 | 2119531506 |

### Network buffers being drained

The received delta can be lower than the sending one.

* sent bitrate < received bitrate[*]
* sent delta < received delta

[*]: Received bitrate is higher than sent bitrate considering the time window
			is defined for first and last sent and received packets respectively.
			*See table below*.


| wideSeq |  sentAtMs | receivedAtMs |
|---------|-----------|--------------|
| 1 | 1612631207408 | 2119531503 |
| 2 | 1612631207409 | 2119531503 |


### Network buffers full

The received delta increases.

* sent bitrate > received bitrate
* sent delta < received delta
* packet loss

| wideSeq |  sentAtMs | received | receivedAtMs |
|---------|-----------|----------|--------------|
| 1 | 1612631207408 | true | 2119531503 |
| 2 | 1612631207408 | false | |
| 3 | 1612631207409 | false | |
| 4 | 1612631207409 | true | 2119531506 |


## Concepts and mechanisms

### BW Overuse detector

A *trendine estimator* is needed in order to determine whether there is BW overuse for a certain moment, given that the accumulated delay increases over time.

The trendline estimator helps avoiding taking premature decisions onthe BWE upon the existence of isolated delta increases/decreases.

Delay is calculated as a difference bettween the send time deltas for RTP packets and the received time deltas of such RTP packets.

## Considerations 

(To be properly documented when appropriate).

### Packet loss

When inspecting TCC feedbacks there may be holes. It can happen because:

1. Some sent RTP packets were lost and never received by the endpoint.
2. Some sent RTP packets were received out of order by the received.
3. Some feedback packets were lost from receiver to mediasoup (there is nothing we can do about this).

Since we know the current average RTT of the transport, we should consider those holes differently when it comes to process the sending info map (every few seconds):

- Those infos for which there is no `recvTime` and `currentTime - sentTime > 2 * RTT` should account as a problem and should contribute to reduce the estimated bandwidth. If for example the network is temporary down, this is the way to detect that no packets are being received by the endpoint so we should NOT contribute to produce more congestion (so we should stop sending RTP by decreasing BWE).

- Those infos for which `currentTime - sentTime < RTT` should be just ignored since it's 100% expected to not have received yet their TCC feedbacks. To clarify, those infos should be ignored even if they have `recvTime`. Why? because it's expected to not have yet all the feedbacks and hence it's better to wait a bit before processing them.

- Those infos in the middle with `(currentTime - sentTime < 2 * RTT) && (currentTime - sentTime >= RTT)` should be processed and account for BWE changes. Here we may have infos with and without `recvTime`, however it's not clear whether holes here (missing `recvTime`) should account for BWE decrease or whether it's just better to wait until future iterations when `currentTime - sentTime > 2 * RTT` happens (as told in the first bullet).

_NOTE:_ This is conceptually WIP. Values may change, etc.
