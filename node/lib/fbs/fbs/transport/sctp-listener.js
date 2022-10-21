"use strict";
// automatically generated by the FlatBuffers compiler, do not modify
Object.defineProperty(exports, "__esModule", { value: true });
exports.SctpListenerT = exports.SctpListener = void 0;
const flatbuffers = require("flatbuffers");
const uint16string_1 = require("../../fbs/common/uint16string");
class SctpListener {
    bb = null;
    bb_pos = 0;
    __init(i, bb) {
        this.bb_pos = i;
        this.bb = bb;
        return this;
    }
    static getRootAsSctpListener(bb, obj) {
        return (obj || new SctpListener()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    static getSizePrefixedRootAsSctpListener(bb, obj) {
        bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
        return (obj || new SctpListener()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    streamIdTable(index, obj) {
        const offset = this.bb.__offset(this.bb_pos, 4);
        return offset ? (obj || new uint16string_1.Uint16String()).__init(this.bb.__indirect(this.bb.__vector(this.bb_pos + offset) + index * 4), this.bb) : null;
    }
    streamIdTableLength() {
        const offset = this.bb.__offset(this.bb_pos, 4);
        return offset ? this.bb.__vector_len(this.bb_pos + offset) : 0;
    }
    static startSctpListener(builder) {
        builder.startObject(1);
    }
    static addStreamIdTable(builder, streamIdTableOffset) {
        builder.addFieldOffset(0, streamIdTableOffset, 0);
    }
    static createStreamIdTableVector(builder, data) {
        builder.startVector(4, data.length, 4);
        for (let i = data.length - 1; i >= 0; i--) {
            builder.addOffset(data[i]);
        }
        return builder.endVector();
    }
    static startStreamIdTableVector(builder, numElems) {
        builder.startVector(4, numElems, 4);
    }
    static endSctpListener(builder) {
        const offset = builder.endObject();
        builder.requiredField(offset, 4); // stream_id_table
        return offset;
    }
    static createSctpListener(builder, streamIdTableOffset) {
        SctpListener.startSctpListener(builder);
        SctpListener.addStreamIdTable(builder, streamIdTableOffset);
        return SctpListener.endSctpListener(builder);
    }
    unpack() {
        return new SctpListenerT(this.bb.createObjList(this.streamIdTable.bind(this), this.streamIdTableLength()));
    }
    unpackTo(_o) {
        _o.streamIdTable = this.bb.createObjList(this.streamIdTable.bind(this), this.streamIdTableLength());
    }
}
exports.SctpListener = SctpListener;
class SctpListenerT {
    streamIdTable;
    constructor(streamIdTable = []) {
        this.streamIdTable = streamIdTable;
    }
    pack(builder) {
        const streamIdTable = SctpListener.createStreamIdTableVector(builder, builder.createObjectOffsetList(this.streamIdTable));
        return SctpListener.createSctpListener(builder, streamIdTable);
    }
}
exports.SctpListenerT = SctpListenerT;
