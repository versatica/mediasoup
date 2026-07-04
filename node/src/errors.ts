/**
 * Error indicating that something is not supported.
 */
export class UnsupportedError extends Error {
	constructor(message: string) {
		super(message);

		this.name = 'UnsupportedError';

		if (Error.hasOwnProperty('captureStackTrace')) {
			// Just in V8.
			Error.captureStackTrace(this, UnsupportedError);
		} else {
			this.stack = new Error(message).stack;
		}
	}
}

/**
 * Error produced when calling a method on a closed Worker.
 */
export class WorkerClosedError extends Error {
	constructor(message: string) {
		super(message);

		this.name = 'WorkerClosedError';

		if (Error.hasOwnProperty('captureStackTrace')) {
			// Just in V8.
			Error.captureStackTrace(this, WorkerClosedError);
		} else {
			this.stack = new Error(message).stack;
		}
	}
}

/**
 * Error indicating that a referenced entity doesn't exist.
 */
export class NotFoundError extends Error {
	constructor(message: string) {
		super(message);

		this.name = 'NotFoundError';

		if (Error.hasOwnProperty('captureStackTrace')) {
			// Just in V8.
			Error.captureStackTrace(this, NotFoundError);
		} else {
			this.stack = new Error(message).stack;
		}
	}
}
