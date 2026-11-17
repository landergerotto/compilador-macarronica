module {
  func.func @somavet_i64(%arg0: memref<1000000xi64>) -> i64 {
    %c0_i64 = arith.constant 0 : i64
    %cst = arith.constant dense<0> : vector<8xi64>
    %0 = affine.for %arg1 = 0 to 1000000 step 8 iter_args(%arg2 = %cst) -> (vector<8xi64>) {
      %2 = vector.transfer_read %arg0[%arg1], %c0_i64 : memref<1000000xi64>, vector<8xi64>
      %3 = arith.addi %arg2, %2 : vector<8xi64>
      affine.yield %3 : vector<8xi64>
    }
    %1 = vector.reduction <add>, %0 : vector<8xi64> into i64
    return %1 : i64
  }
}

