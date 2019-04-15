const ScalabilityModeRegex = new RegExp('^L(\\d+)T(\\d+)');

exports.parse = function(scalabilityMode)
{
	const match = ScalabilityModeRegex.exec(scalabilityMode);

	if (!match)
		return { spatialLayers: 1, temporalLayers: 1 };

	return {
		spatialLayers  : Number(match[1]),
		temporalLayers : Number(match[2])
	};
};
