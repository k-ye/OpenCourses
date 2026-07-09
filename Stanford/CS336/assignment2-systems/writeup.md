## 2.1.4 Nsight System Profiler

RTX 5090

**model=small context-length=256 dtype=bf16**

```sh
# 5090
uv run nsys profile \
  -o nsys_reports/small_ctx256_bf16_full \
  --force-overwrite=true \
  --trace=cuda,cudnn,cublas,nvtx \
  --pytorch=functions-trace,autograd-shapes-nvtx \
  --gpu-metrics-devices=0 \
  -- python -m cs336_systems.benchmark \
    --model-size small \
    --context-length 256 \
    --device cuda \
    --dtype bfloat16
GPU 0: General Metrics for NVIDIA GB20x (any frequency)
Collecting data...
2026-07-08 06:37:50 INFO iter=0: fwd=0.667s bwd=0.239s opt=0.047s
2026-07-08 06:37:50 INFO iter=1: fwd=0.054s bwd=0.100s opt=0.036s
2026-07-08 06:37:51 INFO iter=2: fwd=0.049s bwd=0.096s opt=0.034s
2026-07-08 06:37:51 INFO iter=3: fwd=0.046s bwd=0.093s opt=0.033s
2026-07-08 06:37:51 INFO iter=4: fwd=0.046s bwd=0.098s opt=0.034s
2026-07-08 06:37:51 INFO iter=5: fwd=0.046s bwd=0.096s opt=0.035s
2026-07-08 06:37:51 INFO iter=6: fwd=0.047s bwd=0.097s opt=0.034s
2026-07-08 06:37:52 INFO iter=7: fwd=0.047s bwd=0.098s opt=0.034s
2026-07-08 06:37:52 INFO iter=8: fwd=0.048s bwd=0.098s opt=0.033s
2026-07-08 06:37:52 INFO iter=9: fwd=0.047s bwd=0.097s opt=0.035s
2026-07-08 06:37:52 INFO iter=10: fwd=0.047s bwd=0.100s opt=0.034s
2026-07-08 06:37:52 INFO iter=11: fwd=0.047s bwd=0.099s opt=0.036s
2026-07-08 06:37:53 INFO iter=12: fwd=0.047s bwd=0.098s opt=0.034s
2026-07-08 06:37:53 INFO iter=13: fwd=0.047s bwd=0.098s opt=0.034s
2026-07-08 06:37:53 INFO iter=14: fwd=0.045s bwd=0.097s opt=0.034s
Benchmark with model_size=small mode=full
fwd: 0.047s ± 0.001s
bwd: 0.098s ± 0.001s
opt: 0.034s ± 0.001s
Generating '/tmp/nsys-report-1d91.qdstrm'
[1/1] [========================100%] small_ctx256_bf16_full.nsys-rep
```

---

**model=large context-length=512 dtype=bf16**

