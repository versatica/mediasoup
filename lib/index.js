"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    Object.defineProperty(o, k2, { enumerable: true, get: function() { return m[k]; } });
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.getSupportedRtpCapabilities = exports.createWorker = exports.observer = exports.version = exports.types = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const Worker_1 = require("./Worker");
const utils = __importStar(require("./utils"));
const supportedRtpCapabilities_1 = require("./supportedRtpCapabilities");
const types = __importStar(require("./types"));
exports.types = types;
/**
 * Expose mediasoup version.
 */
exports.version = '3.5.13';
/**
 * Expose parseScalabilityMode() function.
 */
var scalabilityModes_1 = require("./scalabilityModes");
Object.defineProperty(exports, "parseScalabilityMode", { enumerable: true, get: function () { return scalabilityModes_1.parse; } });
const logger = new Logger_1.Logger();
const observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
exports.observer = observer;
/**
 * Create a Worker.
 */
async function createWorker({ logLevel = 'error', logTags, rtcMinPort = 10000, rtcMaxPort = 59999, dtlsCertificateFile, dtlsPrivateKeyFile, appData = {} } = {}) {
    logger.debug('createWorker()');
    if (appData && typeof appData !== 'object')
        throw new TypeError('if given, appData must be an object');
    const worker = new Worker_1.Worker({
        logLevel,
        logTags,
        rtcMinPort,
        rtcMaxPort,
        dtlsCertificateFile,
        dtlsPrivateKeyFile,
        appData
    });
    return new Promise((resolve, reject) => {
        worker.on('@success', () => {
            // Emit observer event.
            observer.safeEmit('newworker', worker);
            resolve(worker);
        });
        worker.on('@failure', reject);
    });
}
exports.createWorker = createWorker;
/**
 * Get a cloned copy of the mediasoup supported RTP capabilities.
 */
function getSupportedRtpCapabilities() {
    return utils.clone(supportedRtpCapabilities_1.supportedRtpCapabilities);
}
exports.getSupportedRtpCapabilities = getSupportedRtpCapabilities;
