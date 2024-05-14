# User Space Threads in xv6
This repository contains an implementation of userspace threads in xv6.
It was duplicated from the CSSE332 repository from Rose-Hulman.

## [Design Document](https://docs.google.com/document/d/1_vCjCfHd6NBrlohUr3pkKFhA88-8aVX6THASi1B85hU/edit?usp=sharing) for Extra Implementation/Usertest Detail

## [Demo Video](https://drive.google.com/file/d/1_l6IKFp8xGTzEj0hU61lnjrJV7hi68ci/view?usp=sharing)

## Documentation (for more details, see design document)

#### New Process Structure

The following three fields were added to <code>struct proc</code> to handle this project:

<code>int is_mainthread</code>: Equals <code>1</code> for regular processes or parent threads, and equals <code>0</code> for subthreads.

<code>int active_subthreads</code>: Equals <code>0</code> for regular processes or subthreads, and equals <code>1/2/3/4</code> for parent threads.

<code>struct proc *subthreads[MAX_THREADS]</code>: Array for holding subthreads if they are created (<code>MAXTHREADS=4</code>).

#### Kernel Functions in <code>kernel/proc.c</code>

<code>static struct proc* allocthread(int* pid)</code>

This function is based closely on the existing <code>allocproc()</code> function in <code>proc.c</code>. However, it does take in a parameter: an integer pointer for where the new PID will be stored. This is going to be used in the <code>tcreate()</code> system call to provide the user with the correct PID. This way, the user will know which PID to use when invoking <code>tjoin()</code> to terminate a subthread. <code>allocthread()</code> first searches the process table for an <code>UNUSED</code> process. If found, a new PID is allocated, and the <code>state</code> is set to <code>USED</code>. Then, <code>copyout()</code> is called to copy the new PID from kernel space to the user space using the integer pointer parameter. Next, we call <code>kalloc()</code> to allocate a new trapframe page. It creates an empty page table (except for <code>TRAMPOLINE</code> and <code>TRAPFRAME</code> pages) which will subsequently be populated with a copy of the parent's page table by the <code>tcreate()</code> system call. Finally, it sets up a context to execute at <code>forkret</code> which returns to user space. 

<code>static void freethread(struct proc *p)</code>

This function is based closely on the existing <code>freeproc()</code> function. It calls <code>kfree()</code> on the trapframe page. It does NOT call <code>proc_freepagetable</code> because that will free all physical memory corresponding to the page table entries. We want that physical memory to persist for the subthread's parent and siblings to continue using. We only call <code>uvmunmap</code> for the <code>TRAMPOLINE</code> and <code>TRAPFRAME</code> pages.

<code>int uvmcopy_thread(pagetable_t old, pagetable_t new, uint64 sz)</code>

This function is based closely on the existing <code>uvmcopy()</code> function. The ONLY difference is that it does not include the 3 lines of code which <code>kalloc()</code> new physical memory for the new page table to point to. The idea of this function is to create brand new page table mappings, but to EXISTING physical memory. No physical memory should be created at all.

<code>uint64 uvmalloc_thread(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm)</code>

This function is based closely on the existing <code>uvmalloc()</code> function. The main difference is that the physical memory which is <code>kalloc()'ed</code> is subsequently mapped to not only the calling thread's page table, but also all of its parent's and siblings' page tables (if the caller is a child subthreads) or its children subthreads' page tables (if the caller is a parent thread). This ensures memory consistency across related threads even when one thread allocates and maps a new page. This function also includes a <code>printf()</code> statement to reflect the new <code>sz</code> of every thread after this update is made.

<code>uint64 uvmdealloc_thread(pagetable_t pagetable, uint64 oldsz, uint64 newsz)</code>

This function is based closely on the existing <code>uvmdealloc()</code> function. The main difference is that the unmapping of the appropriate page happens in not only calling thread's page table, but also all of its parent's and siblings' page tables (if the caller is a child subthreads) or its children subthreads' page tables (if the caller is a parent thread). This ensures memory consistency across related threads even when one thread allocates and maps a new page. This function also includes a <code>printf()</code> statement to reflect the new <code>sz</code> of every thread after this update is made.

<code>int growproc(int n)</code>

This function was simply modified to call <code>uvmalloc_thread()</code> instead of <code>uvmalloc()</code> (or <code>uvmdealloc_thread()</code> instead of <code>uvmdealloc()</code>) if the calling process is a parent thread with children or is a child subthread.

#### Wrapper Functions in <code>user/xv6test.c</code>

<code>int thread_create(void* (\*fnc)(void*), int* pid, void* arg)</code>

Creates a new thread, placing the new pid into <code>pid</code>. This wrapper function <code>malloc()'s</code> a page for the subthread's stack and makes a system call to <code>tcreate()</code>.

