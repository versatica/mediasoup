'use strict';

module.exports = function(funcs)
{
	return new Promise((resolve, reject) =>
	{
		let i = 0;
		let len = funcs.length;

		function next()
		{
			if (i >= len)
				return resolve();

			let func = funcs[i];

			func()
				.then(() =>
				{
					i++;
					next();
				})
				.catch((error) =>
				{
					return reject(error);
				});
		}

		next();
	});
};
