# Programming assignment 1: circular buffer

Fawn Sannar <10725695@uvu.edu> | `10725695`

The program can be compiled and ran by invoking `make`.

So this is a really funny assignment for me because I happen to have spent all summer break working on a [ringbuffer library](https://codeberg.org/rubiefawn/chic) intended for IPC to/from audio worker threads. This assignment is a little different though as `CircBuf` has no realtime performance constraints, is not required to be concurrency safe, but it *is* required to allocate more space to fit items when full. Properly growing the buffer in `CHUNK`s was the hardest part of this assignment.

The rubric mentions "follow the spec for `front_` and `back_`", but there is no spec for `front_` and `back_`. This seems like leftovers from a previous version of the rubric presumably in which the data members representing the head and tail indicies was included in the provided header. Might be good to remove that.
