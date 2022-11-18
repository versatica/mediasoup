import * as flatbuffers from 'flatbuffers';
import { SctpStreamParameters, SctpStreamParametersT } from '../../fbs/sctp-parameters/sctp-stream-parameters';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    id(): string | null;
    id(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    dataProducerId(): string | null;
    dataProducerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    sctpStreamParameters(obj?: SctpStreamParameters): SctpStreamParameters | null;
    label(): string | null;
    label(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    protocol(): string | null;
    protocol(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, idOffset: flatbuffers.Offset): void;
    static addDataProducerId(builder: flatbuffers.Builder, dataProducerIdOffset: flatbuffers.Offset): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addSctpStreamParameters(builder: flatbuffers.Builder, sctpStreamParametersOffset: flatbuffers.Offset): void;
    static addLabel(builder: flatbuffers.Builder, labelOffset: flatbuffers.Offset): void;
    static addProtocol(builder: flatbuffers.Builder, protocolOffset: flatbuffers.Offset): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    id: string | Uint8Array | null;
    dataProducerId: string | Uint8Array | null;
    type: string | Uint8Array | null;
    sctpStreamParameters: SctpStreamParametersT | null;
    label: string | Uint8Array | null;
    protocol: string | Uint8Array | null;
    constructor(id?: string | Uint8Array | null, dataProducerId?: string | Uint8Array | null, type?: string | Uint8Array | null, sctpStreamParameters?: SctpStreamParametersT | null, label?: string | Uint8Array | null, protocol?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map