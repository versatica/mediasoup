"use strict";
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const randomNumber = __importStar(require("random-number"));
const randomNumberGenerator = randomNumber.generator({
    min: 100000000,
    max: 999999999,
    integer: true
});
exports.generateRandomNumber = randomNumberGenerator;
/**
 * Clones the given object/array.
 */
function clone(obj) {
    if (typeof obj !== 'object')
        return {};
    return JSON.parse(JSON.stringify(obj));
}
exports.clone = clone;
