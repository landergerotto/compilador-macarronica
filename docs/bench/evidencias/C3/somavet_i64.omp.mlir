module {
  omp.reduction.declare @__scf_reduction : i64 init {
  ^bb0(%arg0: i64):
    %0 = llvm.mlir.constant(0 : i64) : i64
    omp.yield(%0 : i64)
  } combiner {
  ^bb0(%arg0: i64, %arg1: i64):
    %0 = arith.addi %arg0, %arg1 : i64
    omp.yield(%0 : i64)
  } atomic {
  ^bb0(%arg0: !llvm.ptr, %arg1: !llvm.ptr):
    %0 = llvm.load %arg1 : !llvm.ptr -> i64
    %1 = llvm.atomicrmw add %arg0, %0 monotonic : !llvm.ptr, i64
    omp.yield
  }
  func.func @somavet_i64(%arg0: memref<1000000xi64>) -> i64 {
    %c0_i64 = arith.constant 0 : i64
    %c0 = arith.constant 0 : index
    %c1000000 = arith.constant 1000000 : index
    %c1 = arith.constant 1 : index
    %0 = llvm.mlir.constant(1 : i64) : i64
    %1 = llvm.alloca %0 x i64 : (i64) -> !llvm.ptr
    llvm.store %c0_i64, %1 : i64, !llvm.ptr
    omp.parallel {
      omp.wsloop reduction(@__scf_reduction -> %1 : !llvm.ptr) for  (%arg1) : index = (%c0) to (%c1000000) step (%c1) {
        memref.alloca_scope  {
          %3 = memref.load %arg0[%arg1] : memref<1000000xi64>
          omp.reduction %3, %1 : i64, !llvm.ptr
        }
        omp.yield
      }
      omp.terminator
    }
    %2 = llvm.load %1 : !llvm.ptr -> i64
    return %2 : i64
  }
}

