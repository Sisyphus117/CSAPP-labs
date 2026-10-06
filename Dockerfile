FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y ca-certificates && \
    rm -rf /var/lib/apt/lists/*

RUN printf '%s\n' \
    'deb https://mirrors.ustc.edu.cn/ubuntu/ jammy main restricted universe multiverse' \
    'deb https://mirrors.ustc.edu.cn/ubuntu/ jammy-updates main restricted universe multiverse' \
    'deb https://mirrors.ustc.edu.cn/ubuntu/ jammy-security main restricted universe multiverse' \
    > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        gdb \
        git \
        vim \
        flex \
        bison \
        tk-dev tcl-dev \
        valgrind \
        curl \
        net-tools \
        python3 && \
    rm -rf /var/lib/apt/lists/*

RUN dpkg --add-architecture i386 && \
	apt-get update && apt-get install -y  gcc-multilib && \
	rm -rf /var/lib/apt/lists/*

WORKDIR /lab

CMD ["bash"]
