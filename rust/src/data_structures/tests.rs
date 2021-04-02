use super::*;

#[test]
fn dtls_fingerprint() {
    {
        let dtls_fingerprints = &[
            r#"{"algorithm":"sha-1","value":"0D:88:5B:EF:B9:86:F9:66:67:75:7A:C1:7A:78:34:E4:88:DC:44:67"}"#,
            r#"{"algorithm":"sha-224","value":"6E:0C:C7:23:DF:36:E1:C7:46:AB:D7:B1:CE:DD:97:C3:C1:17:25:D6:26:0A:8A:B4:50:F1:3E:BC"}"#,
            r#"{"algorithm":"sha-256","value":"7A:27:46:F0:7B:09:28:F0:10:E2:EC:84:60:B5:87:9A:D9:C8:8B:F3:6C:C5:5D:C3:F3:BA:2C:5B:4F:8A:3A:E3"}"#,
            r#"{"algorithm":"sha-384","value":"D0:B7:F7:3C:71:9F:F4:A1:48:E1:9B:13:25:59:A4:7D:06:BF:E1:1B:DC:0B:8A:8E:45:09:01:22:7E:81:68:EC:DD:B8:DD:CA:1F:F3:F2:E8:15:A5:3C:23:CF:F7:B6:38"}"#,
            r#"{"algorithm":"sha-512","value":"36:8B:9B:CA:2B:01:2B:33:FD:06:95:F2:CC:28:56:69:5B:DD:38:5E:88:32:9A:72:F7:B1:5D:87:9E:64:97:0B:66:A1:C7:6C:BE:4D:CD:83:90:04:AE:20:6C:6D:5F:F0:BD:4C:D9:DD:6E:8A:19:C1:C9:F6:C2:46:C8:08:94:39"}"#,
        ];

        for dtls_fingerprint_str in dtls_fingerprints {
            let dtls_fingerprint =
                serde_json::from_str::<DtlsFingerprint>(dtls_fingerprint_str).unwrap();
            assert_eq!(
                dtls_fingerprint_str,
                &serde_json::to_string(&dtls_fingerprint).unwrap()
            );
        }
    }

    {
        let bad_dtls_fingerprints = &[
            r#"{"algorithm":"sha-1","value":"0D:88:5B:EF:B9:86:F9:66:67::44:67"}"#,
            r#"{"algorithm":"sha-200","value":"6E:0C:C7:23:DF:36:E1:C7:46:AB:D7:B1:CE:DD:97:C3:C1:17:25:D6:26:0A:8A:B4:50:F1:3E:BC"}"#,
        ];

        for dtls_fingerprint_str in bad_dtls_fingerprints {
            assert!(serde_json::from_str::<DtlsFingerprint>(dtls_fingerprint_str).is_err());
        }
    }
}
