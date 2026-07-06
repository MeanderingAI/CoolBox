FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*

COPY dist/plang_lsp /usr/local/bin/plang_lsp
RUN chmod +x /usr/local/bin/plang_lsp

ENTRYPOINT ["/usr/local/bin/plang_lsp"]
