import * as flatbuffers from 'flatbuffers';
export declare class EnableTraceEventRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): EnableTraceEventRequest;
    static getRootAsEnableTraceEventRequest(bb: flatbuffers.ByteBuffer, obj?: EnableTraceEventRequest): EnableTraceEventRequest;
    static getSizePrefixedRootAsEnableTraceEventRequest(bb: flatbuffers.ByteBuffer, obj?: EnableTraceEventRequest): EnableTraceEventRequest;
    events(index: number): string;
    events(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    eventsLength(): number;
    static startEnableTraceEventRequest(builder: flatbuffers.Builder): void;
    static addEvents(builder: flatbuffers.Builder, eventsOffset: flatbuffers.Offset): void;
    static createEventsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startEventsVector(builder: flatbuffers.Builder, numElems: number): void;
    static endEnableTraceEventRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createEnableTraceEventRequest(builder: flatbuffers.Builder, eventsOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): EnableTraceEventRequestT;
    unpackTo(_o: EnableTraceEventRequestT): void;
}
export declare class EnableTraceEventRequestT {
    events: (string)[];
    constructor(events?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=enable-trace-event-request.d.ts.map