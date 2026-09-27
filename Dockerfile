FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y ca-certificates && \
    rm -rf /var/lib/apt/lists/*

RUN printf '%s\n' \
    'deb https://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse' \
    'deb https://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse' \
    'deb https://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse' \
    > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        gdb \
        git \
        python3 && \
    rm -rf /var/lib/apt/lists/*

RUN dpkg --add-architecture i386 && \
	apt-get update && apt-get install -y  gcc-multilib && \
	rm -rf /var/lib/apt/lists/*

WORKDIR /lab

CMD ["bash"]