<code>int thread_join(int pid)</code>

Cleans up a thread specified by <code>pid</code>. This wrapper function obtains the address of the subthread's stack page with the system call to <code>tjoin()</code>. It then <code>free()'s</code> the proper memory.

#### System Calls

<code>int tcreate(void* (\*fcn)(void*), int *pid, void *arg, void *stack)</code>

This system call creates a thread and is closely based on <code>fork()</code>. It first calls <code>allocthread()</code>, described above. It then calls <code>uvmcopy_thread</code> (described above) to use copy of the parent's page table as the subthread's page table. After updating some fields, it does a check to see if too many threads are created. Our API limits the number of subthreads to <code>MAX_THREADS=4</code>. It then sets the <code>a0</code> to the actual <code>void *</code> argument passed to the subthread by the user. The stack pointer <code>sp</code> is set to the address that was <code>malloc()'ed</code> by the <code>thread_create</code> wrapper function described above. The program counter register <code>epc</code> is set to the function pointer passed by the user so that the thread starts executing the correct code that that user wrote. A very careful tracking of the three extra fields of <code>struct proc</code> is done. The parent subthread's <code>active_subthreads</code> is incremented, the parent thread's <code>is_mainthread</code> is set to <code>1</code>, and the child subthread's <code>is_mainthread</code> and <code>active_subthreads</code> are set to <code>0</code>.

<code>int tjoin(int pid, uint64 addr)</code>

This system call destroys a child subthread and is closely based on <code>wait()</code>. The key difference is that <code>tjoin()</code> obtains the stack pointer <code>sp</code> of the child subthread and uses <code>copyout()</code> to place that into the <code>addr</code> parameter. This is important for the user to know so that the page allocated for the stack can be appropriately <code>free()'ed</code>. Additionally, the child subthread is removed from the parent thread's <code>subthreads</code> array, and any entries which come after the deleted subthread are shifted to the left so that no "holes" exist in the middle of the array. Finally, the <code>active_subthreads</code> and <code>is_mainthread</code> fields are appropriately updated for the parent and child.

<code>void tlock(void)</code>

Grabs a lock. Typically for <code>printf</code> statements from user code on multiple CPUs.

<code>void tunlock(void)</code>

Releases a lock. Typically for <code>printf</code> statements from user code on multiple CPUs.

# All planning notes used by the authors are below.

## Initialization Changes
<code>xv6-riscv/user/xv6test.c</code>: Created new test source code file to test our new system calls or features

<code>xv6-riscv/Makefile</code>: Added <code>$U/_xv6test</code> at line 136 and changed <code>CPUS := 3</code> to <code>CPUS := 1</code> in line 158.

To use or test this, run <code>make qemu</code>. Then, type <code>ls</code> and make sure <code>xv6test</code> is there. Then, run <code>xv6test</code> to run your unit tests and verify the code is working.

## How to Implement #5
The <code>struct proc</code> now has 3 extra members: <code>subthreads</code>, <code>is_mainthread</code>, and <code>active_subthreads</code>

When all processes are getting initialized, it was already the case that their <code>state</code> is set to <code>UNUSED</code>. Now, we also set <code>is_mainthread=1</code> and <code>active_subthreads=0</code>.

In <code>clone()</code>, the parent process's <code>active_subthreads</code> is incremented, a pointer to the subthread is added to the appropriate index of the parent process's <code>subthreads</code> array, and the subthreads's <code>is_mainthread</code> is set to <code>0</code>.

In <code>join()</code>, the parent process's <code>active_subthreads</code> is decremented, the pointer to the subthread replaced with <code>NULL</code> at the appropriate index of the parent process's <code>subthreads</code> array, the subthread's <code>is_mainthread</code> is set back to <code>1</code>, and the subthread's <code>active_subthreads</code> is reset to <code>0</code> just in case.

The next step is to modify <code>uvmalloc_thread()</code> and <code>uvdealloc_thread()</code> to take in an extra parameter: the process which is calling it. Then, those functions can map the same physical memory allocated with <code>kalloc()</code> (or deallocated with <code>uvmunmap</code>) for each of the calling thread's children (if <code>is_mainthread=1</code>) or the calling thread's parent and siblings (if <code>is_mainthread=0</code>).

There is now a duplicate function called <code>growthread()</code> (as opposed to <code>growproc()</code>) which will call the new functions <code>uvmalloc_thread()</code> and <code>uvdealloc_thread()</code>. We may need to perform an extra check in line <code>46</code> in <code>sysproc.c</code> to call <code>growthread()</code> instead of <code>growproc()</code> in the correct circumstances.
