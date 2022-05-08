"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.AudioLevelObserver = void 0;
const Logger_1 = require("./Logger");
const RtpObserver_1 = require("./RtpObserver");
const logger = new Logger_1.Logger('AudioLevelObserver');
class AudioLevelObserver extends RtpObserver_1.RtpObserver {
    /**
     * @private
     * @emits volumes - (volumes: AudioLevelObserverVolume[])
     * @emits silence
     */
    constructor(params) {
        super(params);
        this.handleWorkerNotifications();
    }
    /**
     * Observer.
     *
     * @emits close
     * @emits pause
     * @emits resume
     * @emits addproducer - (producer: Producer)
     * @emits removeproducer - (producer: Producer)
     * @emits volumes - (volumes: AudioLevelObserverVolume[])
     * @emits silence
     */
    get observer() {
        return super.observer;
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.rtpObserverId, (event, data) => {
            switch (event) {
                case 'volumes':
                    {
                        // Get the corresponding Producer instance and remove entries with
                        // no Producer (it may have been closed in the meanwhile).
                        const volumes = data
                            .map(({ producerId, volume }) => ({
                            producer: this.getProducerById(producerId),
                            volume
                        }))
                            .filter(({ producer }) => producer);
                        if (volumes.length > 0) {
                            this.safeEmit('volumes', volumes);
                            // Emit observer event.
                            this.observer.safeEmit('volumes', volumes);
                        }
                        break;
                    }
                case 'silence':
                    {
                        this.safeEmit('silence');
                        // Emit observer event.
                        this.observer.safeEmit('silence');
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
    }
}
exports.AudioLevelObserver = AudioLevelObserver;
