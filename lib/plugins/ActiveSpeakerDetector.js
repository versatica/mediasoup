const Logger = require('../Logger');
const EnhancedEventEmitter = require('../EnhancedEventEmitter');

const REQUIRED_MIN_AUDIO_LEVEL = -80; // dB.
const REQUIRED_MAX_NUM_REPORTS = 4;

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

		// Latest max audio level reports.
		// @type {Array}
		this._reports = [];

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

		let previousActiveProducer = this._activeProducer;
		let requiredNumReports;

		if (
			previousActiveProducer &&
			!previousActiveProducer.closed &&
			!previousActiveProducer.paused
		)
		{
			requiredNumReports = REQUIRED_MAX_NUM_REPORTS;
		}
		else
		{
			previousActiveProducer = null;
			requiredNumReports = 1;
			this._reports = [];
		}

		const reports = this._reports;

		// Insert the loudest audio level report into the reports array.
		// NOTE: This may be undefined, meaning that nobody is producing audio.
		reports.push(levels[0]);

		// No more than REQUIRED_MAX_NUM_REPORTS.
		if (reports.length > REQUIRED_MAX_NUM_REPORTS)
		{
			reports.shift();
		}
		// No less than requiredNumReports.
		else if (reports.length < requiredNumReports)
		{
			this._mayUpdateActiveSpeaker(previousActiveProducer);

			return;
		}

		// If the same Producer was the loudest speaker in the last N reports, make
		// it the active speaker.

		let currentActiveProducer;
		const numReports = reports.length;

		for (let i = numReports - 1; i >= numReports - requiredNumReports; --i)
		{
			const report = reports[i];

			if (!report)
			{
				currentActiveProducer = null;

				break;
			}

			const { producer, audioLevel } = report;

			if (producer.closed || producer.paused || audioLevel < REQUIRED_MIN_AUDIO_LEVEL)
			{
				currentActiveProducer = null;

				break;
			}

			if (currentActiveProducer && producer !== currentActiveProducer)
			{
				currentActiveProducer = null;

				break;
			}

			currentActiveProducer = producer;
		}

		// If there is an effective new audio Producer, make it the active one.
		if (currentActiveProducer)
		{
			this._mayUpdateActiveSpeaker(currentActiveProducer);
		}
		// Otherwise check the previous active speaker.
		else
		{
			this._mayUpdateActiveSpeaker(previousActiveProducer);
		}
	}

	_mayUpdateActiveSpeaker(activeProducer)
	{
		if (this._activeProducer === activeProducer)
			return;

		this._activeProducer = activeProducer;

		if (this._activeProducer)
		{
			const peer = this._activeProducer.peer;

			logger.debug(
				'emitting "activespeakerchange" [peerName:"%s"]', peer.name);

			this.safeEmit(
				'activespeakerchange', peer, this._activeProducer);
		}
		else
		{
			logger.debug(
				'emitting "activespeakerchange" [peerName:null]');

			this.safeEmit('activespeakerchange', undefined, undefined);
		}
	}
}

module.exports = ActiveSpeakerDetector;
