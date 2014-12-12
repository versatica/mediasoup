# ControlProtocol

The MediaSoup ControlProtocol is a bidirectional protocol to manage MediaSoup and multimedia sessions.


## Definitions


### ControlProtocol Request

A message which typically expects a Response.


### ControlProtocol Response

A message which is the response to a Request.


### ControlProtocol Notification

A message that does not expect a Response and its not a Response.


## Message Types


### Request

Every ControlProtocol Request conforms to the following syntax (note that CRLF means "\r\n" symbols):
```
Request type CRLF
uuid CRLF
*(key: value CRLF)
CRLF
```

where:

* *Request* is the "Request" literal string.
* *type* is a string indicating a valid ControlProtocol Request type.
* *uuid* is the integer identifier for this transaction (max length: 9 digits).
* *key* is a string indicating a parameter name.
* *value* is a string representing the value for a parameter.


### Response

Every ControlProtocol Response conforms to the following syntax (note that CRLF means "\r\n" symbols):
```
Response type CRLF
uuid status CRLF
[code: error_code CRLF]
[reason: error_reason CRLF]
*(key: value CRLF)
CRLF
```

where:

* *Response* is the "Response" literal string.
* *type* is a string indicating the type of the Request this Response is for.
* *uuid* matches the value in the Request.
* *status* is the "OK" or "ERROR" literal string.
* *code* is string indicating the exact error.
* *reason* is a descriptive string for the error.
* *key* is a string indicating a parameter name.
* *value* is a string representing the value for a parameter.


### Notification

Every ControlProtocol Notification conforms to the following syntax (note that CRLF means "\r\n" symbols):
```
Notification type CRLF
*(key: value CRLF)
CRLF
```

where:

* *Notification* is the "Notification" literal string.
* *type* is a string indicating a valid ControlProtocol Notification type.
* *key* is a string indicating a parameter name.
* *value* is a string representing the value for a parameter.
