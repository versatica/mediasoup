const { toBeType } = require('jest-tobetype');
// const { UnsupportedError } = require('../lib/errors');
const ortc = require('../lib/ortc');
const supportedRtpCapabilities = require('../lib/supportedRtpCapabilities');

expect.extend({ toBeType });

test('generateRouterRtpCapabilities() succeeds', () =>
{
	const mediaCodecs =
	[
		{
			kind       : 'audio',
			name       : 'opus',
			mimeType   : 'audio/opus',
			clockRate  : 48000,
			channels   : 2,
			parameters :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		},
		{
			kind                 : 'video',
			name                 : 'VP8',
			mimeType             : 'video/VP8',
			preferredPayloadType : 100, // Will be ignored because 100 is for first codec.
			clockRate            : 90000
		}
	];

	const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	expect(rtpCapabilities.codecs.length).toBe(3);
	expect(rtpCapabilities.codecs[0]).toEqual(
		{
			kind                 : 'audio',
			name                 : 'opus',
			mimeType             : 'audio/opus',
			preferredPayloadType : 100, // Because this is the first PT chosen.
			clockRate            : 48000,
			channels             : 2,
			rtcpFeedback         : [],
			parameters           :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		});
	expect(rtpCapabilities.codecs[1]).toEqual(
		{
			kind                 : 'video',
			name                 : 'VP8',
			mimeType             : 'video/VP8',
			preferredPayloadType : 101,
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			],
			parameters : {}
		});
});
