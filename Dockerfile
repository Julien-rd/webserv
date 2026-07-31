FROM ubuntu:24.04
RUN apt update && apt install -y build-essential cmake gdb git valgrind
WORKDIR /src
