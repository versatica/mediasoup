# README

This folder contains a modified/adapted subset of the libwebrtc library, which is used by mediasoup for transport congestion purposes.

* libwebrtc branch: m77
* libwebrtc commit: 2bac7da1349c75e5cf89612ab9619a1920d5d974

The file `libwebrtc/mediasoup_helpers.h` includes some utilities to plug mediasoup classes into libwebrtc.

The file `worker/deps/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/synchronization/internal/graphcycles.cc` has `#include <limits>` added to it to fix CI builds with Clang.

The file `meson.build` is written for using with Meson build system.
