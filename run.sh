#!/bin/bash

docker run --rm -it \
	--cap-add=SYS_PTRACE \
	--platform linux/amd64 \
	-v "$PWD:/lab" \
	csapp-lab
