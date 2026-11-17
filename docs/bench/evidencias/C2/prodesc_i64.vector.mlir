module {
  func.func @prodesc_i64(%arg0: memref<1000000xi64>, %arg1: memref<1000000xi64>) -> i64 {
    %c0_i64 = arith.constant 0 : i64
    %cst = arith.constant dense<0> : vector<8xi64>
    %0 = affine.for %arg2 = 0 to 1000000 step 8 iter_args(%arg3 = %cst) -> (vector<8xi64>) {
      %2 = vector.transfer_read %arg0[%arg2], %c0_i64 : memref<1000000xi64>, vector<8xi64>
      %3 = vector.transfer_read %arg1[%arg2], %c0_i64 : memref<1000000xi64>, vector<8xi64>
      %4 = arith.muli %2, %3 : vector<8xi64>
      %5 = arith.addi %arg3, %4 : vector<8xi64>
      affine.yield %5 : vector<8xi64>
    }
    %1 = vector.reduction <add>, %0 : vector<8xi64> into i64
    return %1 : i64
  }
}

