# Alex C Library

## Collections

### SList

* Containerless singly linked list.
* Not thread safe.
* NULL values permitted.

### SSet

* Array backed ordered string set.
* Operations linearly traverse values.
* NULL not permitted.
* Not thread safe.

### PSet

* Array backed ordered pointer set.
* Operations linearly traverse values.
* NULL not permitted.
* Not thread safe.

### STable

* Array backed string indexed table.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.
* Not thread safe.

### ITable

* Array backed integer indexed table.
* Entries preserve insertion order.
* Operations linearly traverse keys.
* NULL values permitted.
* Not thread safe.

### PTable

* ITable convenience wrapper with pointer key.

## Strings

libc string helpers 
