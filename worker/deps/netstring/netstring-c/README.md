Netstring parsing
=================

A [netstring](http://en.wikipedia.org/wiki/Netstring) is a way of encoding a sequence of bytes for transmission over a network, or for serialization. They're very easy to work with. They encode the data's length, and can be concatenated trivially. The format was [defined by D. J. Bernstein](http://cr.yp.to/proto/netstrings.txt) and is used in various software. Examples of netstrings:

    "12:hello, world!,"
    "3:foo,"
    "0:,"

To specify a list of strings, just concatenate the netstrings. The list ["hey", "everyone"] can be encoded as

    "3:hey,8:everyone,"

This is some code in C for netstring serialization and deserialization. It checks netstrings for validity when parsing them, and guards against malicious input. It's also fast.

Basic API
---------

All the code is in `netstring.c` and `netstring.h`, and these have no external dependencies. To use them, just include them in your application. Include `netstring.h` and link with the C code.

**Parsing netstrings**

To parse a netstring, use `netstring_read()`.

    int netstring_read(char *buffer, size_t buffer_length,
                       char **netstring_start, size_t *netstring_length);

Reads a netstring from a `buffer` of length `buffer_length`. Writes to
`netstring_start` a pointer to the beginning of the string in the
buffer, and to `netstring_length` the length of the string. Does not
allocate any memory. If it reads successfully, then it returns 0. If
there is an error, then the return value will be negative. The error
values are:

    NETSTRING_ERROR_TOO_LONG      More than 999999999 bytes in a field
    NETSTRING_ERROR_NO_COLON      No colon was found after the number
    NETSTRING_ERROR_TOO_SHORT     Number of bytes greater than buffer length
    NETSTRING_ERROR_NO_COMMA      No comma was found at the end
    NETSTRING_ERROR_LEADING_ZERO  Leading zeros are not allowed
    NETSTRING_ERROR_NO_LENGTH     Length not given at start of netstring

If you're sending messages with more than 999999999 bytes -- about 2
GB -- then you probably should not be doing so in the form of a single
netstring. This restriction is in place partially to protect from
malicious or erroneous input, and partly to be compatible with
D. J. Bernstein's reference implementation.

**Creating netstrings**

To create a netstring, there are a few ways to do it. You could do something really simple like this example from [the spec](http://cr.yp.to/proto/netstrings.txt):

    if (printf("%lu:", len) < 0) barf();
    if (fwrite(buf, 1, len, stdout) < len) barf();
    if (putchar(',') < 0) barf();
    
This code provides a convenience function for creating a new netstring:

     size_t netstring_encode_new(char **netstring, char *data, size_t len);

This allocates and creates a netstring containing the first `len` bytes of `data`. This must be manually freed by the client. If `len` is 0 then no data will be read from `data`, and it may be null.

Contributing
------------

All this code is Public Domain. If you want to contribute, you can send bug reports, or fork the project on GitHub. Contributions are welcomed with open arms.