# Benchmarks

# Testing machine (inxi -Fxz) irrelevant details omitted

System:
  Kernel: 6.8.0-117-generic arch: x86_64 bits: 64 compiler: gcc v: 13.3.0
  Desktop: Unity v: N/A Distro: Ubuntu 24.04.4 LTS (Noble Numbat)
Machine:
  Type: Laptop System: ...
...
CPU:
  Info: 8-core model: Intel Core i9-9900K bits: 64 type: MT MCP
    arch: Coffee Lake rev: C cache: L1: 512 KiB L2: 2 MiB L3: 16 MiB
  Speed (MHz): avg: 1324 high: 5001 min/max: 800/5000 cores: 1: 800 2: 5001
    3: 800 4: 800 5: 800 6: 800 7: 800 8: 800 9: 800 10: 4996 11: 800 12: 800
    13: 800 14: 800 15: 800 16: 800 bogomips: 115200
  Flags: avx avx2 ht lm nx pae sse sse2 sse3 sse4_1 sse4_2 ssse3 vmx
...
Info:
  Memory: total: 64 GiB note: est. available: 62.73 GiB used: 5.63 GiB (9.0%)
...

## Counting from 0 to 9999999

### Non-jitted execution

#### ceps vs. python


```bash 

tomas@tomas-big-dev-laptop:~/dev/machines4ceps$ time ceps vm/benchmarks/counting_a.ceps
S.Initial- S.Final+ 

real    0m0,360s
user    0m0,358s
sys     0m0,003s
tomas@tomas-big-dev-laptop:~/dev/machines4ceps$ time python3 vm/benchmarks/counting_a.py 


real    0m0,550s
user    0m0,545s
sys     0m0,003s
tomas@tomas-big-dev-laptop:~/dev/machines4ceps$ 
```

ceps: *0.360* seconds wallclock time
python: *0.550* seconds wallclock time
==> ceps is ~35% faster.