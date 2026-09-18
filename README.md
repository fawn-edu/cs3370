# Programming assignment 2: shared memory pool

Fawn Sannar <10725695@uvu.edu> | `10725695`

The program can be compiled and ran by invoking `make`.

The hardest part of this assignment was debugging. Early on I had written the following logic in `Pool::alllocate()`, erroneously thinking that `vector::back()` returned the index of the final item for some reason:

```c++
const auto i = freelist[freelist.back()];
freelist.pop_back();
```

This is horribly incorrect and caused me about an hour of headache and SIGSEGV, since it compiled fine and just used the number I actually wanted as an index into the freelist, giving me an incorrect result. As soon as I realized the issue and changed it to the following, all tests passed:

```c++
const auto i = freelist.back();
freelist.pop_back();
```

Why does `vector::pop_back()` not return the popped item? C++ is bizzare.
