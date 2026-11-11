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
| 1 | Guard against undersized input files                         | safety      | accept   | asan previously caught this | edge cases no longer crash |
| 2 | Prevent signed integer overflow                              | safety      | accept   | I did already consider this for P5 so I agree | extreme values are handled correctly; smoothing performance suffers significantly |
| 3 | Optimize integer file reading                                | performance | accept   | Greatest potential performance increase | Median reading phase is 514ms down from 1359ms |
| 4 | Reduce the number of printing timers                         | performance | accept   | The clanker put a mutex in the timer totals; every timer not created is a mutex not interacted with | Median printing phase is 3.7ms down from 86ms |
| 5 | Avoid unnecesary allocation when smoothing                   | performance | reject   | Makes smoothing phase perform worse | Median smoothing phase is 49ms up from 18ms |
| 6 | Validate configuration parameters before using them as sizes | safety      | reject   | Parameters are already validated |        |
| 7 | Avoid redundant bounds calculations                          | readability | accept   | `next <= end < data.size()` is an invariant |        |
| 8 | Make the data-processing stages easier to isolate            | readability | reject   | `main()` is 130 loc, it's fine as-is |        |

## 4. Final timing

| Stage           | Baseline ms | Final ms | Change |
|-----------------|------------:|---------:|-------:|
| reading         |   1358.9500 | 534.4590 |  2.54x |
| smoothing       |     18.7148 |  53.4569 |  0.35x |
| detection       |     12.6677 |  27.4625 |  0.46x |
| piggyback test  |      2.9813 |   2.9932 |  0.99x |
| area            |      4.2826 |   7.9801 |  0.54x |
| printing        |     86.9028 |   2.7207 | 31.94x |

## 5. Reflection

> 1. How did your prediction compare with the timing report? Where does the time actually go?

My primary prediction was spot-on. Initially, reading in files took (92% of the runtime); after ditching streams and iterators for C-style I/O, it now only takes 534ms (85% of the runtime).

My secondary prediction was not so accurate. Printing was initially the second longest phase, but that certainly had more to do with the way I was timing it and the fact that every timer's destructor acquired a mutex. Circumventing that actually brought it down to be the shortest phase of all.

The smoothing is actually the second longest phase; doing the arithmetic in `long long` rather than `int` harms performance quite a bit in all non-I/O phases. I'm not sure if this is because less data fits in the cache or another reason.

> 2. Choose one suggestion you rejected, or one that turned out to be wrong. What evidence decided it?

I rejected the suggestion to not initial construct `smoothed` as a copy of `data`. I constructed `smoothed` in this way before since I thought it wouldn't matter for smaller files, but some of the generated test files in this assignment have a million samples, so I thought this might actually be an optimization worth doing. I changed it so that it first constructed `smoothed` from `data[0..3]`, reserved `data.size()` elements, then after the smoothing appended `data[data.size()-3..data.size()]`. I was surprised to find that even with a million samples, this approach was significantly slower than my previous lazy method (smoothing phase went up to 49ms from 18ms). Weird.

This actually has less of a performance impact now that arithmetic is done in `long long`, but it still makes the smoothing phase 5–9ms slower, so I maintian my rejection.

> 3. Did `gen_data.cpp` or `stage_timer.h` contain a mistake? How did you find it, or how did you check that there was none?

I did not see any logic mistakes in either generated source file, only some questionable design decisions, such as assuming my program was multithreaded and acquiring a mutex every time a timer is destroyed.

> 4. Which suggestion taught you something about C++ that you did not know before?

I learned how poorly streams and iterators perform compared to classic C-style I/O. I assumed there was a little overhead, but I had no idea it was this bad, lol.
