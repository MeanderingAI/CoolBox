FROM ubuntu:22.04
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /opt/pljava_lsp
COPY dist/pljava_lsp ./pljava_lsp
EXPOSE 0
ENTRYPOINT ["/opt/pljava_lsp/pljava_lsp"]
