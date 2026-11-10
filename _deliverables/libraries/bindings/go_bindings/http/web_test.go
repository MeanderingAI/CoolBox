package http

import (
	"encoding/json"
	"io"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

const registrationSchema = `{"fields":{
	"name":{"type":"string","required":true,"min_length":2,"max_length":8},
	"email":{"type":"email","required":true},
	"age":{"type":"integer","minimum":18,"maximum":120},
	"score":{"type":"number","minimum":0,"maximum":1},
	"consent":{"type":"boolean","enum":[true]},
	"role":{"type":"string","enum":["user","admin"]}
}}`

func TestFormValidation(t *testing.T) {
	form, err := ParseFormSchema(registrationSchema)
	if err != nil {
		t.Fatal(err)
	}
	tests := []struct {
		name, body, contentType string
		valid                   bool
		errors                  map[string]string
	}{
		{"json", `{"name":"Zo\u00eb","email":"zoe@example.com","age":"21","score":0.5,"consent":true,"role":"user"}`,
			"application/json; charset=utf-8", true, nil},
		{"urlencoded", "name=Ada&email=ada%40example.com&age=21&score=0.5&consent=on&role=user",
			"application/x-www-form-urlencoded", true, nil},
		{"required", `{"name":"  "}`, "application/json", false,
			map[string]string{"name": "required", "email": "required"}},
		{"constraints", `{"name":"A","email":"bad","age":17,"score":2,"role":"owner","consent":false,"extra":"x"}`,
			"application/json", false, map[string]string{"name": "min_length", "email": "email", "age": "minimum",
				"score": "maximum", "role": "enum", "consent": "enum", "extra": "unknown"}},
		{"types", `{"name":5,"email":true,"age":18.5,"score":[],"consent":"maybe"}`,
			"application/json", false, map[string]string{"name": "type", "email": "type", "age": "type",
				"score": "type", "consent": "type"}},
		{"maxima", `{"name":"very long name","email":"a@b.com","age":121}`,
			"application/json", false, map[string]string{"name": "max_length", "age": "maximum"}},
		{"safe-integer", `{"name":"Ada","email":"a@b.com","age":9007199254740992}`,
			"application/json", false, map[string]string{"age": "type"}},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			result, err := form.Validate(test.body, test.contentType)
			if err != nil {
				t.Fatal(err)
			}
			if result.Valid != test.valid {
				t.Fatalf("valid=%v errors=%v", result.Valid, result.Errors)
			}
			for name, code := range test.errors {
				if len(result.Errors[name]) == 0 || result.Errors[name][0] != code {
					t.Errorf("%s error: got %v, want %s", name, result.Errors[name], code)
				}
				if _, exists := result.Values[name]; exists {
					t.Errorf("invalid field %s included in normalized values", name)
				}
			}
			if test.valid && (result.Values["age"] != float64(21) || result.Values["consent"] != true) {
				t.Fatalf("values were not normalized: %#v", result.Values)
			}
		})
	}
}

func TestFormRejectsMalformedSchemasAndBodies(t *testing.T) {
	for _, schema := range []string{
		`{}`, `[]`, `{"fields":{"x":{"type":"date"}}}`,
		`{"fields":{"x":{"type":"string","required":"yes"}}}`,
		`{"fields":{"x":{"type":"string","min_length":-1}}}`,
		`{"fields":{"x":{"type":"integer","minimum":5,"maximum":2}}}`,
		`{"fields":{"x":{"type":"integer","enum":["one"]}}}`,
		`{"fields":{},"typo":true}`,
		`{"fields":{"x":{"type":"string","min":1}}}`,
		`{"fields":{}} trailing`,
	} {
		if _, err := ParseFormSchema(schema); err == nil {
			t.Errorf("expected schema error for %s", schema)
		}
	}
	form, err := ParseFormSchema(registrationSchema)
	if err != nil {
		t.Fatal(err)
	}
	for _, body := range []string{
		`[]`, `{"name":"a","name":"b"}`, `{"name":"unterminated}`,
		`{"name":"\q"}`, `{"age":01}`, `{"age":NaN}`, `{"age":1e999}`,
		`{"consent":truth}`, `{} trailing`, `{"name":"\ud800"}`,
		`{"age":` + strings.Repeat("1", 600) + `}`,
		strings.Repeat("[", 80) + strings.Repeat("]", 80),
	} {
		if _, err := form.Validate(body, "application/json"); err == nil {
			t.Errorf("expected submission error for %s", body)
		}
		escaped, err := ParseFormSchema(`{"fields":{"a\"b\n":{"type":"string"}}}`)
		if err != nil {
			t.Fatal(err)
		}
		result, err := escaped.Validate(`{"a\"b\n":"\ud83d\ude00\t\"\\\u0001"}`, "application/json")
		if err != nil || !result.Valid || result.Values["a\"b\n"] != "\U0001f600\t\"\\\x01" {
			t.Fatalf("JSON string round-trip: %#v, %v", result, err)
		}
	}
	for _, body := range []string{"name=%GG", "name=%", "name=a&name=b"} {
		if _, err := form.Validate(body, "application/x-www-form-urlencoded"); err == nil {
			t.Errorf("expected URL-encoding error for %q", body)
		}
	}
	if _, err := form.Validate("", "multipart/form-data"); err == nil {
		t.Fatal("expected unsupported content type error")
	}
}

