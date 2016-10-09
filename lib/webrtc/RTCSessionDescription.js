'use strict';

class RTCSessionDescription
{
	/**
	 * RTCSessionDescription constructor.
	 * @param  {object} [options]
	 * @param  {string} [options.type] - "offer" / "answer".
	 * @param  {string} [options.sdp] - SDP.
	 */
	constructor(descriptionInitDict)
	{
		let type = descriptionInitDict.type;

		if (['offer', 'answer'].indexOf(type) === -1)
			throw new TypeError(`invalid type "${type}"`);

		let sdp = descriptionInitDict.sdp;

		if (typeof sdp !== 'string')
			throw new TypeError('sdp must be a string');

		// Public r/w properties.
		this.type = type;
		this.sdp = sdp;
	}
}

module.exports = RTCSessionDescription;
