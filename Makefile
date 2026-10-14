.PHONY: image build run test shell clean-cache async-example

IMAGE ?= chuds-dev
TTY   := $(shell [ -t 0 ] && echo -t)
CABIN_CACHE_DIR ?= $(PWD)/.cabin-cache
DOCKER_RUN = docker run --rm -i $(TTY) \
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
	$(DOCKER_RUN) cabin run --cache-dir "$(CABIN_CACHE_DIR)" -- $(ARGS)

test:
	$(DOCKER_RUN) cabin test --cache-dir "$(CABIN_CACHE_DIR)"

async-example:
	@test -n "$(EXAMPLE)" || (echo "Usage: make async-example EXAMPLE=<name>" >&2; exit 2)
	$(DOCKER_RUN) cabin run --cache-dir "$(CABIN_CACHE_DIR)" --package async --bin "$(EXAMPLE)"

shell:
	$(DOCKER_RUN) bash

clean-cache:
	docker volume rm dev-ccache
