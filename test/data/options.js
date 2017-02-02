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
				payloadType : 100,
				numChannels : 2
			},
			{
				kind        : 'audio',
				name        : 'audio/PCMU',
				payloadType : 0,
				clockRate   : 8000
			},
			{
				kind      : 'video',
				name      : 'video/vp8',
				payloadType : 110,
				clockRate : 90000
			},
			{
				kind       : 'video',
				name       : 'video/h264',
				payloadType : 111,
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
				payloadType : 112,
				parameters :
				{
					packetizationMode : 1
				}
			},
			{
				kind      : 'depth',
				name      : 'video/vp8',
				payloadType : 120,
				clockRate : 90000
			}
		]
	},

	peerCapabilities :
	{
		codecs :
		[
			{
				kind        : 'audio',
				name        : 'audio/opus',
				payloadType : 96,
				clockRate   : 48000,
				numChannels : 2
			},
			{
				kind        : 'audio',
				name        : 'audio/PCMU',
				payloadType : 0,
				clockRate   : 8000
			},
			{
				kind        : 'video',
				name        : 'video/VP8',
				payloadType : 97,
				clockRate   : 90000
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 100,
				clockRate   : 90000,
				parameters :
				{
					apt : 97
				}
			},
			{
				kind        : 'video',
				name        : 'video/H264',
				payloadType : 98,
				clockRate   : 90000,
				parameters  :
				{
					packetizationMode : 0
				}
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 101,
				clockRate   : 90000,
				parameters  :
				{
					apt : 98
				}
			},
			{
				kind        : 'video',
				name        : 'video/H264',
				payloadType : 99,
				clockRate   : 90000,
				parameters  :
				{
					packetizationMode : 1
				}
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 102,
				clockRate   : 90000,
				parameters  :
				{
					apt : 99
				}
			}
		]
	}
};
