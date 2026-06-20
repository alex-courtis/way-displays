# Alex C Library

## Collections

Not thread safe.

### SList

* Containerless singly linked list.
* NULL values permitted.

### PSet

* Array backed pointer set.
* Entries preserve insertion order.
* Operations linearly traverse values.
* NULL not permitted.

### SSet

* `PSet` with string values
* Values are memory managed.

### PMap

* Array backed pointer indexed map.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.

### IMap

* `PMap` with `size_t` keys

### SMap

* `PMap` with string keys.
* Keys are memory managed.

### SMapS

* `PMap` with string keys and vals.
* Keys and values are memory managed.

## Strings

libc string helpers 
