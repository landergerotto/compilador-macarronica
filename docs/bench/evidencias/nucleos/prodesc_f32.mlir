// Produto escalar em flut (f32): prodesc(a, b) = soma de a[i] * b[i], na
// ordem do programa.
//
// Só a soma recebe  (" fastmath<reassoc>" na variante C1r): o mulf fica
// sem "contract", então o compilador não pode fundir a multiplicação e a soma
// num FMA. Assim a variante isola o efeito da reassociação.
func.func @prodesc_f32(%a: memref<1000000xf32>, %b: memref<1000000xf32>) -> f32 {
  %zero = arith.constant 0.0 : f32
  %r = affine.for %i = 0 to 1000000 iter_args(%acc = %zero) -> (f32) {
    %x = affine.load %a[%i] : memref<1000000xf32>
    %y = affine.load %b[%i] : memref<1000000xf32>
    %p = arith.mulf %x, %y : f32
    %s = arith.addf %acc, %p : f32
    affine.yield %s : f32
  }
  return %r : f32
}
