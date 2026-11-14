# Relatório de otimização — PjBL 4, Partes B e C

> Esqueleto. Cada parte é escrita pela frente responsável (veja
> `docs/plano-pjbl3-4.md`): a Parte B pela frente DSL/MLIR (`luiz/dsl-mlir`) e a
> Parte C pela frente de benchmark (`luiz/bench`). Uma frente não edita a seção
> da outra.

## Parte B — Geração e lowering via MLIR

<!-- frente dsl-mlir: estratégia (híbrida) e justificativa; núcleo emitido pelo
compilador; o mesmo núcleo em três níveis do pipeline (affine, vector/scf,
dialeto llvm) com o que cada passo preservou e descartou; scripts/pipeline.sh. -->

## Parte C — Estudo experimental de otimização

<!-- frente bench: máquina (CPU, núcleos, AVX2); núcleos e tamanhos (10^6, 10^7);
configurações C0 a C3; mediana de 5 execuções, compilação separada da execução;
tabelas e gráficos gerados por scripts/bench; evidências (-Rpass, trechos de IR);
pergunta obrigatória sobre a redução de flut sem -ffast-math. -->
