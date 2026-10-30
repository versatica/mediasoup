# TODO: Dependency Descriptor

Follow-up work left over from the Dependency Descriptor branch
([PR #2015](https://github.com/versatica/mediasoup/pull/2015)), roughly in order
of value. Everything here is open; nothing in this list blocks that PR.

## 1. Precompute the per-structure lookups

`DependencyDescriptor::GetDecodeTargetsBitmask()` walks every decode target
against every template on **every packet of every Consumer**: 9 × 15 ≈ 135
iterations for an L3T3 stream. The early return that would avoid it only fires
when the descriptor carries `active_decode_targets_present`, and Chrome never
sets it, so it never fires in practice.

Both lookups are a function of the template dependency structure alone, so they
can be built once in `ComputeDecodeTargetLayers()` and stored next to
`decodeTargetLayers`:

- the inverse map `(spatialLayer, temporalLayer) -> decodeTarget`, which
  `GetDecodeTarget()` searches linearly today;
- a per-decode-target bitmask table, which is what `GetDecodeTargetsBitmask()`
  recomputes.

The structure arrives on key frames and is shared through a `shared_ptr`, so the
tables are built once per structure and read by every Consumer.

## 2. Enforce the forward/announce invariant

In `DependencyDescriptorPayloadDescriptorHandler::Process()` two calls must
resolve to the *same* decode target:

- `IsPresentInDecodeTarget(tmpSpatialLayer, tmpTemporalLayer)`, which decides
  whether the packet is forwarded;
- the `maxSpatialLayer`/`maxTemporalLayer` handed to `CreateEncoder()`, which
  decides the `active_decode_targets_bitmask` the endpoint is told about.

Announce more than we send and the client PLIs forever waiting for frames that
never come; send more than we announce and we waste the bandwidth the filter
was meant to save. The two sit about fifteen lines apart with nothing tying
them together.

Resolving the decode target **once** and passing the index to both would make
the invariant structural. This folds naturally into item 1, since that work
already introduces the index-based lookup.

## 3. Update the CHANGELOG

`../CHANGELOG.md` still reads *"AV1: Fix SVC not being decodable"*. That no
longer describes the change:

- K-SVC streams were over-forwarded to every Consumer (a Consumer on the top
  spatial layer got all three layers where it needs one, ~1.8× the packets);
- the fix reaches VP9 and H264 too, not just AV1, since the handler is driven
  by the extension rather than by the codec;
- out of order packet handling is new for the Dependency Descriptor (AV1 never
  had it; see item 5).

## 4. Delete VP9's `SeqManager` in favour of the shared frame id tracker

VP9's `pictureIdManager` is vestigial: its `Input()` output is discarded and
`GetEncoder()` returns `nullptr`. It exists only to answer "is this packet
old?", which `EncodingContext::IsOldFrameId()` now answers for every codec.

The catch is that it is not an exact replacement. Unlike VP8, VP9 never calls
`Drop()`, so its high water mark advances only on **forwarded** packets, where
`UpdateMaxFrameId()` advances on **seen** ones. Ours is the stricter and better
test, but swapping them changes VP9 behaviour established by
[#1486](https://github.com/versatica/mediasoup/pull/1486).

Needs a discriminating test first — drop a run of packets, then deliver a late
one — and its own PR.

## 5. Decide what bitmask an old packet should announce

Unresolved. A reordered packet currently announces the **current**
`active_decode_targets_bitmask`, not the one in effect at its own frame id,
even though every other decision about it is made with the layers its own frame
was given.

Three candidates: the current bitmask (today's behaviour), the one in effect at
its frame id, or no rewrite at all. Worth checking against how libwebrtc
consumes `active_decode_targets_bitmask` on a reordered frame before changing
anything.

## 6. Cover the switch point half of the downgrade guard

The downgrade is gated on `isKeyFrame || (atSwitchPoint && targetAlreadySent)`.
The `targetAlreadySent` half is covered both ways: full SVC switches at the
switch point, K-SVC waits for a key frame.

The `atSwitchPoint` half is not. On every captured structure the T0 frame of a
spatial layer is a `Switch` for its decode targets, so that condition never
decides anything. Covering it needs a structure whose per layer T0 frame is
`Required` instead, which Chrome will not produce, and
`FrameDependencyTemplate` is private and only reachable by parsing, so a test
cannot build one in C++ either. It needs hand encoded Dependency Descriptor
bytes.

## 7. Capture longer fixtures

Both fixtures are a single 12 frame repetition, which has now constrained three
tests: the full SVC reorder test had to pick frame 12 because nothing follows
frame 15, and the K-SVC downgrade can only be exercised once per run.

Multi GOP captures of `L3T3` and `L3T3_KEY`, a few seconds each, would let
tests do downgrade, settle, upgrade, settle with honest frame numbering.

## 8. Track chains

Chains are the one part of the Dependency Descriptor we parse and never use.
`decodeTargetProtectedBy` and `frameDiffChains` are read, dumped and dropped.
A chain says which frames must have been forwarded for a decode target to stay
decodable, so tracking the one protecting the forwarded decode target tells us
exactly when the stream broke, instead of guessing.

This is about **loss handling, not layer switching**. What it buys:

- **Telling a harmless loss from a real break.** Losing a frame that is in the
  forwarded decode target but not in its chain costs a couple of frames and
  heals by itself. We cannot tell that from a break today, so recovery waits on
  the endpoint asking for a key frame.
- **Seeing breaks we cannot see at all.** `IsDecodeTargetActive()` tells us what
  we *meant* to forward. If those packets were lost on the wire the chain is
  broken anyway and nothing here knows. Chains measure what arrived, our
  inference measures intent.
- **Smarter NACKs**, retransmitting only chain critical packets. The article
  below lists this as future work even for Medooze.

Note mediasoup forwards packets as they arrive and has no reorder queue, so the
optimistic formulation is needed: be hopeful on a gap and only act once the
packet is definitely lost. See
<https://medooze.medium.com/mastering-the-av1-svc-chains-a4b2a6a23925>.

## What chains would NOT fix

The key frame that `SvcProducerStreamManager::UpdateTargetLayers()` asks for on
every K-SVC target spatial change. A live run showed six in twenty five seconds
while the endpoint asked for none, which looks like pure waste, and it is not.

A chain is restarted only by a frame that begins it. In K-SVC the chain
protecting the target spatial layer runs through that layer's frames, and we
have been dropping every one of them, so no delta frame can restart it. Chain
tracking would only confirm what `IsDecodeTargetActive()` already infers, and
give no earlier moment to switch. The Medooze algorithm ends in the same place:

> If there is no Decode Target available with a valid Chain, drop the packet and
> request an I Frame to the sender.

So that key frame request is correct and stays. Trying to remove it was
attempted twice on this branch and was wrong both times.

## 9. Document the `codec` log tag

`Bits::CountSetBits()` moving to `Bits.cpp` is done, but the `codec` log tag
still owes a TODO on the website documentation (ibc's condition for landing it).

---

# Design notes worth preserving

Things that are easy to "improve" and break. All of these are load-bearing.

## The filter order in `Process()` is deliberate

Which filter drops a packet determines whether the layer change is committed:

| dropped by | commits the layer change? |
|---|---|
| spatial / temporal filter | no — returns before `SetCurrent*Layer()` |
| **DTI filter** | **yes — runs after, on purpose** |
| old packet | no — the switch logic is gated on `!isOldPacket` |

The DTI filter sits *after* the context updates because in K-SVC the switch
points of a decode target are frequently frames we do not forward. Layer
switching has to watch every frame of the stream, including those the endpoint
never sees. Moving the filter earlier breaks temporal upgrades, and the AV1
tests catch it.

The spatial filter must also stay *between* the spatial and temporal switch
blocks. Move it after and a packet that is about to be dropped on spatial can
still fire the temporal downgrade and commit it.

## A spatial switch does not touch the temporal layer

Both spatial branches used to do `tmpTemporalLayer = 0` with the comment
`// Just in case.`, inherited from the VP9 handler. It cost a frame rate dip on
every spatial change, since the temporal layer then had to climb back through
its own switch point, and with frequent changes it never finished. Live runs
showed Consumers pinned at temporal 0.

The temporal layer is now left alone and the downgrade is gated on the decode
target of the target spatial layer **at the temporal layer in use**. A key frame
is a switch point for every decode target, so the upgrade needs nothing more.

## Layer changes must be reported even for frames that are not sent

The temporal layer changes at the first frame of a picture, which in K-SVC
belongs to a spatial layer below the one the Consumer gets and is therefore
dropped. `SvcProducerStreamManager::ProcessRtpPacket()` originally compared the
layers only after the `ProcessPayload()` early return, so those Consumers' layers
moved without the application being told, while Consumers on spatial layer 0
looked fine because for them that frame is forwarded.

This was a regression introduced by decode target filtering: before it, K-SVC
over forwarded everything, so the switching frame was sent and the notification
fired by accident.

## The `packetSpatialLayer == 0` gate on temporal upgrades is load-bearing

The temporal upgrade assigns the *target* layer, not the packet's, and is gated
on the packet being the lowest spatial layer of its picture. The obvious
redesign — key the switch point on the target decode target and drop the gate —
is wrong: it lets an S2·T1 frame through after its S0/S1 siblings were dropped.

Replacing the check with the Consumer's own decode target while keeping the gate
is worse still: it cannot ever be satisfied. The S0·T0 template is `-` for DT8
in K-SVC and `R` in full SVC, never `Switch`, so the temporal layer would never
rise above 0 for any Consumer above spatial 0. Asking `DT(0, t)` is a proxy for
"this picture is a temporal switch point", and it holds because the encoder
gives every spatial layer the same temporal structure.
Harmless under K-SVC, broken under full SVC. Keeping the gate but querying the
target decode target never fires under K-SVC. Do not repeat this.

## The handler belongs to the packet, the encoder to the Consumer

A payload descriptor handler describes one packet and is shared by every
Consumer of the Producer, so it must hold nothing about any of them. The
encoder is the opposite: it is how **one** Consumer rewrote that packet, kept so
that its retransmissions are rewritten the same way.

The encoder used to sit in a `std::optional` on the descriptor, which every
Consumer overwrote in turn and read back afterwards. That was only correct
because the Router gives them the packet one at a time, an ordering nothing
stated. It now goes to the encoding context, of which each Consumer has its own:

```
handler::Process()          creates it, sets context->encoder
manager (FORWARD path)      result.encoder = std::move(context->encoder)
Consumer::SendRtpPacket()   item->encoder  = std::move(action.encoder)
RtpStreamSend, on NACK      packet->EncodePayload(item->encoder.get())
```

The handler still takes an encoder in `Encode()`: it knows *how* to rewrite, the
encoder knows *what to*. Only VP8 and the Dependency Descriptor make one, since
the other codecs rewrite nothing.

## Only the Consumers that negotiated it are served by the Dependency Descriptor

The layers of a frame can only be told from the descriptor by an endpoint that
receives it, and a Consumer that did not negotiate the extension never does.
Filtering such a Consumer by decode target would drop frames it is never told
not to wait for.

So a packet carries **two** handlers, the codec's and the Dependency
Descriptor's, and `Packet::ProcessPayload()` picks by
`EncodingContext::PrefersDependencyDescriptor()`. The choice cannot be made once
per packet, since one packet is given to every Consumer and each may be served
by a different one.

AV1 is the exception: it only ever gets the Dependency Descriptor handler, in
the codec slot as well, because nothing else tells the layers of an AV1 packet
apart.

## A switch point is not an entry point

The single most expensive misreading on this branch. A `Switch` indication
promises that **no frame of that decode target older than this one is needed**.
It does **not** promise that the frame stands on its own.

libwebrtc marks every S1T0 *delta* frame `---SSS---`, a `Switch` for all three
S1 decode targets, even though it references the previous S1T0 frame. That is
consistent: within the S1 decode target, nothing older than it is needed
*provided you were receiving that decode target*. An endpoint that was not gets
a frame whose reference it never saw.

So the downgrade is gated on `isKeyFrame || (atSwitchPoint && targetAlreadySent)`,
where `targetAlreadySent` is `IsDecodeTargetActive(target, current)`:

| | `targetAlreadySent` | downgrade happens at |
|---|---|---|
| full SVC | true, the lower layers are `R` for the top target | the switch point |
| K-SVC | false, they are `-` | a key frame, as VP9 does |

An earlier version of this note said the opposite on both counts: that waiting
for a key frame would be a regression, and that gating on `IsDecodeTargetActive()`
could never fire. Both were wrong. Credit to satoren for catching it on
[PR #2015](https://github.com/versatica/mediasoup/pull/2015).

The warning sign was there and was talked away twice: a *delta* frame marked
`Switch` for a decode target it cannot start looks impossible, and it is, under
the wrong reading of `Switch`. Chain tracking (item 8) is what would let the
key frame wait be lifted honestly.

## DTI filtering alone is unsound

Filtering on decode target indications without also narrowing the announced
bitmask fails three AV1 tests. The two halves must change together. See item 2.

## K-SVC is not a flag

The Dependency Descriptor spec has no concept of K-SVC. It is an emergent shape
of the decode target indications, on the S0·T0 **delta** template:

| scalability mode | S0·T0 delta DTIs | meaning for the S1/S2 decode targets |
|---|---|---|
| `L3T3` (full SVC) | `SSSRRRRRR` | `R` = Required, so S0 is needed |
| `L3T3_KEY` (K-SVC) | `SSS------` | `-` = NotPresent, so S0 is not needed |

This is why the filter is safe for both: full SVC forwards everything unchanged,
K-SVC drops what the endpoint does not need, and the Dependency Descriptor
handler never asks whether the stream is K-SVC.

The worker does know, mind: `ksvc` reaches it through `RtpEncodingParameters`
and is read by `EncodingContext::IsKSvc()`, `RtpStream::IsKSvc()`, the VP9
handler and the bitrate allocation. It is the Dependency Descriptor path alone
that has no use for it.

## `FrameIdList::GetLayer()` needs two entries

It returns `-1` with one entry or fewer, and the caller then falls back to the
*current* layer. So the reordering protection only engages from the second
recorded layer change onwards. Tests must let the context start at `-1` and must
not pre-set the layers with the one argument setters, or the list never gets its
second entry and the test silently proves nothing.

---

# Testing notes

- **Run the tests from `worker/`**, not the repo root: `TestPacket` reads
  `data/packet*.raw` relative to the working directory.
- `[transporttuple]` cannot bind UDP in a sandbox; exclude it with
  `"~[transporttuple]"`.
- **Prove every new test bites** by sabotaging the source and watching that test
  — and ideally only that test — fail. Then re-`grep` for the sabotage after
  reverting; a failed revert looks exactly like a regression.
- Never change a test expectation just because the output changed. Twice on this
  branch the fixture was the thing that was wrong (a key frame replayed with a
  stale frame number, and `packet3` in `TestRtpStreamSend` sharing a frame number
  with `packet1`), and both times the right fix was to make the capture
  realistic, not to move the goalposts.
- `framesReceived/s` is useless for measuring spatial filtering — it reads ~30 in
  every run. Use `headerBytes` as the proxy.

## Capturing fixtures

```sh
tshark -i <interface> -f "udp portrange <rtcMinPort>-<rtcMaxPort>" -s 0 -w capture.pcapng

tshark -r capture.pcapng -o rtp.heuristic_rtp:TRUE -Y "rtp.p_type==<payload type>" \
  -T fields -e rtp.seq -e rtp.marker -e rtp.ext.rfc5285.data
```

The Dependency Descriptor is the extension whose value is ~90 bytes in the first
packet of a key frame and 3 bytes everywhere else.

Two gotchas:

- `S3T3_KSVC` is **not** a scalability mode. The regex in
  `node/src/scalabilityModesUtils.ts` is not end anchored and accepts only
  `_KEY`, so it silently parses as `S3T3` with `ksvc: false`. Use `L3T3` and
  `L3T3_KEY`.
- mediasoup-client strips the Dependency Descriptor extmap for anything but AV1
  and H264 (`DependencyDescriptorCodecs` in
  `src/handlers/sdp/RemoteSdp.ts`). Add `'vp9'` locally to capture VP9 traffic
  carrying the extension.
