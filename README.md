<h1 align="center">Composite Memory Allocator</h1>

### About


A custom memory allocator based on **Fixed Size Allocator (FSA)** and **Coalesce Allocator**. The **Composite Memory Allocator** is a wrapper over these two allocators.

Basic allocation scheme, depending on the requested memory size:

 - [1B - 512B] — FSA is used. The Composite Memory Allocator contains several FSAs with different block sizes: *16, 32, 64, 128, 256, 512*. The allocator with the minimum suitable size is selected.
 - [512B - 10 MB] — Coalesce Allocator is used.
 - [10M - ...] — VirtualAlloc is used directly.

### Debug mode

Coalesce allocator in debug mode uses special markers inside service blocks and can track memory corruptions.

All allocators in debug mode can track memory leaks, double free, attempt to use uninitialized allocator.

Also Composite Memory Allocator in debug mode can output statistics on allocators and dump of occupied memory.

### Usage example

```cpp
CompositeMemoryAllocator allocator;  
allocator.init();  
void* p = allocator.alloc(1024);
// ...
allocator.free(p);    
allocator.destroy();
```
