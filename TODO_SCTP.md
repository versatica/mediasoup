# STCP

Here some notes about our future SCTP implementation.

## Flow

1. A DTLS packet arrives to `WebRtcTransport` where we check `DtlsTransport::IsDtls()`).
2. It calls `OnDtlsDataReceived()` that calls `this->dtlsTransport->ProcessDtlsData()`.
3. It calls `this->listener->OnDtlsTransportApplicationDataReceived()` in `WebRtcTransport`.
4. It calls `Transport::ReceiveSctpData()` that calls `this->sctpAssociation->ProcessSctpData()`.

However, in step 4 `WebRtcTransport::OnDtlsTransportApplicationDataReceived()` should instead check `RTC::SCTP::Packet.isSctp()` and then `RTC::SCTP::Packet::parse()` and call `Transport::ReceiveSctpData()` with a `SCTP::Packet` instance instead than `data` and `len`. In fact it should be named `Transport::ReceiveSctpPacket()` instead.

Same in `PipeTransport` and `PlainTransport`.

## `std::variant`

In `SCTP::Packet::GetItem(size_t idx)`:

```c++
#include <variant>
#include <memory>

class FooItem;
class BarItem;

class Lalala {
public:
  std::variant<std::unique_ptr<FooItem>, std::unique_ptr<BarItem>> GetItem(size_t idx)
  {
    // Here return unique_ptr of FooItem or BarItem.
  }
};

// In caller:

auto item = lalala->GetItem(0);

if (auto* fooItem = std::get_if<std::unique_ptr<FooItem>>(&item))
{
  std::unique_ptr<FooItem> foo = std::move(*fooItem);
}
else if (auto* barItem = std::get_if<std::unique_ptr<BarItem>>(&item))
{
  std::unique_ptr<BarItem> bar = std::move(*barItem);
}

// However we can check item->GetType() instead and do a switch().
```

## Parsing of subclasses FooItem

- Must initialize Value Length field in the constructor.
- Must provide with a `virtual bool FooItem::Verify() const = 0` method in which subclasses verify if header, flags, value length and value is ok.
- Must probably remove `GetValue()` and `SetValue()` in parent `FooItem` class or make then protected.

NO! Instead:

- Must remove static `Parse()` and `Factory()` in FooItem and implement
- them into each subclass.

## Optimize creation of packets with items

- Add some `GetNextItemPtr(size_t& remainingBufferLength)` in a packert class that returns a pointer to the position in which a new item would take place and modifies the given `remainingBufferLength` with the remaining length of the buffer after that position.
- Then create a `Item` via `Item::Factory()` by passing `ptr` as `buffer` and `remainingBufferLength` as `bufferLength`.
- Then also add `packet->AddInPlaceItem(item, ptr)` (or whatever) so the packet doesn't serialize (doesn't memcpy) the bytes of the `Item` into its buffer.
- Another related option is to add specialized `AddXxxxItem()` methods in the packet class.

## FooItem subclasses

- If we have a FooDataItem, the app should not be able to change its id via `SetId()`, AKA make `SetId()` protected since app should use `FooXxxxItem::Factory()` static methods.
- `FooItem` constructor: It doesn't make sense that it fills things since we want to rely on subclasses' `Parse()`, `Factory()` and constructors (that will override these calls anyway).
- If `FooItem` has valid but unknown id, that's correct, but let's remove the `SetValue()` method so those unknown items can only be parsed but not created.
