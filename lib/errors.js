'use strict';

function createErrorClass(name)
{
	const klass = class extends Error
	{
		constructor(message)
		{
			super(message);

			// Override `name` property value and make it non enumerable.
			Object.defineProperty(this, 'name', { value: name });
		}
	};

	return klass;
}

module.exports =
{
	InvalidStateError : createErrorClass('InvalidStateError')
};