```sh
# 5090
uv run nsys profile \
    -o nsys_reports/large_ctx512_bf16_full \
    --force-overwrite=true \
    --trace=cuda,cudnn,cublas,nvtx \
    --pytorch=functions-trace,autograd-shapes-nvtx --gpu-metrics-devices=0 \
    -- python -m cs336_systems.benchmark \
      --model-size large \
      --context-length 512 \
      --device cuda \
      --dtype bfloat16
GPU 0: General Metrics for NVIDIA GB20x (any frequency)
Collecting data...
2026-07-08 06:39:40 INFO iter=0: fwd=0.682s bwd=0.408s opt=0.127s
2026-07-08 06:39:41 INFO iter=1: fwd=0.155s bwd=0.274s opt=0.095s
2026-07-08 06:39:41 INFO iter=2: fwd=0.139s bwd=0.285s opt=0.095s
2026-07-08 06:39:42 INFO iter=3: fwd=0.128s bwd=0.280s opt=0.096s
2026-07-08 06:39:42 INFO iter=4: fwd=0.126s bwd=0.285s opt=0.093s
2026-07-08 06:39:43 INFO iter=5: fwd=0.124s bwd=0.294s opt=0.099s
2026-07-08 06:39:43 INFO iter=6: fwd=0.123s bwd=0.295s opt=0.095s
2026-07-08 06:39:44 INFO iter=7: fwd=0.125s bwd=0.288s opt=0.105s
2026-07-08 06:39:44 INFO iter=8: fwd=0.129s bwd=0.282s opt=0.094s
2026-07-08 06:39:45 INFO iter=9: fwd=0.129s bwd=0.279s opt=0.094s
2026-07-08 06:39:46 INFO iter=10: fwd=0.131s bwd=0.275s opt=0.095s
2026-07-08 06:39:46 INFO iter=11: fwd=0.131s bwd=0.277s opt=0.103s
2026-07-08 06:39:47 INFO iter=12: fwd=0.127s bwd=0.281s opt=0.100s
2026-07-08 06:39:47 INFO iter=13: fwd=0.131s bwd=0.287s opt=0.094s
2026-07-08 06:39:48 INFO iter=14: fwd=0.128s bwd=0.282s opt=0.096s
Benchmark with model_size=large mode=full
fwd: 0.128s ± 0.003s
bwd: 0.284s ± 0.007s
opt: 0.098s ± 0.004s
Generating '/tmp/nsys-report-e8e0.qdstrm'
[1/1] [========================100%] large_ctx512_bf16_full.nsys-rep
```

**model=large context-length=1024 dtype=bf16**

```sh
# 5090

 uv run nsys profile \
    -o nsys_reports/large_ctx1024_bf16_full \
    --force-overwrite=true \
    --trace=cuda,cudnn,cublas,nvtx \
    --pytorch=functions-trace,autograd-shapes-nvtx \
    --gpu-metrics-devices=0 \
    -- python -m cs336_systems.benchmark \
      --model-size large \
      --context-length 1024 \
      --device cuda \
      --dtype bfloat16
GPU 0: General Metrics for NVIDIA GB20x (any frequency)
Collecting data...
2026-07-08 06:58:08 INFO iter=0: fwd=0.864s bwd=0.489s opt=0.140s
2026-07-08 06:58:08 INFO iter=1: fwd=0.146s bwd=0.340s opt=0.102s
2026-07-08 06:58:09 INFO iter=2: fwd=0.153s bwd=0.309s opt=0.109s
2026-07-08 06:58:09 INFO iter=3: fwd=0.142s bwd=0.347s opt=0.103s
2026-07-08 06:58:10 INFO iter=4: fwd=0.148s bwd=0.341s opt=0.101s
2026-07-08 06:58:11 INFO iter=5: fwd=0.151s bwd=0.344s opt=0.109s
2026-07-08 06:58:11 INFO iter=6: fwd=0.142s bwd=0.349s opt=0.113s
2026-07-08 06:58:12 INFO iter=7: fwd=0.154s bwd=0.337s opt=0.113s
2026-07-08 06:58:13 INFO iter=8: fwd=0.141s bwd=0.351s opt=0.113s
2026-07-08 06:58:13 INFO iter=9: fwd=0.150s bwd=0.338s opt=0.110s
2026-07-08 06:58:14 INFO iter=10: fwd=0.147s bwd=0.342s opt=0.101s
2026-07-08 06:58:15 INFO iter=11: fwd=0.154s bwd=0.349s opt=0.109s
2026-07-08 06:58:15 INFO iter=12: fwd=0.155s bwd=0.304s opt=0.102s
2026-07-08 06:58:16 INFO iter=13: fwd=0.153s bwd=0.308s opt=0.111s
2026-07-08 06:58:16 INFO iter=14: fwd=0.143s bwd=0.300s opt=0.101s
Benchmark with model_size=large mode=full
fwd: 0.149s ± 0.006s
bwd: 0.332s ± 0.020s
opt: 0.108s ± 0.005s
Generating '/tmp/nsys-report-94a5.qdstrm'
[1/1] [========================100%] large_ctx1024_bf16_full.nsys-rep
```

(a)

