use coolbox_rs::Client;

#[test]
fn metadata_client_surface() {
    let client = Client::create_default();

    assert_eq!(client.endpoint(), "local://coolbox");
    assert!(client.is_ready());
    assert_eq!(client.version(), "1.0.0");
    assert_eq!(client.capability_count(), 3);
    assert_eq!(client.capability_at(0), "metadata");
    assert_eq!(client.capability_at(2), "metadata_management");
    assert_eq!(client.capability_at(99), "");
    assert!(!client.describe().is_empty());
}