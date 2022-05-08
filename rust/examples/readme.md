# Examples
This directory contains examples of using mediasoup Rust library.

NOTE: These examples are simplified and provided for demo purposes only, they are by no means complete or representative
of production-grade software, use for educational purposes only and refer to documentation for all possible options.

In order to run server-side part of examples (this directory) use `cargo run --example EXAMPLE_NAME`.

# Echo
Simple echo example that receives audio+video from the client and sends audio+video back to the client via different
transport. Frontend part of this example is in `examples-frontend/echo` directory.

Check WebSocket messages in browser DevTools for better understanding of what is happening under the hood, also source
code has a bunch of comments about what is happening and where changes would be needed for production use.

# Video room
A bit more advanced example that allow multiple participants to join the same virtual room and receive audio+video from
other participants, resulting in a simple video conferencing setup. Frontend part of this example is in
`examples-frontend/videoroom` directory.

Check WebSocket messages in browser DevTools for better understanding of what is happening under the hood, also source
code has a bunch of comments about what is happening and where changes would be needed for production use.

# Multiopus
Demonstration of surround sound in WebRTC using Chromium-specific "multiopus" audio.

This one is very simple in a sense that it only supports one participant playing back sample audio and nothing else.

NOTE: This requires GStreamer 1.20, which at the moment of writing isn't released yet. On Linux
`docker run --rm -it --net=host restreamio/gstreamer:latest-prod-dbg` can be used to enter environment with working
GStreamer version, or you can compile one from upstream sources.

Check WebSocket messages in browser DevTools for better understanding of what is happening under the hood, also source
code has a bunch of comments about what is happening and where changes would be needed for production use.
