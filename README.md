# Hash table

This is a simple hash table implemented in C using no external libraries.

It is by no means meant to be used in an actual application; instead, my goal was to experiment and get an understanding of how hash tables work under the hood.

## Compiling

To compile, run `./build.sh`, which will generate an executable `hashtable`.

## Implementation

This implementation uses [open adressing](https://en.wikipedia.org/wiki/Open_addressing) in order to handle collisions.

Before an insert operation and after a delete operation is performed, the hash table is resized, such that the load factor always stays between 0.25 and 0.5.

When resizing, the new capacity is always choosen to be the next greatest prime number after the requested capacity, which helps to avoid collisions.

The probing function that was chosen is a simple linear one, with an interval of 1.
That is, each cell of the underlying array is probed in order, starting at the index determined by the hash function.
I did experiment with other probing functions, such as quadratic ones, but saw no significant reduction in performance.
It must be mentioned, however, that this experimentation was not performerd with much rigour and is mainly based on my subjective perception.
