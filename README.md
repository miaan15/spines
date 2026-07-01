# Spines
Custom config language parser

## Spines Language Example
```
// Comments are allowed

// Make a "mark":
Base {
    36, 3.141592, 1.6e9, 0x1A6F, // support multiple kind of number
    "Lorem ipsum dolor sit amet", "consectetur adipiscing elit" // support string
    Sub { // nested mark
        6.7, 3.6
        Value: "funny", "nummbers" // mark can be like this
    }
}

Frame {
    // auto indexed mark
    // each "*" will be considered as a mark named: "*<index>"
    // also, you don't really need the ":" or ","
    * 0 1 9 // 0
    * 0 2 8 // 1
    * 1 3 7 // 2
    * 1 4 6 // 3
}
```

# Usage
It's just spines.c and spines.h, you're welcome.
```c
const char *str = "..."; // config file content above
spn_Context cxt = {0}; // remember to zero-init
spn_parse(&cxt, str, strlen(str)); // parse str

// Get the root mark
spn_Mark global_mk = spn_root(&cxt);

// Find mark from root mark's child
spn_Mark sub_mk = spn_find(global_mk, "Base/Sub");

// Get field data
float v0 = (float)spn_fields(&sub_mk)[0].float_val;
float v1 = (float)spn_fields(&sub_mk)[1].float_val;

printf("In Base/Sub: [0] = %.2f; [1] = %.2f\n", v0, v1);
// =============================================================
// In Base/Sub: [0] = 6.70; [1] = 3.60

// Directly mutate the mark by move it to its child
spn_Mark v_mk = spn_root(&cxt);
spn_move(&v_mk, "Base/Sub/Value");

// Get the string value
const char *vs = spn_str(&cxt, spn_fields(&v_mk)[0]);

printf("In Base/Sub/Value: %s\n", vs);
// =============================================================
// In Base/Sub/Value: funny

// Auto indexed mark: *<index>
spn_Mark frame_2_mk = spn_find(global_mk, "Frame/*2");
int v3 = (int)spn_fields(&frame_2_mk)[1].int_val;

// Can also use number with indexed mark
spn_Mark frame_3_mk = spn_find_id(spn_find(global_mk, "Frame"), 3);
int v4 = (int)spn_fields(&frame_3_mk)[2].int_val;

printf("In Frame: *2[1] = %d; *3[2] = %d\n", v3, v4);
// =============================================================
// In Frame: *2[1] = 3; *3[2] = 6

// Get the mark right after, which means its first child
spn_Mark frame_iter_mk = spn_next_flat(spn_find(global_mk, "Frame"));
// Note: this is not the same as spn_next(),
//       spn_next() will move to the next 'sibling' mark (the mark that is the same level)

// Iterate through marks
printf("Iterate Frame: ");
do {
    int x = spn_fields(&frame_iter_mk)[1].int_val;
    printf("%d ", x);
} while (spn_step(&frame_iter_mk));
// =============================================================
// Iterate Frame: 1 2 3 4 

spn_destroy(&cxt); // remember to free memory because this is C
```

# Run
```bash
make BUILD=release
./a

make clean
```

# Speed
Release build - Intel(R) Core(TM) i5-10400 CPU @ 2.90GHz:
```
1KB:   0.076659 ms
1MB:   28.458 ms
10MB:  239.744 ms
100MB: 567.836 ms
```
