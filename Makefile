.DEFAULT_GOAL := help
SHELL := /bin/bash

MAIN_BRANCH := main
BUILD_DIR   := build/host
LLVM_BIN    ?= $(shell [ -d /opt/homebrew/opt/llvm/bin ] && echo /opt/homebrew/opt/llvm/bin/ || true)
CLANG_FORMAT := $(LLVM_BIN)clang-format
CLANG_TIDY   := $(LLVM_BIN)clang-tidy
PYTHON      ?= $(shell command -v python3 || command -v python)
CXX_FILES    = $(shell find src tests wasm/src -name '*.cpp' -o -name '*.hpp' 2>/dev/null)
TIDY_FILES   = $(shell find src/domain src/application src/adapters/profiles -name '*.cpp' 2>/dev/null; find src/adapters -name '*_mapping.cpp' 2>/dev/null)

.PHONY: help sdk host test format format-check tidy check wasm package clean gh-build cleanup_builds cleanup_branches

help: ## Show available targets
	@awk 'BEGIN {FS = ":.*## "; printf "Available targets:\n"} \
		/^[a-zA-Z_-]+:.*## / {printf "  \033[36m%-18s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

sdk: ## Download the MSFS 2024 SDK subset for the WASM build into sdk/ (needs msitools)
	scripts/fetch-sdk.sh

host: ## Configure + build domain/application and the unit tests for the host (CMake)
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_ARGS)
	cmake --build $(BUILD_DIR) -j

test: host ## Run the unit tests (Catch2, host build, no SDK)
	$(BUILD_DIR)/fs24_tests

format: ## Apply clang-format
	$(CLANG_FORMAT) -i $(CXX_FILES)

format-check: ## Verify clang-format
	$(CLANG_FORMAT) --dry-run --Werror $(CXX_FILES)

tidy: host ## clang-tidy over domain/application and the SDK-free adapter parts
	@if [ -n "$(TIDY_FILES)" ]; then $(CLANG_TIDY) -p $(BUILD_DIR) --quiet $(TIDY_FILES); else echo "tidy: no .cpp in src/domain or src/application yet"; fi

check: format-check tidy test ## Pre-commit gate: format, tidy, tests

wasm: ## Compile the WASM module (stock LLVM on the Mac = compile check only; Asobo's LLVM in CI)
	wasm/build.sh

package: wasm ## Assemble the Community package + zip in out/
	$(PYTHON) scripts/build_package.py

clean: ## Remove build output
	rm -rf build out wasm/build

gh-build: ## Run the GitHub build (manual workflow) on origin/main, wait, print the artifact link
	@test -z "$$(git status --porcelain)" || { echo "Uncommitted changes — commit first."; exit 1; }
	@git fetch -q origin $(MAIN_BRANCH)
	@test "$$(git rev-parse HEAD)" = "$$(git rev-parse origin/$(MAIN_BRANCH))" || { echo "Local HEAD differs from origin/$(MAIN_BRANCH) — push first."; exit 1; }
	gh workflow run build.yml --ref $(MAIN_BRANCH)
	@sleep 5
	@id=$$(gh run list --workflow build.yml --limit 1 --json databaseId -q '.[0].databaseId'); \
		gh run watch $$id --exit-status && \
		echo "Artifact: $$(gh run view $$id --json url -q .url) → Artifacts"

cleanup_builds: ## Delete all GitHub Actions runs except the latest one per workflow
	@ids=$$(gh run list --limit 1000 --json databaseId,workflowName,createdAt \
		-q 'group_by(.workflowName) | map(sort_by(.createdAt) | reverse | .[1:]) | flatten | .[].databaseId'); \
	if [ -z "$$ids" ]; then echo "Nothing to delete."; exit 0; fi; \
	echo "Deleting $$(echo "$$ids" | wc -l | tr -d ' ') run(s)…"; \
	for id in $$ids; do gh run delete $$id >/dev/null && echo "  deleted $$id"; done

cleanup_branches: ## Delete local branches already merged into main
	git fetch --prune origin
	@git branch --merged $(MAIN_BRANCH) --format='%(refname:short)' \
		| grep -vE '^($(MAIN_BRANCH))$$' \
		| xargs -r -n1 git branch -d
