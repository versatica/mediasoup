'use strict';

const sdpTransform = require('sdp-transform');

class RTCSessionDescription
{
	/**
	 * RTCSessionDescription constructor.
	 * @param {object} [descriptionInitDict]
	 * @param {string} [descriptionInitDict.type] - "offer" / "answer".
	 * @param {string|object} [descriptionInitDict.sdp] - SDP string or sdp-transform object.
	 */
	constructor(descriptionInitDict)
	{
		const type = descriptionInitDict.type;

		switch (type)
		{
			case 'offer':
				break;
			case 'answer':
				break;
			default:
				throw new TypeError(`invalid type "${type}"`);
		}

		let sdp = descriptionInitDict.sdp;
		let parsed;

		switch (typeof sdp)
		{
			case 'string':
			{
				try
				{
					parsed = sdpTransform.parse(sdp);
				}
				catch (error)
				{
					throw new Error(`invalid sdp: ${error}`);
				}

				break;
			}

			case 'object':
			{
				parsed = sdp;
				try
				{
					sdp = sdpTransform.write(parsed);
				}
				catch (error)
				{
					throw new Error(`invalid sdp-transform object: ${error}`);
				}

				break;
			}

			default:
			{
				throw new TypeError('sdp must be a string or a sdp-transform object');
			}
		}

		// Private attributes.
		this._type = type;
		this._sdp = sdp;
		this._parsed = parsed;
	}

	get type()
	{
		return this._type;
	}

	get sdp()
	{
		return this._sdp;
	}

	get parsed()
	{
		return this._parsed;
	}

	serialize()
	{
		return {
			type : this._type,
			sdp  : this._sdp
		};
	}
}

module.exports = RTCSessionDescription;
