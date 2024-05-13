# User Space Threads
This repository contains an implementation of userspace threads in xv6.
It was duplicated from the CSSE332 repository from Rose-Hulman.

## [Design Document](https://docs.google.com/document/d/1_vCjCfHd6NBrlohUr3pkKFhA88-8aVX6THASi1B85hU/edit?usp=sharing) 

## [Demo Video](https://drive.google.com/file/d/1_l6IKFp8xGTzEj0hU61lnjrJV7hi68ci/view?usp=sharing)

## Initialization Changes
<code>xv6-riscv/user/xv6test.c</code>: Created new test source code file to test our new system calls or features

To use or test this, run <code>make qemu</code>. Then, type <code>ls</code> and make sure <code>xv6test</code> is there. Then, run <code>xv6test</code> to run your unit tests and verify the code is working.

## How to Implement Multiple Threads Allocating Pages
The <code>struct proc</code> now has 3 extra members: <code>subthreads</code>, <code>is_mainthread</code>, and <code>active_subthreads</code>

When all processes are getting initialized, it was already the case that their <code>state</code> is set to <code>UNUSED</code>. Now, we also set <code>is_mainthread=1</code> and <code>active_subthreads=0</code>.

In <code>clone()</code>, the parent process's <code>active_subthreads</code> is incremented, a pointer to the subthread is added to the appropriate index of the parent process's <code>subthreads</code> array, and the subthreads's <code>is_mainthread</code> is set to <code>0</code>.

In <code>join()</code>, the parent process's <code>active_subthreads</code> is decremented, the pointer to the subthread replaced with <code>NULL</code> at the appropriate index of the parent process's <code>subthreads</code> array, the subthread's <code>is_mainthread</code> is set back to <code>1</code>, and the subthread's <code>active_subthreads</code> is reset to <code>0</code> just in case.

The next step is to modify <code>uvmalloc_thread()</code> and <code>uvdealloc_thread()</code> to take in an extra parameter: the process which is calling it. Then, those functions can map the same physical memory allocated with <code>kalloc()</code> (or deallocated with <code>uvmunmap</code>) for each of the calling thread's children (if <code>is_mainthread=1</code>) or the calling thread's parent and siblings (if <code>is_mainthread=0</code>).

There is now a duplicate function called <code>growthread()</code> (as opposed to <code>growproc()</code>) which will call the new functions <code>uvmalloc_thread()</code> and <code>uvdealloc_thread()</code>. We may need to perform an extra check in line <code>46</code> in <code>sysproc.c</code> to call <code>growthread()</code> instead of <code>growproc()</code> in the correct circumstances.
