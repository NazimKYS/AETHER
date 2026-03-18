# ── Stage 1: build ────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Base tools + LLVM 18 apt repository
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget ca-certificates gnupg lsb-release \
 && wget -qO /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
        https://apt.llvm.org/llvm-snapshot.gpg.key \
 && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" \
        > /etc/apt/sources.list.d/llvm-18.list \
 && apt-get update

# 2. LLVM 18 / Clang 18 dev libraries + build tools
RUN apt-get install -y --no-install-recommends \
        llvm-18-dev \
        libclang-cpp18-dev \
        clang-18 \
        libtinfo-dev \
        zlib1g-dev \
        g++ \
        make \
        nlohmann-json3-dev \
 && rm -rf /var/lib/apt/lists/*

# 3. Build the tool
WORKDIR /src
COPY . .
RUN make clean && make


# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. LLVM 18 runtime library + system headers needed when analysing C files
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget ca-certificates gnupg \
 && wget -qO /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
        https://apt.llvm.org/llvm-snapshot.gpg.key \
 && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" \
        > /etc/apt/sources.list.d/llvm-18.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
        libclang-cpp18 \
        libtinfo6 \
        zlib1g \
        # system headers so the tool can parse #include <stdint.h> etc.
        gcc \
        libc6-dev \
 && rm -rf /var/lib/apt/lists/*

# 2. Python 3 + Z3 solver
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3 python3-pip \
 && pip3 install --no-cache-dir z3-solver \
 && rm -rf /var/lib/apt/lists/*

# 3. Copy the compiled binary from the builder stage
COPY --from=builder /src/aether /usr/local/bin/aether

# 4. The user mounts their project into /work
WORKDIR /work

ENTRYPOINT ["aether"]
