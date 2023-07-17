# How to run

To run this project, install rust (doing so through [rustup] is the most straightforward)

Then, in a terminal or command window, navigate to the `opencubes/rust` directory and run:

```shell
cargo run --release -- run <n>
```

where n is the count of cubes for which to calculate the amount of unique polycubes.

For more info, run:

```shell
cargo run --release -- --help
```

[rustup]: https://rustup.rs/

# Rusty Cubes

## Performance
- Times are cumulative (includes time to calculate all subsets from n=3).

- Times are measured with a large sample set of 1 run in an environment with many background processes of varying resource intensiveness throughout and rounded to 3 sig figs

- No cache files used.

- Working with Low% speedrun rules - more cubes is always better no matter the time,
a faster time is better than a slower time if cube count is equal.

OoM - Out of Memory - unable to run due to memory limitations but we can dream

NA - didnt try, probably out of memory but had an estimated time in hours so didnt measure

Hardware:
Ryzen 9 7900X
32GB ram @ DDR5-6000

| Git hash | N = 6 | N = 7 | N = 8 | N = 9 | N = 10 | N = 11 | N = 12 | N = 13 | N = 14 | N = 15 | N = 16 | N = 17 | Mode |
| -------- | ----- | ----- | ----- | ----- | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ---- |
| python | 113ms | 713ms | 5.0s | 37.4s | 239s | 2310s | NA | NA | NA | NA | NA | NA | NA |
| 50b6682 | 0.17ms | 1.37ms | 10.3ms | 73.6ms | 0.643s | 5.86s | 55.5s | OoM | OoM | OoM | OoM | OoM | opti-bit |
| afa90ad | 0.184ms | 1.43ms | 11.4ms | 8.41ms | 0.686s | 6.58s | 62.45s | 574s | OoM | OoM | OoM | OoM | opti-bit |
| 68090de | 13.2ms | 20.4ms | 37.4ms | 85.3ms | 0.304s | 1.74s | 14.2s | 124s | OoM | OoM | OoM | OoM | point-list + rayon |
| b83f9c6 | 3ms | 4.3ms | 8.58ms | 25.4ms | 0.137s | 0.986s | 8.02s | 66.7s | OoM | OoM | OoM | OoM | point-list + rayon |

## To Do
- implement flag for target n

- maybe implement cache file saving and loading

- store a more compressed encoding for polycubes maybe an list of N coordinates
encoded as -ZZZZZYYYYYXXXXX in u16's. this would reduce the size of the poly cubes from
84 Bytes (with size16 enabled) to 32 Bytes for a maximum of 16 cubes. The encoding discribed
https://github.com/mikepound/opencubes/issues/8 would be more efficient for small N (N < 16)
but becomes less efficient if needing to store the maximum space a cube could take up

- might need a denser data stucture than a hashset with its extra space over head but
cant think what to replace it with

- profile and optimise

- run out of memory faster by multi threading polycube generation.
Some concerns about the lock on the set blocking but think it should generally help.
Have rayon but my workload is very unbalanced.

- run out of memory even faster with SIMD potential. not easy to parallelise but
could maybe bucket similar work buckets of polycubes with similar bounding boxes
that need a cube added in the same place

- deduplicate mirrors as well to reduce (optimistically) ~50% memory usage in the
hashset and then test for symatry when counting unique variants

## Foreseen Issues
as many have discussed memory capacity seems to be the big killer. assuming rough
exponential growth based on the growth from n=6 -> n=12 n=16 may require as much as 2tb of
ram to access. Either dividing the problem across many smaller cached files may work
or (easier but painfully slower) mmapping a large ~2tb "swap" file on hard disks could stand a chance of working


## About Code

Uses a similar stratergy as Mike describes in the video by generating all polycubes
of order N by adding a cube to every open face of all polycubes of order N-1.

poly cubes are stored in a struct containing 4 fields
- x, y, z store the maximum coordinate in each dimension. e.g. a 3x3x3 cube has X=Y=Z=2 because in `0, 1, 2` 2 is the highest index
- cube_map is a packed 3d array of bits storing the cubes that make up the polycube.
cubes in the x axis are stored as the bits in the elements, a cubes y and z are determined by its index in the array encoded as `[z * height + y]` where height is `self.y + 1`

### current optimisations
- create a "canonical" orientation of each polycube by enforcing it fitting in a bounding
box where X <= Y <= Z and then taking the minimum value of the binary encoding of the
rotations of the cube data.
this greatly reduces the number of rotations needing to be processed and removes the need for
rotation code the change the shape of the buffer.


### Compile time features

#### size16
Store polycubes as arrays of u16's rather than u32's this halves
the memory usage at the cost of limiting the code to N <= 16
which most modern memory capacities do already

#### diagnostics
enables a bunch of extra checks to ensure various integrity propertys
of the poly cubes
