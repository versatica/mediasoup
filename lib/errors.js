'use strict';

function createErrorClass(name)
{
	let klass = class extends Error
	{
		constructor()
		{
			super(...arguments);

			// Override `name` property value and make it non enumerable
			Object.defineProperty(this, 'name', { value: name });
		}
	};

	return klass;
}

module.exports =
{
	InvalidStateError : createErrorClass('InvalidStateError')
};
