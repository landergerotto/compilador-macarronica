// Núcleo elemento a elemento em flut (f32): c[i] = a[i] + b[i].
// Na DSL: "c receba a + b;" com vetores flut de N elementos.
//
// Cada c[i] depende só de a[i] e b[i]: não há ordem de soma a preservar, então
// vetorizar não muda nenhum bit do resultado (ao contrário das reduções).
// scripts/bench/medir.py troca 1000000 pelo número de elementos.
func.func @soma_f32(%a: memref<1000000xf32>, %b: memref<1000000xf32>,
                    %c: memref<1000000xf32>) {
  affine.for %i = 0 to 1000000 {
    %x = affine.load %a[%i] : memref<1000000xf32>
    %y = affine.load %b[%i] : memref<1000000xf32>
    %s = arith.addf %x, %y : f32
    affine.store %s, %c[%i] : memref<1000000xf32>
  }
  return
}
