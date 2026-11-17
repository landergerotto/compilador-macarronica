module {
  func.func @soma_i64(%arg0: memref<1000000xi64>, %arg1: memref<1000000xi64>, %arg2: memref<1000000xi64>) {
    %c0 = arith.constant 0 : index
    %c1000000 = arith.constant 1000000 : index
    %c1 = arith.constant 1 : index
    omp.parallel {
      omp.wsloop for  (%arg3) : index = (%c0) to (%c1000000) step (%c1) {
        %0 = memref.load %arg0[%arg3] : memref<1000000xi64>
        %1 = memref.load %arg1[%arg3] : memref<1000000xi64>
        %2 = arith.addi %0, %1 : i64
        memref.store %2, %arg2[%arg3] : memref<1000000xi64>
        omp.yield
      }
      omp.terminator
    }
    return
  }
}

