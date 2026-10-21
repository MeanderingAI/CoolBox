package http

import (
	"fmt"
	"io"
	"net"
	nethttp "net/http"
	"os"
	"path/filepath"
	"testing"
	"time"
)

func startTestServer(t *testing.T, configure func(*HttpServer)) string {
	t.Helper()
	listener, err := net.Listen("tcp4", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	port := listener.Addr().(*net.TCPAddr).Port
	if err := listener.Close(); err != nil {
		t.Fatal(err)
	}
	server, err := NewHttpServer(port)
	if err != nil {
		t.Fatal(err)
	}
	configure(server)
	done := make(chan error, 1)
	go func() { done <- server.Start() }()
	deadline := time.Now().Add(5 * time.Second)
	for !server.IsRunning() {
		select {
		case err := <-done:
			server.Close()
			t.Fatalf("server exited before readiness: %v", err)
		default:
		}
		if time.Now().After(deadline) {
			// Avoid waiting indefinitely in Close if startup itself is stuck.
			server.Stop()
			t.Fatal("HTTP server did not become ready")
		}
		time.Sleep(time.Millisecond)
	}
	t.Cleanup(func() {
		server.Stop()
		select {
		case err := <-done:
			if err != nil {
				t.Errorf("server failed: %v", err)
			}
			server.Close()
		case <-time.After(5 * time.Second):
			t.Error("HTTP server did not stop")
		}
	})
	return fmt.Sprintf("http://127.0.0.1:%d", port)
}

func TestHTTPCallbackParameters(t *testing.T) {
	base := startTestServer(t, func(server *HttpServer) {
		err := server.Handle("GET", "/users/:id", func(request HTTPRequest) HTTPResponse {
			return HTTPResponse{
				StatusCode: 201,
				Body: request.Method + ":" + request.PathParams["id"] +
					":" + request.QueryParams["active"],
			}
		})
		if err != nil {
			t.Fatal(err)
		}
	})
	client := &nethttp.Client{Timeout: 5 * time.Second}
	response, err := client.Get(base + "/users/42?active=yes")
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	body, err := io.ReadAll(response.Body)
	if err != nil {
		t.Fatal(err)
	}
	if response.StatusCode != 201 || string(body) != "GET:42:yes" {
		t.Fatalf("unexpected callback result: status=%d body=%q", response.StatusCode, body)
	}
}

func TestRoutesManifestValidationAndServing(t *testing.T) {
	base := startTestServer(t, func(server *HttpServer) {
		err := server.RegisterHandler("health", func(HTTPRequest) HTTPResponse {
			return HTTPResponse{StatusCode: 200, Body: `{"status":"ok"}`}
		})
		if err != nil {
			t.Fatal(err)
		}
		path := filepath.Join(t.TempDir(), "app.routes")
		write := func(content string) {
			t.Helper()
			if err := os.WriteFile(path, []byte(content), 0600); err != nil {
				t.Fatal(err)
			}
		}
		write("GET / health\nGET /unknown missing\n")
		if err := server.LoadRoutes(path); err == nil {
			t.Fatal("expected an unregistered handler error")
		}
		write("GET / health\nGET / health\n")
		if err := server.LoadRoutes(path); err == nil {
			t.Fatal("expected a duplicate route error")
		}
		write("# Shared manifest\nGET / health\n")
		if err := server.LoadRoutes(path); err != nil {
			t.Fatal(err)
		}
		if err := server.LoadRoutes(path); err == nil {
			t.Fatal("expected an already registered route error")
		}
	})
	client := &nethttp.Client{Timeout: 5 * time.Second}
	response, err := client.Get(base + "/")
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	body, err := io.ReadAll(response.Body)
	if err != nil {
		t.Fatal(err)
	}
	if response.StatusCode != 200 || string(body) != `{"status":"ok"}` {
		t.Fatalf("unexpected manifest route result: status=%d body=%q", response.StatusCode, body)
	}
}
