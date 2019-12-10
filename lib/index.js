"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = __importDefault(require("./Logger"));
const EnhancedEventEmitter_1 = __importDefault(require("./EnhancedEventEmitter"));
const Worker_1 = __importDefault(require("./Worker"));
const utils = __importStar(require("./utils"));
const supportedRtpCapabilities_1 = __importDefault(require("./supportedRtpCapabilities"));
const types = __importStar(require("./types"));
exports.types = types;
/**
 * Expose mediasoup version.
 */
exports.version = '3.4.2';
/**
 * Expose parseScalabilityMode() function.
 */
var scalabilityModes_1 = require("./scalabilityModes");
exports.parseScalabilityMode = scalabilityModes_1.parse;
const logger = new Logger_1.default();
const observer = new EnhancedEventEmitter_1.default();
exports.observer = observer;
/**
 * Create a Worker.
 */
function createWorker({ logLevel = 'error', logTags, rtcMinPort = 10000, rtcMaxPort = 59999, dtlsCertificateFile, dtlsPrivateKeyFile, appData = {} } = {}) {
    return __awaiter(this, void 0, void 0, function* () {
        logger.debug('createWorker()');
        if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const worker = new Worker_1.default({
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
    });
}
exports.createWorker = createWorker;
/**
 * Get a cloned copy of the mediasoup supported RTP capabilities.
 */
function getSupportedRtpCapabilities() {
    return utils.clone(supportedRtpCapabilities_1.default);
}
exports.getSupportedRtpCapabilities = getSupportedRtpCapabilities;
