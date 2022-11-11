
* Use enums instead of strig when possible.
* Consumer PreferredLayers: make temporalLayer optional
* Normalize consumer creation utility in Transport & PipeTransport.
  * Check why there is no Request.CreateConsumerRequest method in flatbuffers.
	* This request for pipe transport fails in ChannelRequest:
		// auto s = flatbuffers::FlatBufferToString(msg, FBS::Request::RequestTypeTable());
