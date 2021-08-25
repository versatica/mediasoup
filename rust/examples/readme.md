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
