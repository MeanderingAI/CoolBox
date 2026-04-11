FROM ubuntu:22.04
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /opt/plc3_lsp
COPY dist/plc3_lsp ./plc3_lsp
EXPOSE 0
ENTRYPOINT ["/opt/plc3_lsp/plc3_lsp"]