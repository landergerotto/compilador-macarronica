# Ambiente reproduzivel do compilador Macaronica (PjBL 3 e 4, LFC PUCPR 2026.2).
#
# Mesmos pacotes do WSL usado no desenvolvimento: Ubuntu 26.04 com o LLVM 18 do
# apt (llvm, clang, mlir-opt, mlir-translate e libomp), versao fixada abaixo.
#
#   docker build -t macaronica .
#   docker run --rm macaronica                 # make test && make bench
#   docker run --rm -v "$PWD/docs/bench:/macaronica/docs/bench" macaronica make bench
#
# O ultimo comando grava as tabelas, os graficos e as evidencias de volta em
# docs/bench/. Os tempos medidos dependem da maquina hospedeira (veja
# docs/bench/maquina.txt): o que a imagem fixa sao as versoes do compilador e
# das bibliotecas, nao o hardware.
FROM ubuntu:26.04

ARG LLVM_VERSAO=1:18.1.8-20ubuntu8
ENV DEBIAN_FRONTEND=noninteractive

# Prende todos os pacotes do LLVM 18 (llvm, clang, mlir, libomp e as libs que
# eles puxam) na mesma versao usada no WSL.
RUN printf 'Package: /^(lib)?(llvm|clang|mlir|omp)[-a-z0-9.]*18/\nPin: version %s\nPin-Priority: 1001\n' \
        "$LLVM_VERSAO" > /etc/apt/preferences.d/llvm-18 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
        build-essential llvm-18 clang-18 mlir-18-tools libomp-18-dev \
        python3 python3-matplotlib \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /macaronica
COPY . .

CMD ["bash", "-c", "make test && make bench"]
