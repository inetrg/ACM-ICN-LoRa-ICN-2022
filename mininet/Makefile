.PHONY: all mincom

export UID=$(shell id -u)
export GID=$(shell id -g)

SCRIPT=topology.py
DOCKER_CMD=docker run \
	--privileged\
	--network=host \
	--env GID \
	--env UID \
	-v /tmp/hosts:/tmp/hosts \
    -v $(shell pwd)/..:/root \

DOCKER_TEE=-it inetrg/mininet

all:
	@$(DOCKER_CMD) $(DOCKER_TEE) /root/mininet/$(SCRIPT)

mincom:
	@echo Running Mininet environment. Connect to /tmp/mininet pty to interact with it.
	@socat pty,rawer,link=/tmp/mininet EXEC:'make all',pty > /dev/null

shell:
	$(DOCKER_CMD) --entrypoint bash $(DOCKER_TEE)
