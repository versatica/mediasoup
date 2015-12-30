'use strict';

module.exports =
{
	TypeError: function(message)
	{
		let error = new TypeError(message || 'type error');

		error.status = 399;
		return error;
	},

	Closed: function(message)
	{
		let error = new Error(message || 'closed');

		error.status = 300;
		return error;
	},

	Timeout: function(message)
	{
		let error = new Error(message || 'timeout');

		error.status = 308;
		return error;
	},

	TooBig: function(message)
	{
		let error = new Error(message || 'too big');

		error.status = 313;
		return error;
	}
};