Nsight's forward doesn't match that measured from Python side. Nsight's didn't include the CUDA sync time.

(b)

Mostly GEMM kernels.

```
Time	Total Time	Instances	Avg	Med	Min	Max	StdDev	Name
27.6%	9.305 ms	73	127.462 μs	126.274 μs	122.818 μs	249.156 μs	14.486 μs	void cutlass::Kernel2<cutlass_80_tensorop_bf16_s16816gemm_relu_bf16_256x128_32x3_tn_align8>(T1::Params)
13.8%	4.664 ms	144	32.387 μs	32.384 μs	32.096 μs	32.737 μs	132 ns	void cutlass::Kernel2<cutlass_80_tensorop_bf16_s16816gemm_relu_bf16_128x128_32x4_tn_align8>(T1::Params)
12.7%	4.274 ms	36	118.713 μs	118.434 μs	117.762 μs	121.154 μs	835 ns	void cutlass::Kernel2<cutlass_80_tensorop_bf16_s16816gemm_relu_bf16_64x256_32x4_tn_align8>(T1::Params)
```

(c)

```
Time	Total Time	Instances	Avg	Med	Min	Max	StdDev	Name
5.9%	1.998 ms	361	5.535 μs	5.024 μs	4.896 μs	7.744 μs	980 ns	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl<at::native::BinaryFunctor<float, float, float, at::native::binary_internal::MulFunctor<float>>>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
4.4%	1.493 ms	144	10.369 μs	6.656 μs	4.896 μs	23.681 μs	7.445 μs	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::direct_copy_kernel_cuda(at::TensorIteratorBase &)::[lambda() (instance 3)]::operator ()() const::[lambda() (instance 12)]::operator ()() const::[lambda(c10::BFloat16) (instance 1)]>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
```

(e)

nvtx: "computing attention scores"

```
Time	Total Time	Instances	Avg	Med	Min	Max	StdDev	Name
44.3%	34.688 μs	1	34.688 μs	34.688 μs	34.688 μs	34.688 μs	0 ns	void cutlass::Kernel2<cutlass_80_wmma_tensorop_bf16_s161616gemm_bf16_32x32_64x1_nn_align8>(T1::Params)
36.1%	28.288 μs	2	14.144 μs	14.144 μs	5.024 μs	23.264 μs	12.897 μs	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::direct_copy_kernel_cuda(at::TensorIteratorBase &)::[lambda() (instance 3)]::operator ()() const::[lambda() (instance 12)]::operator ()() const::[lambda(c10::BFloat16) (instance 1)]>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
19.6%	15.361 μs	1	15.361 μs	15.361 μs	15.361 μs	15.361 μs	0 ns	void at::native::vectorized_elementwise_kernel<(int)4, at::native::BUnaryFunctor<c10::BFloat16, c10::BFloat16, c10::BFloat16, at::native::binary_internal::MulFunctor<float>>, std::array<char *, (unsigned long)2>>(int, T2, T3)
```

78.337 us

nvtx: "computing softmax"

```
Time	Total Time	Instances	Avg	Med	Min	Max	StdDev	Name
33.9%	35.104 μs	1	35.104 μs	35.104 μs	35.104 μs	35.104 μs	0 ns	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::BinaryFunctor<c10::BFloat16, c10::BFloat16, c10::BFloat16, at::native::binary_internal::DivFunctor<c10::BFloat16>>>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
24.8%	25.697 μs	1	25.697 μs	25.697 μs	25.697 μs	25.697 μs	0 ns	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::CUDAFunctor_add<c10::BFloat16>>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
17.7%	18.304 μs	1	18.304 μs	18.304 μs	18.304 μs	18.304 μs	0 ns	void at::native::reduce_kernel<(int)512, (int)1, at::native::ReduceOp<c10::BFloat16, at::native::MaxOps<c10::BFloat16>, unsigned int, c10::BFloat16, (int)4, (int)4>>(T3)
13.9%	14.400 μs	1	14.400 μs	14.400 μs	14.400 μs	14.400 μs	0 ns	void at::native::vectorized_elementwise_kernel<(int)4, at::native::exp_kernel_cuda(at::TensorIteratorBase &)::[lambda() (instance 2)]::operator ()() const::[lambda() (instance 4)]::operator ()() const::[lambda(c10::BFloat16) (instance 1)], std::array<char *, (unsigned long)2>>(int, T2, T3)
9.8%	10.144 μs	1	10.144 μs	10.144 μs	10.144 μs	10.144 μs	0 ns	void at::native::reduce_kernel<(int)512, (int)1, at::native::ReduceOp<c10::BFloat16, at::native::func_wrapper_t<c10::BFloat16, at::native::sum_functor<c10::BFloat16, float, c10::BFloat16>::operator ()(at::TensorIterator &)::[lambda(float, float) (instance 1)]>, unsigned int, c10::BFloat16, (int)4, (int)8>>(T3)
```

