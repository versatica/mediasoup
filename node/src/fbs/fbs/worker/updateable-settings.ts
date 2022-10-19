// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';



export class UpdateableSettings {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):UpdateableSettings {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsUpdateableSettings(bb:flatbuffers.ByteBuffer, obj?:UpdateableSettings):UpdateableSettings {
  return (obj || new UpdateableSettings()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsUpdateableSettings(bb:flatbuffers.ByteBuffer, obj?:UpdateableSettings):UpdateableSettings {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new UpdateableSettings()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

logLevel():string|null
logLevel(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
logLevel(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

logTags(index: number):string
logTags(index: number,optionalEncoding:flatbuffers.Encoding):string|Uint8Array
logTags(index: number,optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__string(this.bb!.__vector(this.bb_pos + offset) + index * 4, optionalEncoding) : null;
}

logTagsLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

static startUpdateableSettings(builder:flatbuffers.Builder) {
  builder.startObject(2);
}

static addLogLevel(builder:flatbuffers.Builder, logLevelOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, logLevelOffset, 0);
}

static addLogTags(builder:flatbuffers.Builder, logTagsOffset:flatbuffers.Offset) {
  builder.addFieldOffset(1, logTagsOffset, 0);
}

static createLogTagsVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startLogTagsVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static endUpdateableSettings(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createUpdateableSettings(builder:flatbuffers.Builder, logLevelOffset:flatbuffers.Offset, logTagsOffset:flatbuffers.Offset):flatbuffers.Offset {
  UpdateableSettings.startUpdateableSettings(builder);
  UpdateableSettings.addLogLevel(builder, logLevelOffset);
  UpdateableSettings.addLogTags(builder, logTagsOffset);
  return UpdateableSettings.endUpdateableSettings(builder);
}

unpack(): UpdateableSettingsT {
  return new UpdateableSettingsT(
    this.logLevel(),
    this.bb!.createScalarList(this.logTags.bind(this), this.logTagsLength())
  );
}


unpackTo(_o: UpdateableSettingsT): void {
  _o.logLevel = this.logLevel();
  _o.logTags = this.bb!.createScalarList(this.logTags.bind(this), this.logTagsLength());
}
}

export class UpdateableSettingsT {
constructor(
  public logLevel: string|Uint8Array|null = null,
  public logTags: (string)[] = []
){}


pack(builder:flatbuffers.Builder): flatbuffers.Offset {
  const logLevel = (this.logLevel !== null ? builder.createString(this.logLevel!) : 0);
  const logTags = UpdateableSettings.createLogTagsVector(builder, builder.createObjectOffsetList(this.logTags));

  return UpdateableSettings.createUpdateableSettings(builder,
    logLevel,
    logTags
  );
}
}