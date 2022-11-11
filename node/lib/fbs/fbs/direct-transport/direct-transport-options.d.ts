import * as flatbuffers from 'flatbuffers';
import { BaseTransportOptions, BaseTransportOptionsT } from '../../fbs/transport/base-transport-options';
export declare class DirectTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DirectTransportOptions;
    static getRootAsDirectTransportOptions(bb: flatbuffers.ByteBuffer, obj?: DirectTransportOptions): DirectTransportOptions;
    static getSizePrefixedRootAsDirectTransportOptions(bb: flatbuffers.ByteBuffer, obj?: DirectTransportOptions): DirectTransportOptions;
    base(obj?: BaseTransportOptions): BaseTransportOptions | null;
    static startDirectTransportOptions(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static endDirectTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDirectTransportOptions(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DirectTransportOptionsT;
    unpackTo(_o: DirectTransportOptionsT): void;
}
export declare class DirectTransportOptionsT {
    base: BaseTransportOptionsT | null;
    constructor(base?: BaseTransportOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=direct-transport-options.d.ts.map