import * as flatbuffers from 'flatbuffers';
import { SctpStreamParameters, SctpStreamParametersT } from '../../fbs/sctp-parameters/sctp-stream-parameters';
export declare class ConsumeDataRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumeDataRequest;
    static getRootAsConsumeDataRequest(bb: flatbuffers.ByteBuffer, obj?: ConsumeDataRequest): ConsumeDataRequest;
    static getSizePrefixedRootAsConsumeDataRequest(bb: flatbuffers.ByteBuffer, obj?: ConsumeDataRequest): ConsumeDataRequest;
    dataConsumerId(): string | null;
    dataConsumerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    dataProducerId(): string | null;
    dataProducerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    sctpStreamParameters(obj?: SctpStreamParameters): SctpStreamParameters | null;
    label(): string | null;
    label(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    protocol(): string | null;
    protocol(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startConsumeDataRequest(builder: flatbuffers.Builder): void;
    static addDataConsumerId(builder: flatbuffers.Builder, dataConsumerIdOffset: flatbuffers.Offset): void;
    static addDataProducerId(builder: flatbuffers.Builder, dataProducerIdOffset: flatbuffers.Offset): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addSctpStreamParameters(builder: flatbuffers.Builder, sctpStreamParametersOffset: flatbuffers.Offset): void;
    static addLabel(builder: flatbuffers.Builder, labelOffset: flatbuffers.Offset): void;
    static addProtocol(builder: flatbuffers.Builder, protocolOffset: flatbuffers.Offset): void;
    static endConsumeDataRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ConsumeDataRequestT;
    unpackTo(_o: ConsumeDataRequestT): void;
}
export declare class ConsumeDataRequestT {
    dataConsumerId: string | Uint8Array | null;
    dataProducerId: string | Uint8Array | null;
    type: string | Uint8Array | null;
    sctpStreamParameters: SctpStreamParametersT | null;
    label: string | Uint8Array | null;
    protocol: string | Uint8Array | null;
    constructor(dataConsumerId?: string | Uint8Array | null, dataProducerId?: string | Uint8Array | null, type?: string | Uint8Array | null, sctpStreamParameters?: SctpStreamParametersT | null, label?: string | Uint8Array | null, protocol?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consume-data-request.d.ts.map