func TestHTMLFilesAndTemplates(t *testing.T) {
	root := t.TempDir()
	outside := t.TempDir()
	if err := os.WriteFile(filepath.Join(outside, "outside.html"), []byte("outside"), 0600); err != nil {
		t.Fatal(err)
	}
	relativeOutside, err := filepath.Rel(root, filepath.Join(outside, "outside.html"))
	if err != nil {
		t.Fatal(err)
	}
	if _, err := HTMLFile(root, relativeOutside); err == nil {
		t.Fatal("expected traversal to existing outside file to fail")
	}
	if err := os.Symlink(filepath.Join(outside, "outside.html"), filepath.Join(root, "escape.html")); err == nil {
		if _, err := HTMLFile(root, "escape.html"); err == nil {
			t.Fatal("expected symlink escape to fail")
		}
	} else {
		t.Logf("symlink test unavailable: %v", err)
	}
	page := `<h1>{{ title }}</h1><p>{{body}}</p>`
	if err := os.WriteFile(filepath.Join(root, "page.html"), []byte(page), 0600); err != nil {
		t.Fatal(err)
	}
	file, err := HTMLFile(root, "page.html")
	if err != nil || file.Body != page || file.Headers["Content-Type"] != "text/html; charset=utf-8" {
		t.Fatalf("HTMLFile: %#v, %v", file, err)
	}
	rendered, err := Render(root, "page.html", map[string]string{
		"title": `<script>"&'</script>`, "body": "{{title}}",
	})
	if err != nil {
		t.Fatal(err)
	}
	if rendered.Body != `<h1>&lt;script&gt;&quot;&amp;&#39;&lt;/script&gt;</h1><p>{{title}}</p>` {
		t.Fatalf("unsafe or recursive template substitution: %s", rendered.Body)
	}
	for _, path := range []string{"missing.html", filepath.Join("..", filepath.Base(root), "..", "outside.html"),
		filepath.Join(root, "page.html"), ".", "page.html\x00suffix"} {
		if _, err := HTMLFile(root, path); err == nil {
			t.Errorf("expected restricted/missing path error: %s", path)
		}
	}
	if _, err := Render(root, "page.html", map[string]string{"title": "x"}); err == nil {
		t.Fatal("expected missing template value error")
	}
	if err := os.WriteFile(filepath.Join(root, "bad.html"), []byte("{{title"), 0600); err != nil {
		t.Fatal(err)
	}
	if _, err := Render(root, "bad.html", map[string]string{}); err == nil {
		t.Fatal("expected malformed template error")
	}
}

func TestFormsAndHTMLOverHTTP(t *testing.T) {
	schemaPath := filepath.Join(t.TempDir(), "registration.form.json")
	if err := os.WriteFile(schemaPath, []byte(registrationSchema), 0600); err != nil {
		t.Fatal(err)
	}
	form, err := LoadFormSchema(schemaPath)
	if err != nil {
		t.Fatal(err)
	}
	root := t.TempDir()
	if err := os.WriteFile(filepath.Join(root, "welcome.html"), []byte("<h1>Hello {{name}}</h1>"), 0600); err != nil {
		t.Fatal(err)
	}
	base := startTestServer(t, func(server *HttpServer) {
		if err := server.Handle("POST", "/register", func(request HTTPRequest) HTTPResponse {
			result, err := form.ValidateRequest(request)
			if err != nil {
				return HTTPResponse{StatusCode: 400, Body: err.Error()}
			}
			if !result.Valid {
				body, err := json.Marshal(result)
				if err != nil {
					panic(err)
				}
				return HTTPResponse{StatusCode: 422, Body: string(body)}
			}
			response, err := Render(root, "welcome.html", map[string]string{"name": result.Values["name"].(string)})
			if err != nil {
				panic(err)
			}
			return response
		}); err != nil {
			t.Fatal(err)
		}
	})
	client := &http.Client{Timeout: 5 * time.Second}
	response, err := client.PostForm(base+"/register", url.Values{"name": {"<Ada>"}, "email": {"ada@example.com"}})
	if err != nil {
		t.Fatal(err)
	}
	body, readErr := io.ReadAll(response.Body)
	response.Body.Close()
	if readErr != nil {
		t.Fatal(readErr)
	}
	if response.StatusCode != 200 || response.Header.Get("Content-Type") != "text/html; charset=utf-8" ||
		string(body) != "<h1>Hello &lt;Ada&gt;</h1>" {
		t.Fatalf("HTML response: %d %v %s", response.StatusCode, response.Header, body)
	}
	response, err = client.PostForm(base+"/register", url.Values{"email": {"bad"}})
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	var invalid ValidationResult
	if err := json.NewDecoder(response.Body).Decode(&invalid); err != nil {
		t.Fatal(err)
	}
	if response.StatusCode != 422 || invalid.Valid || invalid.Errors["name"][0] != "required" {
		t.Fatalf("invalid form response: %d %#v", response.StatusCode, invalid)
	}
}
