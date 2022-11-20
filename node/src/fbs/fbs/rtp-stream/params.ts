// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';



export class Params {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):Params {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsParams(bb:flatbuffers.ByteBuffer, obj?:Params):Params {
  return (obj || new Params()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsParams(bb:flatbuffers.ByteBuffer, obj?:Params):Params {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new Params()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

encodingIdx():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
}

ssrc():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
}

payloadType():number {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

mimeType():string|null
mimeType(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
mimeType(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

clockRate():number {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
}

rid():string|null
rid(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
rid(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 14);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

cname():string|null
cname(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
cname(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 16);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

rtxSsrc():number|null {
  const offset = this.bb!.__offset(this.bb_pos, 18);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : null;
}

rtxPayloadType():number|null {
  const offset = this.bb!.__offset(this.bb_pos, 20);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : null;
}

useNack():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 22);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

usePli():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 24);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

useFir():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 26);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

useInBandFec():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 28);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

useDtx():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 30);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

spatialLayers():number {
  const offset = this.bb!.__offset(this.bb_pos, 32);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

temporalLayers():number {
  const offset = this.bb!.__offset(this.bb_pos, 34);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

static startParams(builder:flatbuffers.Builder) {
  builder.startObject(16);
}

static addEncodingIdx(builder:flatbuffers.Builder, encodingIdx:number) {
  builder.addFieldInt32(0, encodingIdx, 0);
}

static addSsrc(builder:flatbuffers.Builder, ssrc:number) {
  builder.addFieldInt32(1, ssrc, 0);
}

static addPayloadType(builder:flatbuffers.Builder, payloadType:number) {
  builder.addFieldInt8(2, payloadType, 0);
}

static addMimeType(builder:flatbuffers.Builder, mimeTypeOffset:flatbuffers.Offset) {
  builder.addFieldOffset(3, mimeTypeOffset, 0);
}

static addClockRate(builder:flatbuffers.Builder, clockRate:number) {
  builder.addFieldInt32(4, clockRate, 0);
}

static addRid(builder:flatbuffers.Builder, ridOffset:flatbuffers.Offset) {
  builder.addFieldOffset(5, ridOffset, 0);
}

static addCname(builder:flatbuffers.Builder, cnameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(6, cnameOffset, 0);
}

static addRtxSsrc(builder:flatbuffers.Builder, rtxSsrc:number) {
  builder.addFieldInt32(7, rtxSsrc, 0);
}

static addRtxPayloadType(builder:flatbuffers.Builder, rtxPayloadType:number) {
  builder.addFieldInt8(8, rtxPayloadType, 0);
}

static addUseNack(builder:flatbuffers.Builder, useNack:boolean) {
  builder.addFieldInt8(9, +useNack, +false);
}

static addUsePli(builder:flatbuffers.Builder, usePli:boolean) {
  builder.addFieldInt8(10, +usePli, +false);
}

static addUseFir(builder:flatbuffers.Builder, useFir:boolean) {
  builder.addFieldInt8(11, +useFir, +false);
}

static addUseInBandFec(builder:flatbuffers.Builder, useInBandFec:boolean) {
  builder.addFieldInt8(12, +useInBandFec, +false);
}

static addUseDtx(builder:flatbuffers.Builder, useDtx:boolean) {
  builder.addFieldInt8(13, +useDtx, +false);
}

static addSpatialLayers(builder:flatbuffers.Builder, spatialLayers:number) {
  builder.addFieldInt8(14, spatialLayers, 0);
}

static addTemporalLayers(builder:flatbuffers.Builder, temporalLayers:number) {
  builder.addFieldInt8(15, temporalLayers, 0);
}

static endParams(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  builder.requiredField(offset, 10) // mime_type
  builder.requiredField(offset, 16) // cname
  return offset;
}

static createParams(builder:flatbuffers.Builder, encodingIdx:number, ssrc:number, payloadType:number, mimeTypeOffset:flatbuffers.Offset, clockRate:number, ridOffset:flatbuffers.Offset, cnameOffset:flatbuffers.Offset, rtxSsrc:number|null, rtxPayloadType:number|null, useNack:boolean, usePli:boolean, useFir:boolean, useInBandFec:boolean, useDtx:boolean, spatialLayers:number, temporalLayers:number):flatbuffers.Offset {
  Params.startParams(builder);
  Params.addEncodingIdx(builder, encodingIdx);
  Params.addSsrc(builder, ssrc);
  Params.addPayloadType(builder, payloadType);
  Params.addMimeType(builder, mimeTypeOffset);
  Params.addClockRate(builder, clockRate);
  Params.addRid(builder, ridOffset);
  Params.addCname(builder, cnameOffset);
  if (rtxSsrc !== null)
    Params.addRtxSsrc(builder, rtxSsrc);
  if (rtxPayloadType !== null)
    Params.addRtxPayloadType(builder, rtxPayloadType);
  Params.addUseNack(builder, useNack);
  Params.addUsePli(builder, usePli);
  Params.addUseFir(builder, useFir);
  Params.addUseInBandFec(builder, useInBandFec);
  Params.addUseDtx(builder, useDtx);
  Params.addSpatialLayers(builder, spatialLayers);
  Params.addTemporalLayers(builder, temporalLayers);
  return Params.endParams(builder);
}

unpack(): ParamsT {
  return new ParamsT(
    this.encodingIdx(),
    this.ssrc(),
    this.payloadType(),
    this.mimeType(),
    this.clockRate(),
    this.rid(),
    this.cname(),
    this.rtxSsrc(),
    this.rtxPayloadType(),
    this.useNack(),
    this.usePli(),
    this.useFir(),
    this.useInBandFec(),
    this.useDtx(),
    this.spatialLayers(),
    this.temporalLayers()
  );
}


unpackTo(_o: ParamsT): void {
  _o.encodingIdx = this.encodingIdx();
  _o.ssrc = this.ssrc();
  _o.payloadType = this.payloadType();
  _o.mimeType = this.mimeType();
  _o.clockRate = this.clockRate();
  _o.rid = this.rid();
  _o.cname = this.cname();
  _o.rtxSsrc = this.rtxSsrc();
  _o.rtxPayloadType = this.rtxPayloadType();
  _o.useNack = this.useNack();
  _o.usePli = this.usePli();
  _o.useFir = this.useFir();
  _o.useInBandFec = this.useInBandFec();
  _o.useDtx = this.useDtx();
  _o.spatialLayers = this.spatialLayers();
  _o.temporalLayers = this.temporalLayers();
}
}

export class ParamsT {
constructor(
  public encodingIdx: number = 0,
  public ssrc: number = 0,
  public payloadType: number = 0,
  public mimeType: string|Uint8Array|null = null,
  public clockRate: number = 0,
  public rid: string|Uint8Array|null = null,
  public cname: string|Uint8Array|null = null,
  public rtxSsrc: number|null = null,
  public rtxPayloadType: number|null = null,
  public useNack: boolean = false,
  public usePli: boolean = false,
  public useFir: boolean = false,
  public useInBandFec: boolean = false,
  public useDtx: boolean = false,
  public spatialLayers: number = 0,
  public temporalLayers: number = 0
){}


pack(builder:flatbuffers.Builder): flatbuffers.Offset {
  const mimeType = (this.mimeType !== null ? builder.createString(this.mimeType!) : 0);
  const rid = (this.rid !== null ? builder.createString(this.rid!) : 0);
  const cname = (this.cname !== null ? builder.createString(this.cname!) : 0);

  return Params.createParams(builder,
    this.encodingIdx,
    this.ssrc,
    this.payloadType,
    mimeType,
    this.clockRate,
    rid,
    cname,
    this.rtxSsrc,
    this.rtxPayloadType,
    this.useNack,
    this.usePli,
    this.useFir,
    this.useInBandFec,
    this.useDtx,
    this.spatialLayers,
    this.temporalLayers
  );
}
}