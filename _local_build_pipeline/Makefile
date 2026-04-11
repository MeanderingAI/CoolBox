SHELL := /usr/bin/env bash

.DEFAULT_GOAL := help

.PHONY: help build-image all-supported \
	build-libs build-tests build-products build-cpp-docs build-tutorials build-purchase-pipeline docs-publish-dry-run \
	generate-purchase-c generate-purchase-python generate-purchase-go generate-purchase-js \
	generate-purchase-rust generate-purchase-r generate-purchase-java

help:
	@echo "Local build pipeline targets"
	@echo "  make build-image"
	@echo "  make build-libs"
	@echo "  make build-tests"
	@echo "  make build-products"
	@echo "  make build-cpp-docs"
	@echo "  make build-tutorials"
	@echo "  make build-purchase-pipeline"
	@echo "  make docs-publish-dry-run"
	@echo "  make generate-purchase-c"
	@echo "  make generate-purchase-python"
	@echo "  make generate-purchase-go"
	@echo "  make generate-purchase-js"
	@echo "  make generate-purchase-rust"
	@echo "  make generate-purchase-r"
	@echo "  make generate-purchase-java"
	@echo "  make all-supported"

build-image:
	bash ./scripts/run_workflow.sh build-libs.yaml --rebuild-image

build-libs:
	bash ./scripts/run_workflow.sh build-libs.yaml

build-tests:
	bash ./scripts/run_workflow.sh build-tests

build-products:
	bash ./scripts/run_workflow.sh build-products.yaml

build-cpp-docs:
	bash ./scripts/run_workflow.sh build-cpp-docs.yaml

build-tutorials:
	bash ./scripts/run_workflow.sh build-tutorials.yaml

build-purchase-pipeline:
	bash ./scripts/run_workflow.sh build-purchase-pipeline.yaml

docs-publish-dry-run:
	bash ./scripts/run_workflow.sh docs-publish.yaml

generate-purchase-c:
	bash ./scripts/run_workflow.sh generate-purchase-c.yaml

generate-purchase-python:
	bash ./scripts/run_workflow.sh generate-purchase-python.yaml

generate-purchase-go:
	bash ./scripts/run_workflow.sh generate-purchase-go.yaml

generate-purchase-js:
	bash ./scripts/run_workflow.sh generate-purchase-js.yaml

generate-purchase-rust:
	bash ./scripts/run_workflow.sh generate-purchase-rust.yaml

generate-purchase-r:
	bash ./scripts/run_workflow.sh generate-purchase-r.yaml

generate-purchase-java:
	bash ./scripts/run_workflow.sh generate-purchase-java.yaml

all-supported:
	bash ./scripts/run_all_supported.sh
