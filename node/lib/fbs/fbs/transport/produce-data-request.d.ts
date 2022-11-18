import * as flatbuffers from 'flatbuffers';
import { SctpStreamParameters, SctpStreamParametersT } from '../../fbs/sctp-parameters/sctp-stream-parameters';
export declare class ProduceDataRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ProduceDataRequest;
    static getRootAsProduceDataRequest(bb: flatbuffers.ByteBuffer, obj?: ProduceDataRequest): ProduceDataRequest;
    static getSizePrefixedRootAsProduceDataRequest(bb: flatbuffers.ByteBuffer, obj?: ProduceDataRequest): ProduceDataRequest;
    dataProducerId(): string | null;
    dataProducerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    sctpStreamParameters(obj?: SctpStreamParameters): SctpStreamParameters | null;
    label(): string | null;
    label(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    protocol(): string | null;
    protocol(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startProduceDataRequest(builder: flatbuffers.Builder): void;
    static addDataProducerId(builder: flatbuffers.Builder, dataProducerIdOffset: flatbuffers.Offset): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addSctpStreamParameters(builder: flatbuffers.Builder, sctpStreamParametersOffset: flatbuffers.Offset): void;
    static addLabel(builder: flatbuffers.Builder, labelOffset: flatbuffers.Offset): void;
    static addProtocol(builder: flatbuffers.Builder, protocolOffset: flatbuffers.Offset): void;
    static endProduceDataRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ProduceDataRequestT;
    unpackTo(_o: ProduceDataRequestT): void;
}
export declare class ProduceDataRequestT {
    dataProducerId: string | Uint8Array | null;
    type: string | Uint8Array | null;
    sctpStreamParameters: SctpStreamParametersT | null;
    label: string | Uint8Array | null;
    protocol: string | Uint8Array | null;
    constructor(dataProducerId?: string | Uint8Array | null, type?: string | Uint8Array | null, sctpStreamParameters?: SctpStreamParametersT | null, label?: string | Uint8Array | null, protocol?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=produce-data-request.d.ts.map