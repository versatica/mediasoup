/**
 * Error indicating not support for something.
 */
class UnsupportedError extends Error
{
	constructor(message)
	{
		super(message);

		this.name = 'UnsupportedError';

		if (Error.hasOwnProperty('captureStackTrace')) // Just in V8.
			Error.captureStackTrace(this, UnsupportedError);
		else
			this.stack = (new Error(message)).stack;
	}
}

/**
 * Error produced when calling a method in an invalid state.
 */
class InvalidStateError extends Error
{
	constructor(message)
	{
		super(message);

		this.name = 'InvalidStateError';

		if (Error.hasOwnProperty('captureStackTrace')) // Just in V8.
			Error.captureStackTrace(this, InvalidStateError);
		else
			this.stack = (new Error(message)).stack;
	}
}

module.exports =
{
	UnsupportedError,
	InvalidStateError
};
