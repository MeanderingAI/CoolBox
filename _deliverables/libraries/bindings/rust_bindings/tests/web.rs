use coolbox_rs::{FormSchema, HttpRequest, HttpResponse};
use std::collections::BTreeMap;

#[test]
fn shared_form_normalizes_and_reports_field_errors() {
    let schema = FormSchema::parse(
        r#"{"fields":{"age":{"type":"integer","required":true,"minimum":18},
            "active":{"type":"boolean"}}}"#,
    )
    .expect("parse shared schema");
    let request = HttpRequest {
        method: "POST".into(),
        path: "/register".into(),
        body: "age=21&active=on".into(),
        headers: BTreeMap::from([(
            "content-type".into(),
            "application/x-www-form-urlencoded".into(),
        )]),
        path_params: BTreeMap::new(),
        query_params: BTreeMap::new(),
    };
    let result = schema.validate_request(&request).expect("validate request");
    assert!(result.valid);
    assert_eq!(result.values["age"], 21);
    assert_eq!(result.values["active"], true);
    let invalid = schema
        .validate(r#"{"age":17}"#, "application/json")
        .expect("field errors");
    assert!(!invalid.valid);
    assert_eq!(invalid.errors["age"], vec!["minimum"]);
    assert!(schema.validate("not JSON", "application/json").is_err());
}

#[test]
fn html_responses_use_the_shared_escaped_renderer() {
    let unique = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .expect("system time")
        .as_nanos();
    let root = std::env::temp_dir().join(format!("coolbox-rust-web-{unique}"));
    std::fs::create_dir(&root).expect("create template directory");
    let template = root.join("page.html");
    std::fs::write(&template, "<h1>{{name}}</h1>").expect("write template");
    let response = HttpResponse::render(
        root.to_str().expect("UTF-8 path"),
        "page.html",
        &BTreeMap::from([("name".into(), "<Ada>".into())]),
    )
    .expect("render HTML");
    assert_eq!(response.body, "<h1>&lt;Ada&gt;</h1>");
    assert_eq!(response.headers["Content-Type"], "text/html; charset=utf-8");
    assert!(HttpResponse::render(
        root.to_str().expect("UTF-8 path"),
        "page.html",
        &BTreeMap::new()
    )
    .is_err());
    std::fs::remove_file(template).expect("remove template");
    std::fs::remove_dir(root).expect("remove template directory");
}
