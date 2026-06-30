# Spines
Custom config language parser

## Spines Language Example
```
// Comments are allowed

// Make a group:
Base {
    36, 3.141592, 1.6e9, 0x1A6F, // support multiple kind of number
    "Lorem ipsum dolor sit amet", "consectetur adipiscing elit" // support string
    Sub { // nested group
        6.7, 3.6
        Value = "funny nummbers" // direct assign
    }
}

Frame {
    // auto indexed group
    // each *{ will be considered as "*<index>"
    *{ 0, 1, 9 } // 0
    *{ 0, 2, 8 } // 1
    *{ 1, 3, 7 } // 2
    *{ 1, 4, 6 } // 3
}
```

# Usage
It's just spines.c and spines.h, you're welcome.
```c
const char *str = "..."; // config file content above
spn_Context cxt = {0}; // remember to zero-init
spn_parse(&cxt, str, strlen(str)); // parse str

// =============================================================
// Get the root group
spn_Group global_gr = spn_root(&cxt);

// Find group from its child
spn_Group sub_gr = spn_find(global_gr, "Base/Sub");

// Get field data
float v0 = (float)spn_fields(&sub_gr)[0].float_val;
float v1 = (float)spn_fields(&sub_gr)[1].float_val;

printf("In Base/Sub: [0] = %.2f; [1] = %.2f\n", v0, v1);
// In Base/Sub: [0] = 6.70; [1] = 3.60

// =============================================================
// directly mutate the group by move it to its child
spn_Group v_gr = spn_root(&cxt);
spn_move(&v_gr, "Base/Sub/Value");

// Get the string value
const char *vs = spn_str(&cxt, spn_fields(&v_gr)[0]);

printf("In Base/Sub/Value: %s\n", vs);
// In Base/Sub/Value: funny nummbers

// =============================================================
// Auto indexed group: *<index>
spn_Group frame_2_gr = spn_find(global_gr, "Frame/*2");
int v3 = (int)spn_fields(&frame_2_gr)[1].int_val;

// Can also use number with indexed group
spn_Group frame_3_gr = spn_find_id(spn_find(global_gr, "Frame"), 3);
int v4 = (int)spn_fields(&frame_3_gr)[2].int_val;

printf("In Frame: *2[1] = %d; *3[2] = %d\n", v3, v4);
// In Frame: *2[1] = 3; *3[2] = 6

// =============================================================
// Get the group right after, which means its first child
spn_Group frame_iter_gr = spn_next_flat(spn_find(global_gr, "Frame"));
// Note: this is not the same as spn_next(),
//       which will move to the next group but still the same level

// =============================================================
// Iterate through groups
printf("Iterate Frame: ");
do {
    int x = spn_fields(&frame_iter_gr)[1].int_val;
    printf("%d ", x);
} while (spn_step(&frame_iter_gr));
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
