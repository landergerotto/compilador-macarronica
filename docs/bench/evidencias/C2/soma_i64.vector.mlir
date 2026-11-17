module {
  func.func @soma_i64(%arg0: memref<1000000xi64>, %arg1: memref<1000000xi64>, %arg2: memref<1000000xi64>) {
    %c0_i64 = arith.constant 0 : i64
    affine.for %arg3 = 0 to 1000000 step 8 {
      %0 = vector.transfer_read %arg0[%arg3], %c0_i64 : memref<1000000xi64>, vector<8xi64>
      %1 = vector.transfer_read %arg1[%arg3], %c0_i64 : memref<1000000xi64>, vector<8xi64>
      %2 = arith.addi %0, %1 : vector<8xi64>
      vector.transfer_write %2, %arg2[%arg3] : vector<8xi64>, memref<1000000xi64>
    }
    return
  }
}

