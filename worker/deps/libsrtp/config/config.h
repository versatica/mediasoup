// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a stub config.h for libSRTP. It doesn't define anything besides
// version number strings because the build is configured by libsrtp.gyp.

#define PACKAGE_STRING "libsrtp 2.3.0"
#define PACKAGE_VERSION "2.3.0"

#if defined(_MSC_VER) && !defined(__cplusplus)
  // Microsoft provides "inline" only for C++ code
  #define inline __inline
#endif

