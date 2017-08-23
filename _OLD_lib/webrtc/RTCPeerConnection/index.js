'use strict';

const RTCPeerConnectionUnifiedPlan = require('./RTCPeerConnectionUnifiedPlan');
const RTCPeerConnectionPlanB = require('./RTCPeerConnectionPlanB');
const Peer = require('../../Peer');

class RTCPeerConnection
{
	/**
	 * RTCPeerConnection constructor.ç
	 * @param {object} options
	 * @param {Peer} options.peer - Peer instance.
	 * @param {object} [options.transportOptions] - Options for Transport.
	 * @param {boolean} [options.usePlanB] - Use Plan-B rather than Unified-Plan.
	 * @param {object} [options.bandwidth]
	 * @param {number} [options.bandwidth.audio] - Maximum bandwidth for audio streams.
	 * @param {number} [options.bandwidth.video] - Maximum bandwidth for video streams.
	 */
	constructor(options)
	{
		options = options || {};

		if (!(options.peer instanceof Peer))
			throw new TypeError('missing/wrong options.peer');

		if (!options.usePlanB)
			return new RTCPeerConnectionUnifiedPlan(options);
		else
			return new RTCPeerConnectionPlanB(options);
	}
}

module.exports = RTCPeerConnection;
