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
				kind        : 'video',
				name        : 'video/vp8',
				payloadType : 110,
				clockRate   : 90000
			},
			{
				kind        : 'video',
				name        : 'video/h264',
				clockRate   : 90000,
				payloadType : 112,
				parameters  :
				{
					packetizationMode : 1
				}
			},
			{
				kind        : 'depth',
				name        : 'video/vp8',
				payloadType : 120,
				clockRate   : 90000
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
				payloadType : 100,
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
				kind         : 'video',
				name         : 'video/VP8',
				payloadType  : 110,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'nack', parameter: 'sli' },
					{ type: 'nack', parameter: 'rpsi' },
					{ type: 'nack', parameter: 'app' },
					{ type: 'ccm', parameter: 'fir' },
					{ type: 'ack', parameter: 'rpsi' },
					{ type: 'ack', parameter: 'app' }
				]
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 97,
				clockRate   : 90000,
				parameters  :
				{
					apt : 110
				}
			},
			{
				kind        : 'video',
				name        : 'video/H264',
				payloadType : 111,
				clockRate   : 90000,
				parameters  :
				{
					packetizationMode : 0
				},
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'nack', parameter: 'sli' },
					{ type: 'nack', parameter: 'rpsi' },
					{ type: 'nack', parameter: 'app' },
					{ type: 'ccm', parameter: 'fir' },
					{ type: 'ack', parameter: 'rpsi' },
					{ type: 'ack', parameter: 'app' }
				]
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 98,
				clockRate   : 90000,
				parameters  :
				{
					apt : 111
				}
			},
			{
				kind        : 'video',
				name        : 'video/H264',
				payloadType : 112,
				clockRate   : 90000,
				parameters  :
				{
					packetizationMode : 1
				},
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'nack', parameter: 'sli' },
					{ type: 'nack', parameter: 'rpsi' },
					{ type: 'nack', parameter: 'app' },
					{ type: 'ccm', parameter: 'fir' },
					{ type: 'ack', parameter: 'rpsi' },
					{ type: 'ack', parameter: 'app' }
				]
			},
			{
				kind        : 'video',
				name        : 'video/rtx',
				payloadType : 99,
				clockRate   : 90000,
				parameters  :
				{
					apt : 112
				}
			}
		],
		headerExtensions :
		[
			{
				kind             : 'audio',
				uri              : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				preferredId      : 1,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:ietf:params:rtp-hdrext:toffset',
				preferredId      : 2,
				preferredEncrypt : false
			},
			{
				kind             : '',
				uri              : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', // eslint-disable-line max-len
				preferredId      : 3,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:3gpp:video-orientation',
				preferredId      : 4,
				preferredEncrypt : false
			},
			{
				kind             : '',
				uri              : 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
				preferredId      : 5,
				preferredEncrypt : false
			}
		],
		fecMechanisms : []
	}
};
