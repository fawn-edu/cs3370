# Programming assignment 3: `BigInt`

Fawn Sannar <10725695@uvu.edu> | `10725695`

The program can be compiled and ran by invoking `make`.

`BigInt` uses two's complement representation. Arbitrary precision is achieved by storing a variable-length sequence of `size_t`, meaning it can also be described as a base-`SIZE_MAX` integer where each digit is a `size_t`.
