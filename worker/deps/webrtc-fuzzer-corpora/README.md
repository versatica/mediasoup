# webrtc-fuzzer-corpora

This repository contains [libFuzzer](http://libfuzzer.info) corpus files useful for WebRTC developers to test their implementations.

The `corpora` folders contains subfolders with corpus files.

Some corpus files are taken from the Chromium project (those into the [webrtc/test/fuzzer/](https://chromium.googlesource.com/external/webrtc/+/master/test/fuzzers/corpora/) folder), so credits to the Chromium guys.

The `reports` folder contains some relevant fuzzer generated crash and memory leak reports contributed by the community.


## SHA1 hash and file duplicates

It is desiderable to avoid duplicate files in this repository in order to not pollute the folders with samples that might lead to usless testing iterations (e.g. file with different names but same binary content).

To help managing this kind of issues, the script `add_sha1.sh` can be used to append the sha1 hashsum to the corpora and results files.

```
$ ./add_sha1.sh .
renamed './corpora/agc-corpus/agc-1' -> './corpora/agc-corpus/agc-1-b35a35f11d86e802f694f84c45e48da96158f73f'
renamed './corpora/agc-corpus/agc-2' -> './corpora/agc-corpus/agc-2-f3d59a997d30890da3239b377bba3bc502d18f1e'
...
renamed './corpora/rtcp-corpus/0.rtcp' -> './corpora/rtcp-corpus/0-01b13c2eb549daadeec1eb7eb0846e9a2f5729eb.rtcp'
```

Target folder can be specified as first input. File extensions will not be touched.

The script will detect an already hashed file by looking for the sha1sum in the filename. In this cases the output will be like:

```
> ./add_sha1.sh . | grep OK
./corpora/pseudotcp-corpus/785b96587d0eb44dd5d75b7a886f37e2ac504511 -> OK
./corpora/rtcp-corpus/0-01b13c2eb549daadeec1eb7eb0846e9a2f5729eb.rtcp -> OK
./corpora/rtp-corpus/rtp-0-951641f47532884149e6ebe9a87386226d8bf4fe -> OK
```

This script will alse reveal duplicate files in the same subfolder by matching the file sha1sum with other files names:

```
> ./add_sha1.sh . | grep DUPLICATE
./corpora/rtp-corpus/rtp-4 -> DUPLICATE ./corpora/rtp-corpus/rtp-1-5a709d82364ddf4f9350104c83994dded1c9f91c
./corpora/sdp-corpus/4.sdp -> DUPLICATE ./corpora/sdp-corpus/2-60495c33fc8758c5ce44f896c5f508bc026842c2.sdp
./corpora/sdp-corpus/8.sdp -> DUPLICATE ./corpora/sdp-corpus/2-60495c33fc8758c5ce44f896c5f508bc026842c2.sdp
```

Anyway `add_sha1` will only detect duplicates in the same subfolder.
Another way to quickly reveal file duplicates across all subfolders is the following command:

```
> find . \( ! -regex '.*/\..*' \) -type f -exec sha1sum {} + | sort | uniq -w40 -dD
131d620296fa9eb2c372c7fd71b8e58f8a10abd5  ./corpora/sdp-corpus/46-131d620296fa9eb2c372c7fd71b8e58f8a10abd5.sdp
131d620296fa9eb2c372c7fd71b8e58f8a10abd5  ./corpora/sdp-corpus/50.sdp
271138fdddb4f02313f25b47581f9f8f4a2eb201  ./corpora/sdp-corpus/12-271138fdddb4f02313f25b47581f9f8f4a2eb201.sdp
271138fdddb4f02313f25b47581f9f8f4a2eb201  ./corpora/sdp-corpus/15.sdp
271138fdddb4f02313f25b47581f9f8f4a2eb201  ./corpora/sdp-corpus/40.sdp
...
461a0e9201a7ea5ea6a43511571bdafce10b8185  ./corpora/rtcp-corpus/17-461a0e9201a7ea5ea6a43511571bdafce10b8185.rtcp
461a0e9201a7ea5ea6a43511571bdafce10b8185  ./reports/crashes/rtcp/crash-461a0e9201a7ea5ea6a43511571bdafce10b8185
```


## License

MIT or BSD or ISC or something like that (**let me focus on just fuzzing things!!**).
