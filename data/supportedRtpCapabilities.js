'use strict';

/**
 * This file is not used by the JavaScript engine, but copied into the C++
 * source code once JSONized by gulp.
 */

let supportedRtpCapabilities =
{
	codecs :
	[
		// Audio codecs.
		{
			kind                 : 'audio',
			name                 : 'audio/opus',
			preferredPayloadType : 96,
			clockRate            : 48000,
			numChannels          : 2
		},
		{
			kind                 : 'audio',
			name                 : 'audio/ISAC',
			preferredPayloadType : 97,
			clockRate            : 16000
		},
		{
			kind                 : 'audio',
			name                 : 'audio/ISAC',
			preferredPayloadType : 98,
			clockRate            : 32000
		},
		{
			kind                 : 'audio',
			name                 : 'audio/G722',
			preferredPayloadType : 9,
			clockRate            : 8000
		},
		{
			kind                 : 'audio',
			name                 : 'audio/PCMU',
			preferredPayloadType : 0,
			clockRate            : 8000
		},
		{
			kind                 : 'audio',
			name                 : 'audio/PCMA',
			preferredPayloadType : 8,
			clockRate            : 8000
		},
		// Video codecs.
		{
			kind                 : 'video',
			name                 : 'video/VP8',
			preferredPayloadType : 100,
			clockRate            : 90000
		},
		{
			kind                 : 'video',
			name                 : 'video/VP9',
			preferredPayloadType : 101,
			clockRate            : 90000
		},
		{
			kind                 : 'video',
			name                 : 'video/H264',
			preferredPayloadType : 102,
			clockRate            : 90000
		},
		{
			kind                 : 'video',
			name                 : 'video/H265',
			preferredPayloadType : 103,
			clockRate            : 90000
		},
		// Depth codecs.
		{
			kind                 : 'depth',
			name                 : 'video/VP8',
			preferredPayloadType : 110,
			clockRate            : 90000
		}
	]
};

module.exports = supportedRtpCapabilities;
