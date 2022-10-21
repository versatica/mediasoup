// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { Body, unionToBody, unionListToBody } from '../../fbs/response/body';
import { RouterDump, RouterDumpT } from '../../fbs/router/router-dump';
import { ConsumeResponse, ConsumeResponseT } from '../../fbs/transport/consume-response';
import { TransportDump, TransportDumpT } from '../../fbs/transport/transport-dump';
import { WebRtcServerDump, WebRtcServerDumpT } from '../../fbs/web-rtc-server/web-rtc-server-dump';
import { ResourceUsage, ResourceUsageT } from '../../fbs/worker/resource-usage';
import { WorkerDump, WorkerDumpT } from '../../fbs/worker/worker-dump';


export class Response {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):Response {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsResponse(bb:flatbuffers.ByteBuffer, obj?:Response):Response {
  return (obj || new Response()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsResponse(bb:flatbuffers.ByteBuffer, obj?:Response):Response {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new Response()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

id():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
}

accepted():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

bodyType():Body {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : Body.NONE;
}

body<T extends flatbuffers.Table>(obj:any):any|null {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.__union(obj, this.bb_pos + offset) : null;
}

static startResponse(builder:flatbuffers.Builder) {
  builder.startObject(4);
}

static addId(builder:flatbuffers.Builder, id:number) {
  builder.addFieldInt32(0, id, 0);
}

static addAccepted(builder:flatbuffers.Builder, accepted:boolean) {
  builder.addFieldInt8(1, +accepted, +false);
}

static addBodyType(builder:flatbuffers.Builder, bodyType:Body) {
  builder.addFieldInt8(2, bodyType, Body.NONE);
}

static addBody(builder:flatbuffers.Builder, bodyOffset:flatbuffers.Offset) {
  builder.addFieldOffset(3, bodyOffset, 0);
}

static endResponse(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createResponse(builder:flatbuffers.Builder, id:number, accepted:boolean, bodyType:Body, bodyOffset:flatbuffers.Offset):flatbuffers.Offset {
  Response.startResponse(builder);
  Response.addId(builder, id);
  Response.addAccepted(builder, accepted);
  Response.addBodyType(builder, bodyType);
  Response.addBody(builder, bodyOffset);
  return Response.endResponse(builder);
}

unpack(): ResponseT {
  return new ResponseT(
    this.id(),
    this.accepted(),
    this.bodyType(),
    (() => {
      let temp = unionToBody(this.bodyType(), this.body.bind(this));
      if(temp === null) { return null; }
      return temp.unpack()
  })()
  );
}


unpackTo(_o: ResponseT): void {
  _o.id = this.id();
  _o.accepted = this.accepted();
  _o.bodyType = this.bodyType();
  _o.body = (() => {
      let temp = unionToBody(this.bodyType(), this.body.bind(this));
      if(temp === null) { return null; }
      return temp.unpack()
  })();
}
}

export class ResponseT {
constructor(
  public id: number = 0,
  public accepted: boolean = false,
  public bodyType: Body = Body.NONE,
  public body: ConsumeResponseT|ResourceUsageT|RouterDumpT|TransportDumpT|WebRtcServerDumpT|WorkerDumpT|null = null
){}


pack(builder:flatbuffers.Builder): flatbuffers.Offset {
  const body = builder.createObjectOffset(this.body);

  return Response.createResponse(builder,
    this.id,
    this.accepted,
    this.bodyType,
    body
  );
}
}
