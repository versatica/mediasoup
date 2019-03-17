const supportedRtpCapabilities =
{
	codecs :
	[
		{
			kind      : 'audio',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 2
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/PCMU',
			preferredPayloadType : 0,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/PCMA',
			preferredPayloadType : 8,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/ISAC',
			clockRate : 32000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/ISAC',
			clockRate : 16000,
			channels  : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/G722',
			preferredPayloadType : 9,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/iLBC',
			clockRate : 8000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/SILK',
			clockRate : 24000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/SILK',
			clockRate : 16000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/SILK',
			clockRate : 12000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/SILK',
			clockRate : 8000,
			channels  : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 32000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 16000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 32000,
			channels             : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/telephone-event',
			clockRate : 48000,
			channels  : 1
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/telephone-event',
			clockRate : 32000,
			channels  : 1
		},

		{
			kind      : 'audio',
			mimeType  : 'audio/telephone-event',
			clockRate : 16000
		},
		{
			kind      : 'audio',
			mimeType  : 'audio/telephone-event',
			clockRate : 8000
		},
		{
			kind         : 'video',
			mimeType     : 'video/VP8',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind         : 'video',
			mimeType     : 'video/VP9',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 0,
				'profile-level-id'   : '42001f'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '42001f'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 0,
				'profile-level-id'   : '42e01f'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '42e01f'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '640032'
			},
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind         : 'video',
			mimeType     : 'video/H265',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			]
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
			kind             : 'audio',
			uri              : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', // eslint-disable-line max-len
			preferredId      : 3,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
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
			kind             : 'audio',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 5,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 5,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
			preferredId      : 6,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id',
			preferredId      : 7,
			preferredEncrypt : false
		}
	],
	fecMechanisms : []
};

module.exports = supportedRtpCapabilities;
