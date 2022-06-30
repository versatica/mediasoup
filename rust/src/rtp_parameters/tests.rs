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
