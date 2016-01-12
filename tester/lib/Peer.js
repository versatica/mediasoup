'use strict';

const EventEmitter = require('events').EventEmitter;

const debug = require('debug');
const protooClient = require('protoo-client');
const rtcninja = require('rtcninja');
const sdpTransform = require('sdp-transform');

class Peer extends EventEmitter
{
	constructor(wsUrl, username, uuid)
	{
		super();

		this._debug = debug(`mediasoup-tester:Peer:${username}`);
		this._debugerror = debug(`mediasoup-tester:ERROR:Peer:${username}`);
		this._debugerror.log = console.error.bind(console);

		this._connected = false;
		this._stream = null;

		this._debug('new()');

		// protoo-client instance
		this._protoo = protooClient(
			{
				url : `${wsUrl}/?username=${username}&uuid=${uuid}`
			});

		this._protoo.on('online', () =>
		{
			this._debug('protoo online');

			this._connected = true;
			mayRun.call(this);
		});

		// PeerConnection instance
		this._pc = new rtcninja.RTCPeerConnection({ iceServers: [] });

		rtcninja.getUserMedia(
			{ audio: true, video: false },
			// callback
			((stream) =>
			{
				this._debug('got local stream');

				this._stream = stream;
				mayRun.call(this);
			}),
			// errback
			((error) =>
			{
				throw new Error(error);
			})
		);

		function mayRun()
		{
			if (this._connected && this._stream)
			{
				this._debug('emitting "ready" event');

				// this._pc.addStream(this._stream);

				this.emit('ready');
			}
		}
	}

	close()
	{
		this._debug('close()');

		this.closeProtoo();
		this.closePeerConnection();
	}

	closeProtoo()
	{
		this._debug('closeProtoo()');

		this._protoo.close();
	}

	closePeerConnection()
	{
		this._debug('closePeerConnection()');

		try
		{
			this._pc.close();
		}
		catch (error)
		{}

		try
		{
			this._stream.getTracks().forEach((track) => track.stop());
		}
		catch (error)
		{}
	}

	testTransport()
	{
		this._debug('testTransport()');

		this._pc.createOffer(
			// callback
			(offer) =>
			{
				this._pc.setLocalDescription(offer,
					// callback
					() =>
					{
						this._debug('this._pc.iceGatheringState: %s', this._pc.iceGatheringState);

						if (this._pc.iceGatheringState === 'complete')
						{
							sendRequest.call(this);
						}
						else
						{
							this._pc.onicecandidate = (event) =>
							{
								if (!event.candidate)
									sendRequest.call(this);
							};
						}
					},
					// errback
					(error) => { throw new Error(error); }
				);
			},
			// errback
			(error) => { throw new Error(error); },
			// options
			{ offerToReceiveAudio: true, offerToReceiveVideo: true }
		);

		function sendRequest()
		{
			this._debug('sendRequest()');

			let offer = this._pc.localDescription;
			let data =
			{
				type : offer.type,
				sdp  : offer.sdp
			};

			this._debug('SDP OFFER:\n%s', this._pc.localDescription.sdp);

			// TODO: TMP
			console.warn(sdpTransform.parse(this._pc.localDescription.sdp));

			this._protoo.send('put', '/test-transport', data, (res, error) =>
			{
				if (res && res.isAccept)
				{
					this._debug('got success reply from "test-transport"');

					this._debug('SDP ANSWER:\n%s', res.data.sdp);

					let answer = new rtcninja.RTCSessionDescription({ type: 'answer', sdp: res.data.sdp });

					this._pc.setRemoteDescription(answer,
						// callback
						() => { this._debug('setRemoteDescription() succeeded'); },
						// errback
						(error) =>
						{
							this._debugerror('setRemoteDescription() failed: %o', error);

							throw error;
						}
					);
				}
				else if (error || res.isReject)
				{
					throw new Error(error || new Error(res.reason));
				}
			});
		}
	}
}

module.exports = Peer;
