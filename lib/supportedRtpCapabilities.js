const supportedRtpCapabilities =
{
	codecs :
	[
		{
			kind      : 'audio',
			name      : 'opus',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 2
		},
		{
			kind                 : 'audio',
			name                 : 'PCMU',
			mimeType             : 'audio/PCMU',
			preferredPayloadType : 0,
			clockRate            : 8000,
			channels             : 1,
			rtcpFeedback         : []
		},
		{
			kind                 : 'audio',
			name                 : 'PCMA',
			mimeType             : 'audio/PCMA',
			preferredPayloadType : 8,
			clockRate            : 8000,
			channels             : 1,
			rtcpFeedback         : []
		},
		{
			kind         : 'audio',
			name         : 'ISAC',
			mimeType     : 'audio/ISAC',
			clockRate    : 32000,
			channels     : 1,
			rtcpFeedback : []
		},
		{
			kind      : 'audio',
			name      : 'ISAC',
			mimeType  : 'audio/ISAC',
			clockRate : 16000,
			channels  : 1
		},
		{
			kind                 : 'audio',
			name                 : 'G722',
			mimeType             : 'audio/G722',
			preferredPayloadType : 9,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind      : 'audio',
			name      : 'iLBC',
			mimeType  : 'audio/iLBC',
			clockRate : 8000,
			channels  : 1
		},
		{
			kind      : 'audio',
			name      : 'SILK',
			mimeType  : 'audio/SILK',
			clockRate : 24000,
			channels  : 1
		},
		{
			kind      : 'audio',
			name      : 'SILK',
			mimeType  : 'audio/SILK',
			clockRate : 16000,
			channels  : 1
		},
		{
			kind      : 'audio',
			name      : 'SILK',
			mimeType  : 'audio/SILK',
			clockRate : 12000,
			channels  : 1
		},
		{
			kind      : 'audio',
			name      : 'SILK',
			mimeType  : 'audio/SILK',
			clockRate : 8000,
			channels  : 1
		},
		{
			kind                 : 'audio',
			name                 : 'CN',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 32000,
			channels  : 1
		},
		{
			kind                 : 'audio',
			name                 : 'CN',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 16000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			name                 : 'CN',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 8000,
			channels             : 1
		},
		{
			kind                 : 'audio',
			name                 : 'CN',
			mimeType             : 'audio/CN',
			preferredPayloadType : 13,
			clockRate            : 32000,
			channels             : 1
		},
		{
			kind      : 'audio',
			name      : 'telephone-event',
			mimeType  : 'audio/telephone-event',
			clockRate : 48000,
			channels  : 1
		},
		{
			kind      : 'audio',
			name      : 'telephone-event',
			mimeType  : 'audio/telephone-event',
			clockRate : 32000,
			channels  : 1
		},

		{
			kind      : 'audio',
			name      : 'telephone-event',
			mimeType  : 'audio/telephone-event',
			clockRate : 16000
		},
		{
			kind      : 'audio',
			name      : 'telephone-event',
			mimeType  : 'audio/telephone-event',
			clockRate : 8000
		},
		{
			kind         : 'video',
			name         : 'VP8',
			mimeType     : 'video/VP8',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack' }, // Locally consumed.
				{ type: 'nack', parameter: 'pli' }, // Bypassed.
				{ type: 'nack', parameter: 'sli' }, // Bypassed.
				{ type: 'nack', parameter: 'rpsi' }, // Bypassed.
				{ type: 'nack', parameter: 'app' }, // Bypassed.
				{ type: 'ccm', parameter: 'fir' }, // Bypassed.
				{ type: 'ack', parameter: 'rpsi' }, // Bypassed.
				{ type: 'ack', parameter: 'app' }, // Bypassed.
				{ type: 'goog-remb' } // Locally generated.
			]
		},
		{
			kind         : 'video',
			name         : 'VP9',
			mimeType     : 'video/VP9',
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
				{ type: 'ack', parameter: 'app' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			name       : 'H264',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 0
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
				{ type: 'ack', parameter: 'app' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind       : 'video',
			name       : 'H264',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1
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
				{ type: 'ack', parameter: 'app' },
				{ type: 'goog-remb' }
			]
		},
		{
			kind         : 'video',
			name         : 'H265',
			mimeType     : 'video/H265',
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
				{ type: 'ack', parameter: 'app' },
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
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
			preferredId      : 5,
			preferredEncrypt : false
		}
	],
	fecMechanisms : []
};

module.exports = supportedRtpCapabilities;
