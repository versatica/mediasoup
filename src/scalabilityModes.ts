const ScalabilityModeRegex =
	new RegExp('^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1})(_KEY)?');

export type ScalabilityMode =
{
	spatialLayers: number;
	temporalLayers: number;
	ksvc: boolean;
}

export function parse(scalabilityMode: string): ScalabilityMode
{
	const match = ScalabilityModeRegex.exec(scalabilityMode);

	if (match)
	{
		return {
			spatialLayers  : parseInt(match[1]),
			temporalLayers : parseInt(match[2]),
			ksvc           : !!match[3]
		};
	}
	else
	{
		return {
			spatialLayers  : 1,
			temporalLayers : 1,
			ksvc           : false
		};
	}
}
