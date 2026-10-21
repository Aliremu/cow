.PHONY: image build run test shell clean-cache async-example

IMAGE ?= dev
TTY   := $(shell [ -t 0 ] && echo -t)
CABIN_CACHE_DIR ?= $(PWD)/.cabin-cache
HOST_PORT ?= 8080
PACKAGE ?=
BIN ?=
RUN_TARGET = $(if $(PACKAGE),--package "$(PACKAGE)") $(if $(BIN),--bin "$(BIN)")
DOCKER_RUN = docker run --rm -i $(TTY) \
		--network host\
    -v "$(PWD)":"$(PWD)" -w "$(PWD)" \
    -e CCACHE_DIR=/ccache -e CCACHE_MAXSIZE=50G -v dev-ccache:/ccache \
    $(IMAGE)

image:
	docker build -t $(IMAGE) .

sysroot:
	$(DOCKER_RUN) bash -c 'rm -rf sysroot && mkdir -p sysroot/include \
	  && cp -r /opt/gcc-trunk/include/c++ sysroot/ \
	  && cp -rL /usr/include/. sysroot/include/'

build:
	$(DOCKER_RUN) cabin build --cache-dir "$(CABIN_CACHE_DIR)"

run:
	$(DOCKER_RUN) cabin run --cache-dir "$(CABIN_CACHE_DIR)" $(RUN_TARGET) -- $(ARGS)

fmt:
	$(DOCKER_RUN) cabin fmt --workspace -- $(ARGS)

test:
	$(DOCKER_RUN) cabin test --cache-dir "$(CABIN_CACHE_DIR)"

async-example:
	@test -n "$(EXAMPLE)" || (echo "Usage: make async-example EXAMPLE=<name>" >&2; exit 2)
	$(DOCKER_RUN) cabin run --cache-dir "$(CABIN_CACHE_DIR)" --package async --bin "$(EXAMPLE)"

shell:
	$(DOCKER_RUN) bash

clean-cache:
	docker volume rm dev-ccache
