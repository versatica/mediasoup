'use strict';

let supportedRtpCapabilities =
{
	codecs :
	[
		{
			kind         : 'audio',
			name         : 'audio/opus',
			clockRate    : 48000,
			numChannels  : 2,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/PCMU',
			clockRate    : 8000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/PCMA',
			clockRate    : 8000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/ISAC',
			clockRate    : 32000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/ISAC',
			clockRate    : 16000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/G722',
			clockRate    : 8000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/CN',
			clockRate    : 32000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/CN',
			clockRate    : 16000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/CN',
			clockRate    : 8000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/CN',
			clockRate    : 32000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/telephone-event',
			clockRate    : 48000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/telephone-event',
			clockRate    : 32000,
			rtcpFeedback : []
		},

		{
			kind         : 'audio',
			name         : 'audio/telephone-event',
			clockRate    : 16000,
			rtcpFeedback : []
		},
		{
			kind         : 'audio',
			name         : 'audio/telephone-event',
			clockRate    : 8000,
			rtcpFeedback : []
		},
		{
			kind         : 'video',
			name         : 'video/VP8',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    }, // Locally consumed.
				{ type: 'nack', parameter: 'pli'  }, // Bypassed.
				{ type: 'nack', parameter: 'sli'  }, // Bypassed.
				{ type: 'nack', parameter: 'rpsi' }, // Bypassed.
				{ type: 'nack', parameter: 'app'  }, // Bypassed.
				{ type: 'ccm',  parameter: 'fir'  }, // Bypassed.
				{ type: 'ack',  parameter: 'rpsi' }, // Bypassed.
				{ type: 'ack',  parameter: 'app'  }  // Bypassed.
			]
		},
		{
			kind         : 'video',
			name         : 'video/VP9',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    },
				{ type: 'nack', parameter: 'pli'  },
				{ type: 'nack', parameter: 'sli'  },
				{ type: 'nack', parameter: 'rpsi' },
				{ type: 'nack', parameter: 'app'  },
				{ type: 'ccm',  parameter: 'fir'  },
				{ type: 'ack',  parameter: 'rpsi' },
				{ type: 'ack',  parameter: 'app'  }
			]
		},
		{
			kind         : 'video',
			name         : 'video/H264',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    },
				{ type: 'nack', parameter: 'pli'  },
				{ type: 'nack', parameter: 'sli'  },
				{ type: 'nack', parameter: 'rpsi' },
				{ type: 'nack', parameter: 'app'  },
				{ type: 'ccm',  parameter: 'fir'  },
				{ type: 'ack',  parameter: 'rpsi' },
				{ type: 'ack',  parameter: 'app'  }
			],
			parameters :
			{
				packetizationMode : 0
			}
		},
		{
			kind         : 'video',
			name         : 'video/H264',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    },
				{ type: 'nack', parameter: 'pli'  },
				{ type: 'nack', parameter: 'sli'  },
				{ type: 'nack', parameter: 'rpsi' },
				{ type: 'nack', parameter: 'app'  },
				{ type: 'ccm',  parameter: 'fir'  },
				{ type: 'ack',  parameter: 'rpsi' },
				{ type: 'ack',  parameter: 'app'  }
			],
			parameters :
			{
				packetizationMode : 1
			}
		},
		{
			kind         : 'video',
			name         : 'video/H265',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    },
				{ type: 'nack', parameter: 'pli'  },
				{ type: 'nack', parameter: 'sli'  },
				{ type: 'nack', parameter: 'rpsi' },
				{ type: 'nack', parameter: 'app'  },
				{ type: 'ccm',  parameter: 'fir'  },
				{ type: 'ack',  parameter: 'rpsi' },
				{ type: 'ack',  parameter: 'app'  }
			],
			parameters :
			{
				packetizationMode : 0
			}
		},
		{
			kind         : 'video',
			name         : 'video/H265',
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack'                    },
				{ type: 'nack', parameter: 'pli'  },
				{ type: 'nack', parameter: 'sli'  },
				{ type: 'nack', parameter: 'rpsi' },
				{ type: 'nack', parameter: 'app'  },
				{ type: 'ccm',  parameter: 'fir'  },
				{ type: 'ack',  parameter: 'rpsi' },
				{ type: 'ack',  parameter: 'app'  }
			],
			parameters :
			{
				packetizationMode : 1
			}
		}
	],
	headerExtensions :
	[
		{
			kind             : '',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 1,
			preferredEncrypt : false
		}
	],
	fecMechanisms : []
};

module.exports = supportedRtpCapabilities;
