"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseSrtpParameters = void 0;
function parseSrtpParameters(binary) {
    return {
        cryptoSuite: binary.cryptoSuite(),
        keyBase64: binary.keyBase64()
    };
}
exports.parseSrtpParameters = parseSrtpParameters;
