// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { ConsumerLayers, ConsumerLayersT } from '../../fbs/consumer/consumer-layers';
import { ConsumerScore, ConsumerScoreT } from '../../fbs/consumer/consumer-score';


export class ConsumeResponse {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):ConsumeResponse {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsConsumeResponse(bb:flatbuffers.ByteBuffer, obj?:ConsumeResponse):ConsumeResponse {
  return (obj || new ConsumeResponse()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsConsumeResponse(bb:flatbuffers.ByteBuffer, obj?:ConsumeResponse):ConsumeResponse {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new ConsumeResponse()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

paused():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

producerPaused():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

score(obj?:ConsumerScore):ConsumerScore|null {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? (obj || new ConsumerScore()).__init(this.bb!.__indirect(this.bb_pos + offset), this.bb!) : null;
}

preferredLayers(obj?:ConsumerLayers):ConsumerLayers|null {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? (obj || new ConsumerLayers()).__init(this.bb!.__indirect(this.bb_pos + offset), this.bb!) : null;
}

static startConsumeResponse(builder:flatbuffers.Builder) {
  builder.startObject(4);
}

static addPaused(builder:flatbuffers.Builder, paused:boolean) {
  builder.addFieldInt8(0, +paused, +false);
}

static addProducerPaused(builder:flatbuffers.Builder, producerPaused:boolean) {
  builder.addFieldInt8(1, +producerPaused, +false);
}

static addScore(builder:flatbuffers.Builder, scoreOffset:flatbuffers.Offset) {
  builder.addFieldOffset(2, scoreOffset, 0);
}

static addPreferredLayers(builder:flatbuffers.Builder, preferredLayersOffset:flatbuffers.Offset) {
  builder.addFieldOffset(3, preferredLayersOffset, 0);
}

static endConsumeResponse(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}


unpack(): ConsumeResponseT {
  return new ConsumeResponseT(
    this.paused(),
    this.producerPaused(),
    (this.score() !== null ? this.score()!.unpack() : null),
    (this.preferredLayers() !== null ? this.preferredLayers()!.unpack() : null)
  );
}


unpackTo(_o: ConsumeResponseT): void {
  _o.paused = this.paused();
  _o.producerPaused = this.producerPaused();
  _o.score = (this.score() !== null ? this.score()!.unpack() : null);
  _o.preferredLayers = (this.preferredLayers() !== null ? this.preferredLayers()!.unpack() : null);
}
}

export class ConsumeResponseT {
constructor(
  public paused: boolean = false,
  public producerPaused: boolean = false,
  public score: ConsumerScoreT|null = null,
  public preferredLayers: ConsumerLayersT|null = null
){}


pack(builder:flatbuffers.Builder): flatbuffers.Offset {
  const score = (this.score !== null ? this.score!.pack(builder) : 0);
  const preferredLayers = (this.preferredLayers !== null ? this.preferredLayers!.pack(builder) : 0);

  ConsumeResponse.startConsumeResponse(builder);
  ConsumeResponse.addPaused(builder, this.paused);
  ConsumeResponse.addProducerPaused(builder, this.producerPaused);
  ConsumeResponse.addScore(builder, score);
  ConsumeResponse.addPreferredLayers(builder, preferredLayers);

  return ConsumeResponse.endConsumeResponse(builder);
}
}