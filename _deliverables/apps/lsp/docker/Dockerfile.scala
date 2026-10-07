FROM ubuntu:22.04
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /opt/plscala_lsp
COPY dist/plscala_lsp ./plscala_lsp
EXPOSE 0
ENTRYPOINT ["/opt/plscala_lsp/plscala_lsp"]
