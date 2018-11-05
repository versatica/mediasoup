module.exports = function(funcs)
{
	return new Promise((resolve, reject) =>
	{
		let i = 0;
		const len = funcs.length;

		function next()
		{
			if (i >= len)
				return resolve();

			const func = funcs[i];

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
