import * as flatbuffers from 'flatbuffers';
export declare class ChannelMessageHandlers {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ChannelMessageHandlers;
    static getRootAsChannelMessageHandlers(bb: flatbuffers.ByteBuffer, obj?: ChannelMessageHandlers): ChannelMessageHandlers;
    static getSizePrefixedRootAsChannelMessageHandlers(bb: flatbuffers.ByteBuffer, obj?: ChannelMessageHandlers): ChannelMessageHandlers;
    channelRequestHandlers(index: number): string;
    channelRequestHandlers(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    channelRequestHandlersLength(): number;
    payloadChannelRequestHandlers(index: number): string;
    payloadChannelRequestHandlers(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    payloadChannelRequestHandlersLength(): number;
    payloadChannelNotificationHandlers(index: number): string;
    payloadChannelNotificationHandlers(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    payloadChannelNotificationHandlersLength(): number;
    static startChannelMessageHandlers(builder: flatbuffers.Builder): void;
    static addChannelRequestHandlers(builder: flatbuffers.Builder, channelRequestHandlersOffset: flatbuffers.Offset): void;
    static createChannelRequestHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startChannelRequestHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPayloadChannelRequestHandlers(builder: flatbuffers.Builder, payloadChannelRequestHandlersOffset: flatbuffers.Offset): void;
    static createPayloadChannelRequestHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startPayloadChannelRequestHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPayloadChannelNotificationHandlers(builder: flatbuffers.Builder, payloadChannelNotificationHandlersOffset: flatbuffers.Offset): void;
    static createPayloadChannelNotificationHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startPayloadChannelNotificationHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static endChannelMessageHandlers(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createChannelMessageHandlers(builder: flatbuffers.Builder, channelRequestHandlersOffset: flatbuffers.Offset, payloadChannelRequestHandlersOffset: flatbuffers.Offset, payloadChannelNotificationHandlersOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): ChannelMessageHandlersT;
    unpackTo(_o: ChannelMessageHandlersT): void;
}
export declare class ChannelMessageHandlersT {
    channelRequestHandlers: (string)[];
    payloadChannelRequestHandlers: (string)[];
    payloadChannelNotificationHandlers: (string)[];
    constructor(channelRequestHandlers?: (string)[], payloadChannelRequestHandlers?: (string)[], payloadChannelNotificationHandlers?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=channel-message-handlers.d.ts.map