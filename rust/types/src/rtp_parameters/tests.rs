use super::*;

#[test]
fn rtcp_feedback_serde() {
    {
        let nack_pli_str = r#"{"type":"nack","parameter":"pli"}"#;

        assert_eq!(
            serde_json::from_str::<RtcpFeedback>(nack_pli_str).unwrap(),
            RtcpFeedback::NackPli
        );

        let result = serde_json::to_string(&RtcpFeedback::NackPli).unwrap();
        assert_eq!(result.as_str(), nack_pli_str);
    }
    {
        let transport_cc_str = r#"{"type":"transport-cc","parameter":""}"#;

        assert_eq!(
            serde_json::from_str::<RtcpFeedback>(transport_cc_str).unwrap(),
            RtcpFeedback::TransportCc
        );

        let result = serde_json::to_string(&RtcpFeedback::TransportCc).unwrap();
        assert_eq!(result.as_str(), transport_cc_str);
    }
    {
        let nack_bar_str = r#"{"type":"nack","parameter":"bar"}"#;

        assert_eq!(
            serde_json::from_str::<RtcpFeedback>(nack_bar_str).unwrap(),
            RtcpFeedback::Unsupported
        );
    }
}

#[test]
fn rtp_header_extension_uri_serde() {
    {
        let mid_str = r#""urn:ietf:params:rtp-hdrext:sdes:mid""#;

        assert_eq!(
            serde_json::from_str::<RtpHeaderExtensionUri>(mid_str).unwrap(),
            RtpHeaderExtensionUri::Mid
        );

        let result = serde_json::to_string(&RtpHeaderExtensionUri::Mid).unwrap();
        assert_eq!(result.as_str(), mid_str);
    }
    {
        // Unknown URIs are not tolerated, they must fail to deserialize rather than end up as a
        // value that cannot be given to the worker.
        let chicken_str = r#""urn:ietf:params:rtp-hdrext:chicken""#;

        assert!(serde_json::from_str::<RtpHeaderExtensionUri>(chicken_str).is_err());

        assert_eq!(
            RtpHeaderExtensionUri::from_str("urn:ietf:params:rtp-hdrext:chicken"),
            Err(RtpHeaderExtensionUriParseError::Unsupported(
                "urn:ietf:params:rtp-hdrext:chicken".to_string()
            ))
        );
    }
}
