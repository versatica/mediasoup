'use strict';

module.exports =
{
	roomOptions :
	{
		mediaCodecs :
		[
			{
				kind        : 'audio',
				name        : 'audio/opus',
				clockRate   : 48000,
				numChannels : 2
			},
			{
				kind        : 'audio',
				name        : 'audio/PCMU',
				clockRate   : 8000
			},
			{
				kind      : 'video',
				name      : 'video/vp8',
				clockRate : 90000
			},
			{
				kind       : 'video',
				name       : 'video/h264',
				clockRate  : 90000,
				parameters :
				{
					packetizationMode : 0
				}
			},
			{
				kind       : 'video',
				name       : 'video/h264',
				clockRate  : 90000,
				parameters :
				{
					packetizationMode : 1
				}
			},
			{
				kind      : 'depth',
				name      : 'video/vp8',
				clockRate : 90000
			}
		]
	},

	peerOptions :
	{
		capabilities :
		{
			codecs :
			[
				{
					kind                 : 'audio',
					name                 : 'audio/opus',
					preferredPayloadType : 100,
					clockRate            : 48000,
					numChannels          : 2
				},
				{
					kind                 : 'video',
					name                 : 'video/VP8',
					preferredPayloadType : 101,
					clockRate            : 90000
				}
			]
		}
	}
};
