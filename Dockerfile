# ── Stage 1: build ────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Base tools
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget ca-certificates gnupg lsb-release \
        g++ make \
 && rm -rf /var/lib/apt/lists/*

# 2. Add the LLVM 18 apt repository with a properly dearmored GPG key.
#    Using /etc/apt/keyrings/ + [signed-by=...] is the modern, reliable method.
RUN mkdir -p /etc/apt/keyrings \
 && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor > /etc/apt/keyrings/llvm.gpg \
 && echo "deb [signed-by=/etc/apt/keyrings/llvm.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" \
        > /etc/apt/sources.list.d/llvm-18.list \
 && apt-get update

# 3. LLVM 18 / Clang 18 dev libraries + JSON headers
RUN apt-get install -y --no-install-recommends \
        llvm-18-dev \
        libclang-cpp18-dev \
        clang-18 \
        libtinfo-dev \
        zlib1g-dev \
        nlohmann-json3-dev \
 && rm -rf /var/lib/apt/lists/*

# 4. Sanity check: fail fast if the build toolchain is broken
RUN llvm-config-18 --version

# 5. Build AETHER
WORKDIR /src
COPY . .
RUN make clean && make


# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Add the LLVM 18 apt repository (same method as builder)
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget ca-certificates gnupg \
 && mkdir -p /etc/apt/keyrings \
 && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor > /etc/apt/keyrings/llvm.gpg \
 && echo "deb [signed-by=/etc/apt/keyrings/llvm.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" \
        > /etc/apt/sources.list.d/llvm-18.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
        libclang-cpp18 \
        libtinfo6 \
        zlib1g \
 && rm -rf /var/lib/apt/lists/*

# 2. System C headers so AETHER can parse #include <stdint.h> etc. in user code
RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc \
        libc6-dev \
 && rm -rf /var/lib/apt/lists/*

# 3. Python 3 + Z3 solver
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3 python3-pip \
 && pip3 install --no-cache-dir z3-solver \
 && rm -rf /var/lib/apt/lists/*

# 4. Copy the compiled binary and knowledge base from the builder stage
COPY --from=builder /src/aether           /usr/local/bin/aether
COPY --from=builder /src/KnowledgeBase.json /usr/local/bin/KnowledgeBase.json

# 5. The user mounts their project into /work
WORKDIR /work

ENTRYPOINT ["aether"]
