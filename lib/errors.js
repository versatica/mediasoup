'use strict';

module.exports =
{
	TypeErrorPromise: function(message)
	{
		var error = new TypeError(message || 'type error');

		error.status = 399;
		return Promise.reject(error);
	},

	ClosedPromise: function(message)
	{
		var error = new Error(message || 'closed');

		error.status = 300;
		return Promise.reject(error);
	},

	TooBigPromise: function(message)
	{
		var error = new Error(message || 'too big');

		error.status = 313;
		return Promise.reject(error);
	},

	ClosedError: function(message)
	{
		var error = new Error(message || 'closed');

		error.status = 300;
		return error;
	},

	TimeoutError: function(message)
	{
		var error = new Error(message || 'timeout');

		error.status = 308;
		return error;
	}
};
