const SPATIAL_LAYERS = new Set([ 'low', 'medium', 'high' ]);

/**
 * Validates given spatial layer.
 *
 * @param {String} spatialLayer
 *
 * @returns {Boolean}
 */
exports.isValidSpatialLayer = function(spatialLayer)
{
	return SPATIAL_LAYERS.has(spatialLayer);
};
