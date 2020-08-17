
# Experiments with LUT'ed binary search in C++

Instead of starting binary search with the range \[0, N), a lookup-table
is used to provide a smaller, more precise initial range.

See the first [reference](#references) for a full explanation.

The purpose of this repo is to explore issues around building and using the LUT
on various types of input, it is not to build a production-ready binary-search _per se_.

## Issues

Obviously this approach only makes sense for repeated searches over the same data,
but that is a pretty common scenario.

Unsigned keys work very well, especially if they're uniformly distributed.

The `extra-shift` approach, which AFAIK is novel by me, but was probably explored
by Tarjan et.al in the 1980s, neatly handles the problem of small-value keys in a larger key-type,
which would otherwise distribute poorly over the LUT buckets because all the top-bits
would be zeros.

Signed keys work, but distributes poorly for (small) standard distributions because
`extra-shift` can't handle them. I have an idea on how to solve this, by calculating
the `extra-shift` on |key|, but this needs to be explored.

Floating-point keys are so far unexplored.

## Related work

This connects to my work on [radix sorting](https://github.com/eloj/radix-sorting) in that sorting is a requirement,
of course, and it also shares the concept of key-derivation to impose a total order of the input keys in 'unsigned space',
which is what enables the bit-tricks used to parition the keys.

## References

* David Geier "[Optimizing binary search](https://geidav.wordpress.com/2013/12/29/optimizing-binary-search/)", 2013.
* Relates to "[Interpolation Search](https://en.wikipedia.org/wiki/Interpolation_search)".
