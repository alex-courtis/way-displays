# Alex C Library

## Collections

Not thread safe.

### Pslist

* Containerless singly linked list.
* NULL values permitted.

### Pset

* Array backed pointer set.
* Entries preserve insertion order.
* Operations linearly traverse values.
* NULL not permitted.

### Sset

* `Pset` with string values
* Values are memory managed.

### PPmap

* Array backed pointer indexed map.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.

### IPmap

* `PPmap` with `size_t` keys

### SPmap

* `PPmap` with string keys.
* Keys are memory managed.

### SSmap

* `PPmap` with string keys and vals.
* Keys and values are memory managed.

### SImap

* `PPmap` with string keys `size_t` vals.
* Keys are memory managed.

## Strings

libc string helpers 

`*printf_alloc` allocates a string of the correct size to printf into.

`*printf_append` allocates a new string for the existing and new to printf into, frees existing.

## File System

`fs_mkdir_p` performs `mkdir -p`

`fs_file_write` writes a string to a new or existing file.

`fs_canonical_path` returns a new string from `realpath`, if it can be accessed.

## Functions

function typedefs for equals, less_than, predicates, freeing, cloning and to-string.

Some convenience implementations provided.

TODO: doxygen
