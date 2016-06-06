'use strict';

/**
 * This file is not used by the JavaScript engine, but copied into the C++
 * source code once JSONized by gulp.
 */

let defaultRtpCapabilities =
{
	codecs :
	[
		{
			kind                 : 'audio',
			name                 : 'audio/opus',
			preferredPayloadType : 100,
			clockRate            : 90000,
			rtcpFeedback :
			[
				{ type: 'ccm',         parameter: 'fir' },
				{ type: 'nack',        parameter: '' },
				{ type: 'nack',        parameter: 'pli' },
				{ type: 'google-remb', parameter: '' }
			]
		}
	]
};

module.exports = defaultRtpCapabilities;
