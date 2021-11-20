"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parse = void 0;
const ScalabilityModeRegex = new RegExp('^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1})(_KEY)?');
function parse(scalabilityMode) {
    const match = ScalabilityModeRegex.exec(scalabilityMode || '');
    if (match) {
        return {
            spatialLayers: Number(match[1]),
            temporalLayers: Number(match[2]),
            ksvc: Boolean(match[3])
        };
    }
    else {
        return {
            spatialLayers: 1,
            temporalLayers: 1,
            ksvc: false
        };
    }
}
exports.parse = parse;
