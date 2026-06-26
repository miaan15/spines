# Spines
Custom config language parser

# Usage
It's just spines.c and spines.h, you're welcome.
```c
SpinesContext cxt = SpinesContext_make(); // this basically just does zero-init to SpinesContext,
                                          // so you won't need SpinesContext_make() if it guarantee to zero-init
const char *str = "..."; // config file content
spines_parse(&cxt, str, strlen(str)); // parse str

SpinesContext_destroy(&cxt); // remember to free memory because this is C
```

## Config Example
```
// Comments are allowed

// Make a group:
Base {
    36, 3.141592, 1.6e9, 0x1A6F, // support multiple kind of number
    "Lorem ipsum dolor sit amet", "consectetur adipiscing elit" // support string
    Sub { // nested group
        3, 6
        Value = "funny nummbers" // direct assign 
    }
}

Frame {
    // auto indexed group
    *{0, 0, 1} // 0
    *{0, 2, 3} // 1
    *{0, 4, 5} // 2
    *{0, 6, 7} // 3
}
```

# Run
```bash
# compile C version - the true version
make a
./a

# compile C++ version - the benchmark version (implementation exactly like C)
make b
./b

# add BUILD=release/BUILD=debug to control build type
# make a BUILD=release

# clean
make clean
```

# Output
Parse speed:
```
1KB:   0.046829 ms
1MB:   11.6174 ms
10MB:  114.519 ms
100MB: 1352.39 ms
```
