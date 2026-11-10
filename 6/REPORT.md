# Pulse Analysis Report

## 1. Prediction (written before any timing)

I predict that reading the pulse data from files will be the slowest operation in the program, with printing output being the second slowest. This is because everything else is (presumably, I don't know how the STL is implemented) just number crunching over contiguous data, while I/O operations must wait while the kernel actually moves data from disk into the program's memory and back to `cout`.

In fact, unless the data files are unusually large, I would expect that if this is not the case, I must be doing something catastrophically wrong in my program logic.

## 2. Baseline (commit tagged `baseline`)

Data: 20 files x 1000000 samples, seed 42. Build: -O2.

The generated `gen_data.cpp` looks like it is doing what it should at a quick glance. It does some weird stuff like including but not using `<cstdint>`, needlessly cloning `argv[1]`, use of `vector::push_back()` rather than `vector::emplace_back()`, and pointlessly `static_cast<long long>`ing the result of `llround()`. It compiles without error.

 The generated pulses do go down, and the pulse start positions in `truth.txt` are close but not exactly equal to the pulse start positions found by the `pulse` program.

Running `pulse_debug` on the generated edge-cases reveals the following error:

```
thread 5092 panic: applying non-zero offset 8 to null pointer

~/src/uvu.cs3370/6/main.cpp:46: 0x7ff627bb183c in main (main.obj)
	smoothed[i] = (3 * (data[i - 1] + data[i] + data[i + 1])
```

I assumed that a file with less than three samples would fail the `i < data.size() - 3` check before the first loop iteration, but `i` is an unsigned type so `0 - 3` underflows to some stupid huge value instead.

| Stage           | Median ms | Share of total |
|-----------------|-----------|---------------:|
| reading         | 1358.9500 | 91.5150224346% |
| smoothing       |   18.7148 |  1.2673304566% |
| detection       |   12.6677 |  0.8507649939% |
| piggyback test  |    2.9813 |  0.2007064135% |
| area            |    4.2826 |  0.2905041013% |
| printing        |   86.9028 |  5.8756716001% |

## 3. Suggestions

| # | Suggestion                                                   | Category    | Decision | Reason | Effect |
|---|--------------------------------------------------------------|-------------|----------|--------|--------|
| 1 | Guard against undersized input files                         | safety      |          |        |        |
| 2 | Prevent signed integer overflow                              | safety      |          |        |        |
| 3 | Optimize integer file reading                                | performance |          |        |        |
| 4 | Reduce the number of printing timers                         | performance |          |        |        |
| 5 | Avoid unnecesary allocation when smoothing                   | performance |          |        |        |
| 6 | Validate configuration parameters before using them as sizes | safety      |          |        |        |
| 7 | Avoid redundant bounds calculations                          | readability |          |        |        |
| 8 | Make the data-processing stages easier to isolate            | readability |          |        |        |

## 4. Final timing

| Stage | Baseline ms | Final ms | Change |
|-------|-------------|----------|--------|

## 5. Reflection

> 1. How did your prediction compare with the timing report? Where does the time actually go?

> 2. Choose one suggestion you rejected, or one that turned out to be wrong. What evidence decided it?

> 3. Did `gen_data.cpp` or `stage_timer.h` contain a mistake? How did you find it, or how did you check that there was none?

> 4. Which suggestion taught you something about C++ that you did not know before?

