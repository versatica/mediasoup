'use strict';

module.exports =
{
	roomOptions :
	{
		mediaCodecs :
		[
			{ kind: 'audio', name: 'audio/opus' },
			{ kind: 'video', name: 'video/vp8' },
			{ kind: 'depth', name: 'audio/opus' }
		]
	},

	peerRtpCapabilities :
	{
		codecs :
		[
			{
				kind                 : 'audio',
				name                 : 'audio/opus',
				preferredPayloadType : 100,
				clockRate            : 90000
			},
			{
				kind                 : 'video',
				name                 : 'video/vp8',
				preferredPayloadType : 101,
				clockRate            : 90000
			}
		]
	}
};
