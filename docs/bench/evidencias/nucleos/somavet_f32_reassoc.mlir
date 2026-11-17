// Redução em flut (f32): somavet(a) = (((0 + a[0]) + a[1]) + ...) + a[N-1].
//
// A ordem acima é a do programa: cada soma arredonda, e somar em outra ordem
// dá outro resultado.  fastmath<reassoc> fica vazio na versão exata; na variante C1r da
// pergunta obrigatória vira " fastmath<reassoc>", o atributo que autoriza o
// compilador a reassociar a soma (vira "fadd reassoc" no LLVM IR).
func.func @somavet_f32(%a: memref<1000000xf32>) -> f32 {
  %zero = arith.constant 0.0 : f32
  %r = affine.for %i = 0 to 1000000 iter_args(%acc = %zero) -> (f32) {
    %x = affine.load %a[%i] : memref<1000000xf32>
    %s = arith.addf %acc, %x fastmath<reassoc> : f32
    affine.yield %s : f32
  }
  return %r : f32
}
