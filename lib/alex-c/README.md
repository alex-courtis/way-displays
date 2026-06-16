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
* Values are strdup'd on successful `sset_add`, `sset_clone` and `sset_slist`
* Values are free'd on `sset_free`

### PTable

* Array backed pointer indexed table.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.

### ITable

* `PTable` with `size_t` keys

### STable

* `PTable` with string keys.
* Keys are strdup'd on successful `stable_put` and `stable_clone`
* Keys are free'd on `stable_free`

## Strings

libc string helpers 
