<h1 align="center">Composite Memory Allocator</h1>

### About

**Composite Memory Allocator** is a custom memory allocator that combines:

- **Fixed Size Allocator (FSA)** — for small fixed-size blocks  
- **Coalesce Allocator** — for larger blocks  

Memory allocation depends on size:

| Size range | Allocator |
|------------|-----------|
| 1B – 512B  | FSA (blocks: 16, 32, 64, 128, 256, 512) |
| 512B – 10MB | Coalesce Allocator |
| 10MB+      | Direct VirtualAlloc |

---

### Key Optimizations

1. **Lazy Free List Initialization (FSA)**  
   - Free list is built on demand. Blocks are added as they are used, not all at once.  

2. **Segregated Free Lists (Coalesce Allocator)**  
   - Free blocks are split into bins by size. Allocation searches only the appropriate bin.  

3. **Boundary Tags / Block Footer (Coalesce Allocator)**  
   - Each block stores its size at start and end. Allows fast merging with neighbors on free.

 …and other

---

### Debug Mode

In debug mode, the allocator tracks:

- Memory corruptions  
- Memory leaks  
- Double free attempts  
- Using an uninitialized allocator  

Additionally, it can:

- Output **allocator statistics**  
- Dump **occupied memory** 

---

### Usage Example

```cpp
CompositeMemoryAllocator allocator;  
allocator.init();  

void* p = allocator.alloc(1024);
// Work with memory
allocator.free(p);    

allocator.destroy();
