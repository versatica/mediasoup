const ScalabilityModeRegex =
	new RegExp('^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1})(_KEY)?');

exports.parse = function(scalabilityMode)
{
	const match = ScalabilityModeRegex.exec(scalabilityMode);

	if (!match)
	{
		return {
			spatialLayers  : 1,
			temporalLayers : 1,
			ksvc           : false
		};
	}

	return {
		spatialLayers  : Number(match[1]),
		temporalLayers : Number(match[2]),
		ksvc           : Boolean(match[3])
	};
};
