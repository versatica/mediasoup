import * as flatbuffers from 'flatbuffers';
import { StringUint8, StringUint8T } from '../../fbs/common/string-uint8';
import { Uint32String, Uint32StringT } from '../../fbs/common/uint32string';
import { RtpListener, RtpListenerT } from '../../fbs/transport/rtp-listener';
import { SctpAssociation, SctpAssociationT } from '../../fbs/transport/sctp-association';
export declare class BaseTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): BaseTransportDump;
    static getRootAsBaseTransportDump(bb: flatbuffers.ByteBuffer, obj?: BaseTransportDump): BaseTransportDump;
    static getSizePrefixedRootAsBaseTransportDump(bb: flatbuffers.ByteBuffer, obj?: BaseTransportDump): BaseTransportDump;
    id(): string | null;
    id(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    direct(): boolean;
    producerIds(index: number): string;
    producerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    producerIdsLength(): number;
    consumerIds(index: number): string;
    consumerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    consumerIdsLength(): number;
    mapSsrcConsumerId(index: number, obj?: Uint32String): Uint32String | null;
    mapSsrcConsumerIdLength(): number;
    mapRtxSsrcConsumerId(index: number, obj?: Uint32String): Uint32String | null;
    mapRtxSsrcConsumerIdLength(): number;
    dataProducerIds(index: number): string;
    dataProducerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    dataProducerIdsLength(): number;
    dataConsumerIds(index: number): string;
    dataConsumerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    dataConsumerIdsLength(): number;
    recvRtpHeaderExtensions(index: number, obj?: StringUint8): StringUint8 | null;
    recvRtpHeaderExtensionsLength(): number;
    rtpListener(obj?: RtpListener): RtpListener | null;
    maxMessageSize(): number;
    sctpAssociation(obj?: SctpAssociation): SctpAssociation | null;
    traceEventTypes(index: number): string;
    traceEventTypes(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    traceEventTypesLength(): number;
    static startBaseTransportDump(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, idOffset: flatbuffers.Offset): void;
    static addDirect(builder: flatbuffers.Builder, direct: boolean): void;
    static addProducerIds(builder: flatbuffers.Builder, producerIdsOffset: flatbuffers.Offset): void;
    static createProducerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startProducerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addConsumerIds(builder: flatbuffers.Builder, consumerIdsOffset: flatbuffers.Offset): void;
    static createConsumerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startConsumerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addMapSsrcConsumerId(builder: flatbuffers.Builder, mapSsrcConsumerIdOffset: flatbuffers.Offset): void;
    static createMapSsrcConsumerIdVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startMapSsrcConsumerIdVector(builder: flatbuffers.Builder, numElems: number): void;
    static addMapRtxSsrcConsumerId(builder: flatbuffers.Builder, mapRtxSsrcConsumerIdOffset: flatbuffers.Offset): void;
    static createMapRtxSsrcConsumerIdVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startMapRtxSsrcConsumerIdVector(builder: flatbuffers.Builder, numElems: number): void;
    static addDataProducerIds(builder: flatbuffers.Builder, dataProducerIdsOffset: flatbuffers.Offset): void;
    static createDataProducerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startDataProducerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addDataConsumerIds(builder: flatbuffers.Builder, dataConsumerIdsOffset: flatbuffers.Offset): void;
    static createDataConsumerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startDataConsumerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRecvRtpHeaderExtensions(builder: flatbuffers.Builder, recvRtpHeaderExtensionsOffset: flatbuffers.Offset): void;
    static createRecvRtpHeaderExtensionsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRecvRtpHeaderExtensionsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRtpListener(builder: flatbuffers.Builder, rtpListenerOffset: flatbuffers.Offset): void;
    static addMaxMessageSize(builder: flatbuffers.Builder, maxMessageSize: number): void;
    static addSctpAssociation(builder: flatbuffers.Builder, sctpAssociationOffset: flatbuffers.Offset): void;
    static addTraceEventTypes(builder: flatbuffers.Builder, traceEventTypesOffset: flatbuffers.Offset): void;
    static createTraceEventTypesVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startTraceEventTypesVector(builder: flatbuffers.Builder, numElems: number): void;
    static endBaseTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): BaseTransportDumpT;
    unpackTo(_o: BaseTransportDumpT): void;
}
export declare class BaseTransportDumpT {
    id: string | Uint8Array | null;
    direct: boolean;
    producerIds: (string)[];
    consumerIds: (string)[];
    mapSsrcConsumerId: (Uint32StringT)[];
    mapRtxSsrcConsumerId: (Uint32StringT)[];
    dataProducerIds: (string)[];
    dataConsumerIds: (string)[];
    recvRtpHeaderExtensions: (StringUint8T)[];
    rtpListener: RtpListenerT | null;
    maxMessageSize: number;
    sctpAssociation: SctpAssociationT | null;
    traceEventTypes: (string)[];
    constructor(id?: string | Uint8Array | null, direct?: boolean, producerIds?: (string)[], consumerIds?: (string)[], mapSsrcConsumerId?: (Uint32StringT)[], mapRtxSsrcConsumerId?: (Uint32StringT)[], dataProducerIds?: (string)[], dataConsumerIds?: (string)[], recvRtpHeaderExtensions?: (StringUint8T)[], rtpListener?: RtpListenerT | null, maxMessageSize?: number, sctpAssociation?: SctpAssociationT | null, traceEventTypes?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=base-transport-dump.d.ts.map