'use strict';

const RTCPeerConnectionUnifiedPlan = require('./RTCPeerConnectionUnifiedPlan');
const RTCPeerConnectionPlanB = require('./RTCPeerConnectionPlanB');

class RTCPeerConnection
{
	/**
	 * RTCPeerConnection constructor.
	 * @param {Room} room
	 * @param {string} peerName
	 * @param {object} [options]
	 * @param {object} [options.transportOptions] - Options for Transport.
	 * @param {boolean} [options.usePlanB] - Use Plan-B rather than Unified-Plan.
	 * @param {object} [options.bandwidth]
	 * @param {number} [options.bandwidth.audio] - Maximum bandwidth for audio streams.
	 * @param {number} [options.bandwidth.video] - Maximum bandwidth for video streams.
	 */
	constructor(room, peerName, options)
	{
		options = options || {};

		if (!options.usePlanB)
			return new RTCPeerConnectionUnifiedPlan(room, peerName, options);
		else
			return new RTCPeerConnectionPlanB(room, peerName, options);
	}
}

module.exports = RTCPeerConnection;