103.649 us


nvtx: "computing final matmul"

```
Time	Total Time	Instances	Avg	Med	Min	Max	StdDev	Name
47.4%	35.104 μs	1	35.104 μs	35.104 μs	35.104 μs	35.104 μs	0 ns	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::BinaryFunctor<c10::BFloat16, c10::BFloat16, c10::BFloat16, at::native::binary_internal::DivFunctor<c10::BFloat16>>>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
40.8%	30.177 μs	1	30.177 μs	30.177 μs	30.177 μs	30.177 μs	0 ns	void cutlass::Kernel2<cutlass_80_wmma_tensorop_bf16_s161616gemm_bf16_32x32_64x1_nn_align8>(T1::Params)
11.8%	8.768 μs	1	8.768 μs	8.768 μs	8.768 μs	8.768 μs	0 ns	void at::native::elementwise_kernel<(int)128, (int)4, void at::native::gpu_kernel_impl_nocast<at::native::direct_copy_kernel_cuda(at::TensorIteratorBase &)::[lambda() (instance 3)]::operator ()() const::[lambda() (instance 12)]::operator ()() const::[lambda(c10::BFloat16) (instance 1)]>(at::TensorIteratorBase &, const T1 &)::[lambda(int) (instance 1)]>(int, T3)
```

38.7us

Note that NSight GUI reports CUDA kernels whose **execution** overlaps with an NVTX range. That can be misleading because GPU kernel execs are async, and kernels from the previous range could leak into a later range. What we really want is the kernels that are **launched** within an NVTX range. Each GPU kernel launch is tagged with a `correlationId`, which is also recorded throughout its execution span. We need to look at the kernels whose launch correlationId falls within an NVTX range.

The `"elementwise_kernel ... DivFunctor ..."` kernel in "computing final matmul" actually belongs to the "computing softmax" range. To see why:

1. "computing final matmul" has no elementwise division at all. softmax does: softmax = exp(x−max)/sum, which is exactly max-reduce → subtract → exp → sum-reduce → divide.
2. the logged line are byte-for-byte identical.



computing final matmul (launch-correlated):  gemm 30.1µs + direct_copy 8.5µs = 38.7µs
                        (GUI overlap):   gemm 30.2µs + copy 8.8µs + PHANTOM Div 35.1µs = 74µs

So the real final matmul is ~38.7 µs, not 74 µs. So softmax (103.6 µs) beats matmul by ~2.7×.

┌─────────────────────┬──────────────────────────────────────┬─────────┬──────────────────────────┐
│                     │         contraction dim (K)          │  tile   │      kernel picked       │
├─────────────────────┼──────────────────────────────────────┼─────────┼──────────────────────────┤
│ FFN / projections   │ 1280 (large)                         │ 256×128 │ s16816 big tensorop GEMM │
├─────────────────────┼──────────────────────────────────────┼─────────┼──────────────────────────┤
│ attention QK^T, P·V │ d_head ≈ 64 (tiny), batched over B·H │ 32×32   │ s161616 wmma variant     │
└─────────────────────┴──────────────────────────────────────┴─────────┴──────────────────────────┘
