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
    payloadchannelRequestHandlers(index: number): string;
    payloadchannelRequestHandlers(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    payloadchannelRequestHandlersLength(): number;
    payloadchannelNotificationHandlers(index: number): string;
    payloadchannelNotificationHandlers(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    payloadchannelNotificationHandlersLength(): number;
    static startChannelMessageHandlers(builder: flatbuffers.Builder): void;
    static addChannelRequestHandlers(builder: flatbuffers.Builder, channelRequestHandlersOffset: flatbuffers.Offset): void;
    static createChannelRequestHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startChannelRequestHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPayloadchannelRequestHandlers(builder: flatbuffers.Builder, payloadchannelRequestHandlersOffset: flatbuffers.Offset): void;
    static createPayloadchannelRequestHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startPayloadchannelRequestHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPayloadchannelNotificationHandlers(builder: flatbuffers.Builder, payloadchannelNotificationHandlersOffset: flatbuffers.Offset): void;
    static createPayloadchannelNotificationHandlersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startPayloadchannelNotificationHandlersVector(builder: flatbuffers.Builder, numElems: number): void;
    static endChannelMessageHandlers(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createChannelMessageHandlers(builder: flatbuffers.Builder, channelRequestHandlersOffset: flatbuffers.Offset, payloadchannelRequestHandlersOffset: flatbuffers.Offset, payloadchannelNotificationHandlersOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=channel-message-handlers.d.ts.map