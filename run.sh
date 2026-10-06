#!/bin/bash

docker run --rm -it \
	-p 8080:8080 \
	--dns 8.8.8.8 \
	--cap-add=SYS_PTRACE \
	--platform linux/amd64 \
	-v "$PWD:/lab" \
	csapp-lab
