// Redução em inteira (i64): somavet(a) = a[0] + a[1] + ... + a[N-1].
//
// O acumulador passa de uma iteração para a outra pelo iter_args do
// affine.for (é assim que o MLIR representa uma redução sem memória).
// A soma de inteiros em complemento de dois é associativa mesmo com overflow
// (aritmética módulo 2^64), então qualquer ordem dá o mesmo resultado.
func.func @somavet_i64(%a: memref<1000000xi64>) -> i64 {
  %zero = arith.constant 0 : i64
  %r = affine.for %i = 0 to 1000000 iter_args(%acc = %zero) -> (i64) {
    %x = affine.load %a[%i] : memref<1000000xi64>
    %s = arith.addi %acc, %x : i64
    affine.yield %s : i64
  }
  return %r : i64
}
