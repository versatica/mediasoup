# libsrtp dependency for mediasoup

Here we provide our own `libsrtp.gyp` for mediasoup which is inspired by the corresponding file in Chromium sources. However, Chromium has migrated from GYP to GN:

* https://chromium.googlesource.com/chromium/deps/libsrtp/+/master/BUILD.gn


## IMPORTANT

When the `libsrtp` dependency is updated to a new version, we must update according the `config/config.h` file which defines C macros with the new version.
