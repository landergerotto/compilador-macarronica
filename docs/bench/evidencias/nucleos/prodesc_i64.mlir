// Produto escalar em inteira (i64): prodesc(a, b) = soma de a[i] * b[i].
//
// Ponto de discussão: o AVX2 não tem multiplicação de inteiros de 64 bits
// (vpmullq só existe no AVX-512DQ). Vetorizado, cada arith.muli em i64 vira
// três vpmuludq (32x32->64) mais deslocamentos e somas.
func.func @prodesc_i64(%a: memref<1000000xi64>, %b: memref<1000000xi64>) -> i64 {
  %zero = arith.constant 0 : i64
  %r = affine.for %i = 0 to 1000000 iter_args(%acc = %zero) -> (i64) {
    %x = affine.load %a[%i] : memref<1000000xi64>
    %y = affine.load %b[%i] : memref<1000000xi64>
    %p = arith.muli %x, %y : i64
    %s = arith.addi %acc, %p : i64
    affine.yield %s : i64
  }
  return %r : i64
}
