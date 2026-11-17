module {
  func.func @somavet_f32(%arg0: memref<1000000xf32>) -> f32 {
    %cst = arith.constant 0.000000e+00 : f32
    %cst_0 = arith.constant dense<0.000000e+00> : vector<8xf32>
    %0 = affine.for %arg1 = 0 to 1000000 step 8 iter_args(%arg2 = %cst_0) -> (vector<8xf32>) {
      %2 = vector.transfer_read %arg0[%arg1], %cst : memref<1000000xf32>, vector<8xf32>
      %3 = arith.addf %arg2, %2 : vector<8xf32>
      affine.yield %3 : vector<8xf32>
    }
    %1 = vector.reduction <add>, %0 : vector<8xf32> into f32
    return %1 : f32
  }
}

