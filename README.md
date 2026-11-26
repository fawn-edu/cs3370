# Programming assignment 7: pipelined e-commerce order processing system

Fawn Sannar <10725695@uvu.edu> | `10725695`

## Reflection

I have a decent amount of experience with thread synchronization; see [Chic](https://codeberg.org/rubiefawn/chic), a lock-free queue library for C with a focus on realtime safety I wrote last summer. However, since that library and the context in which I found myself needing it has realtime requirements, synchronization mechanisms such as mutexes and condition variables are actually forbidden. This assignment was a nice opportunity to practice using those synchronization mechanisms that I would otherwise not be using in my main projects due to those realtime requirements.

The part I struggled the most with was getting the condition variables and the stop tokens to cooperate. No thread ever holds more than one mutex at a time, so deadlocks were by definition impossible, and yet the routing threads would never finish. After scouring the docs, I found `std::stop_callback` which saved the day. We might have already gone over `std::stop_token` during the lectures, but I'm a bit behind on those so I had to find it the hard way instead.
