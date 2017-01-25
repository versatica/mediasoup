'use strict';

const EventEmitter = require('events').EventEmitter;
// const sdpTransform = require('sdp-transform');
// const randomString = require('random-string');
const logger = require('../../logger');
// const extra = require('../../extra');
// const InvalidStateError = require('../../errors').InvalidStateError;
// const RTCDictionaries = require('../RTCDictionaries');
// const RTCSessionDescription = require('../RTCSessionDescription');
// const RTCRtpTransceiver = require('../RTCRtpTransceiver');

// const NEGOTIATION_NEEDED_DELAY = 1000;

class RTCPeerConnectionUnifiedPlan extends EventEmitter
{
	constructor(room, peerName, options)
	{
		super();
		this.setMaxListeners(Infinity);

		this._logger = logger(`webrtc:RTCPeerConnectionUnifiedPlan:${peerName}`);
		this._logger.debug('constructor() [options:%o]', options);
	}
}

module.exports = RTCPeerConnectionUnifiedPlan;
