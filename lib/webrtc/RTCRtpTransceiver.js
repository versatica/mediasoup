'use strict';

const RTCDictionaries = require('./RTCDictionaries');

class RTCRtpTransceiver
{
	constructor(kind)
	{
		// kind attribute.
		this._kind = kind;

		// mid attribute.
		this._mid = null;

		// RtpReceiver instance.
		this._receiver = null;

		// RtpSender instance.
		this._sender = null;

		// stopped attribute.
		this._stopped = false;

		// direction attribute.
		this._direction = RTCDictionaries.RTCRtpTransceiverDirection.inactive;

		// Bind events.
		this._onReceiverClose = this._onReceiverClose.bind(this);
		this._onSenderClose = this._onSenderClose.bind(this);
		// TODO: Add more.
	}

	get kind()
	{
		return this._kind;
	}

	get mid()
	{
		return this._mid;
	}

	set mid(mid)
	{
		if (this._stopped)
			return;

		this._mid = mid;

		// TODO: pass it to the receiver and sender.
	}

	get receiver()
	{
		return this._receiver;
	}

	set receiver(receiver)
	{
		if (this._stopped)
			return;

		if (this._receiver)
			this._unhandleReceiverEvents(this._receiver);

		if (receiver)
		{
			this._receiver = receiver;
			this._handleReceiverEvents(receiver);

			// Update direction.
			switch (this._direction)
			{
				case RTCDictionaries.RTCRtpTransceiverDirection.inactive:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.recvonly;
					break;
				case RTCDictionaries.RTCRtpTransceiverDirection.sendonly:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.sendrecv;
					break;
			}

			// TODO: set mid?
		}
		else
		{
			this._receiver = null;

			// Update direction.
			switch (this._direction)
			{
				case RTCDictionaries.RTCRtpTransceiverDirection.recvonly:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.inactive;
					break;
				case RTCDictionaries.RTCRtpTransceiverDirection.sendrecv:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.sendonly;
					break;
			}
		}
	}

	get sender()
	{
		return this._sender;
	}

	set sender(sender)
	{
		if (this._stopped)
			return;

		if (this._sender)
			this._unhandleSenderEvents(this._sender);

		if (sender)
		{
			this._sender = sender;
			this._handleSenderEvents(sender);

			// Update direction.
			switch (this._direction)
			{
				case RTCDictionaries.RTCRtpTransceiverDirection.inactive:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.sendonly;
					break;
				case RTCDictionaries.RTCRtpTransceiverDirection.recvonly:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.sendrecv;
					break;
			}

			// TODO: set mid.
		}
		else
		{
			this._sender = null;

			// Update direction.
			switch (this._direction)
			{
				case RTCDictionaries.RTCRtpTransceiverDirection.sendonly:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.inactive;
					break;
				case RTCDictionaries.RTCRtpTransceiverDirection.sendrecv:
					this._direction = RTCDictionaries.RTCRtpTransceiverDirection.recvonly;
					break;
			}
		}
	}

	get stopped()
	{
		return this._stopped;
	}

	get direction()
	{
		return this._direction;
	}

	stop()
	{
		if (this._stopped)
			return;

		// Close receiver.
		if (this._receiver)
			this._receiver.close();

		// Close sender.
		if (this._sender)
			this._sender.close();

		// Finally set stopped flag.
		this._stopped = true;
	}

	_handleReceiverEvents(receiver)
	{
		receiver.on('close', this._onReceiverClose);
	}

	_unhandleReceiverEvents(receiver)
	{
		receiver.removeListener('close', this._onReceiverClose);
	}

	_handleSenderEvents(sender)
	{
		sender.on('close', this._onSenderClose);
	}

	_unhandleSenderEvents(sender)
	{
		sender.removeListener('close', this._onSenderClose);
	}

	_onReceiverClose()
	{
		this.receiver = null;
	}

	_onSenderClose()
	{
		this.sender = null;
	}
}

module.exports = RTCRtpTransceiver;
