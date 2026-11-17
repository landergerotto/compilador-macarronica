module {
  omp.reduction.declare @__scf_reduction : f32 init {
  ^bb0(%arg0: f32):
    %0 = llvm.mlir.constant(0.000000e+00 : f32) : f32
    omp.yield(%0 : f32)
  } combiner {
  ^bb0(%arg0: f32, %arg1: f32):
    %0 = arith.addf %arg0, %arg1 : f32
    omp.yield(%0 : f32)
  } atomic {
  ^bb0(%arg0: !llvm.ptr, %arg1: !llvm.ptr):
    %0 = llvm.load %arg1 : !llvm.ptr -> f32
    %1 = llvm.atomicrmw fadd %arg0, %0 monotonic : !llvm.ptr, f32
    omp.yield
  }
  func.func @prodesc_f32(%arg0: memref<1000000xf32>, %arg1: memref<1000000xf32>) -> f32 {
    %cst = arith.constant 0.000000e+00 : f32
    %c0 = arith.constant 0 : index
    %c1000000 = arith.constant 1000000 : index
    %c1 = arith.constant 1 : index
    %0 = llvm.mlir.constant(1 : i64) : i64
    %1 = llvm.alloca %0 x f32 : (i64) -> !llvm.ptr
    llvm.store %cst, %1 : f32, !llvm.ptr
    omp.parallel {
      omp.wsloop reduction(@__scf_reduction -> %1 : !llvm.ptr) for  (%arg2) : index = (%c0) to (%c1000000) step (%c1) {
        memref.alloca_scope  {
          %4 = memref.load %arg0[%arg2] : memref<1000000xf32>
          %5 = memref.load %arg1[%arg2] : memref<1000000xf32>
          %6 = arith.mulf %4, %5 : f32
          omp.reduction %6, %1 : f32, !llvm.ptr
        }
        omp.yield
      }
      omp.terminator
    }
    %2 = llvm.load %1 : !llvm.ptr -> f32
    %3 = arith.addf %2, %cst : f32
    return %3 : f32
  }
}

