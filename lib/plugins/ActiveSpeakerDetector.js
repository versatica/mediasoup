'use strict';

const Logger = require('../Logger');
const EnhancedEventEmitter = require('../EnhancedEventEmitter');

const MIN_AUDIO_LEVEL = -70; // dB.

const logger = new Logger('ActiveSpeakerDetector');

class ActiveSpeakerDetector extends EnhancedEventEmitter
{
	constructor(room)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Handled Room.
		// @type {Room}
		this._room = room;

		// Latest max audio levels.
		// @type {Array}
		this._latestMaxAudioLevels = [];

		// Current active Producer.
		// @type {Producer}
		this._activeProducer = null;

		// Bind the 'audiolevels' listener so we can remove it on closure.
		this._onRoomAudioLevels = this._onRoomAudioLevels.bind(this);

		this._handleRoom();
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the ActiveSpeakerDetector.
	 */
	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		if (!this._room.closed)
			this._room.removeListener('audiolevels', this._onRoomAudioLevels);

		this.safeEmit('close');
	}

	_handleRoom()
	{
		const room = this._room;

		room.on('close', () => this.close());

		room.on('audiolevels', this._onRoomAudioLevels);
	}

	_onRoomAudioLevels(levels)
	{
		if (this._closed)
			return;

		const times = 2;
		const latest = this._latestMaxAudioLevels;

		// Insert the loudest audio level entry into the latest array.
		// NOTE: This may be undefined, meaning that nobody is producing audio.
		latest.push(levels[0]);

		// No more than times.
		if (latest.length > times)
			latest.shift();
		// No less than times.
		else if (latest.length < times)
			return;

		// If the same Producer was the loudest speaker in the last 4 reports, make
		// it the active speaker.

		let newActiveProducer;
		let newAudioLevel;
		const latestLen = latest.length;

		for (let i = latestLen - 1; i >= latestLen - times; --i)
		{
			const entry = latest[i];

			if (!entry || entry.audioLevel < MIN_AUDIO_LEVEL)
			{
				newActiveProducer = undefined;

				break;
			}

			const { producer, audioLevel } = entry;

			if (producer.closed || producer.paused)
			{
				newActiveProducer = undefined;

				break;
			}

			if (newActiveProducer && producer !== newActiveProducer)
			{
				newActiveProducer = undefined;

				break;
			}

			newActiveProducer = producer;
			newAudioLevel = audioLevel;
		}

		// If there is an effective new audio Producer, make it the active one.
		if (newActiveProducer && newActiveProducer !== this._activeProducer)
		{
			this._activeProducer = newActiveProducer;

			const peer = this._activeProducer.peer;

			logger.debug(
				'emitting "activespeakerchange" [peerName:"%s"]', peer.name);

			this.safeEmit(
				'activespeakerchange', peer, this._activeProducer, newAudioLevel);
		}
		// Otherwise, if the previous active speaker (if any) is closed or paused,
		// unset it.
		else if (
			this._activeProducer &&
			(this._activeProducer.closed || this._activeProducer.paused))
		{
			this._activeProducer = undefined;

			logger.debug(
				'emitting "activespeakerchange" [peerName:null]');

			this.safeEmit('activespeakerchange', null, null);
		}
	}
}

module.exports = ActiveSpeakerDetector;
