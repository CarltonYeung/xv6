# COW fork()
This is a summary of the changes I made to the original Lab 2 commit, my thought process, and the results.

## Changes
git diff be616a1 f1481c6

### Makefile
* Add `cowforktest` to xv6 user programs.

### cowforktest.c
* Test that a child process doesn't write to the same physical page as its parent.
* Test that a cow forked child process' cow forked child cannot write to its  
parent's physical page.
* Test that the kernel can write to `PTE_COW` pages in system calls.

### defs.h
Add function declarations for:
* reference count helper functions
* `cowuvm`
* `cow_handler`

### kalloc.c
#### `struct run`
* Add refcount

#### `kfree`
* This function should only be called when refcount == 1, but it is also called  
to initialize the allocator when refcount == 0, so both of these cases are  
considered before actually freeing a page.
* memset should only fill a page with garbage if we are actually freeing the page.
* When freeing a page, the refcount is reset to 0. If the page is being referenced  
by at least one other process, the page is not freed and we just decrement the refcount.

#### `kalloc`
* When allocating a physical page, the refcount is initialized to 1.

#### `inc_refcount`, `dec_refcount`, `get_refcount`
* Check that the pointer passed to these functions resides in kernel virtual  
address space.
* I use the atomic built-in functions instead of locks in `struct run` for simplicity.
* refcount should never be decremented below 0.

### mmu.h
* define a `PTE_COW` flag to designate a copy-on-write page.

### proc.c
Naturally I started the cow fork lab by reading `fork` and tracing the functions that  
it called. When I thought I understood enough of the default implementation to  
begin thinking about how to implement cow fork, I thought that not a whole lot  
had to be changed from the default implementation.

* Call `cowuvm` instead of `copyuvm`

### trap.c
My first implementation followed the lab instructions (i.e. use a variation of the  
default error messages and test to see that the program control flow would end up  
in my trap handler). I used my user test from Lab 1, `wolfietest`, instead of writing  
a test case because my user test would page fault anyway. The control flow did  
end up in my trap handler and I later fixed `wolfietest` by calling `exit()` instead  
of `return 0` in main.

#### `traps`
* Follow the format of the `syscall` case. If the trap is a `T_PGFLT`, then call  
`cow_handler`.

### vm.c
Most of the changes I planned to make were to `copyuvm`, so it made sense for  
`cowuvm` and `cow_handler` to be in this file. The idea was to split the functionality  
of `copyuvm` into two functions that should be called at different times because  
`copyuvm` actually already did most of the work for cow forking at the wrong time.

#### `cowuvm`
* As with `copyuvm`, call `setupkvm` to get the page directory with the kernel  
mappings.
* The child's user address space mappings should be the same as the parent's at  
this point in the forking process. We just need to change some flags so that the  
other half of `copyuvm` can be implemented in `cow_handler`.

#### `cow_handler`
The format of this function mostly follows the instructions given in Exercise 5.  
I think the only thing I had to search for was how to get the faulting address.  
I found the Wikipedia page on x86 [control registers](https://en.wikipedia.org/wiki/Control_register#CR2) to be helpful. I also saw that  
`rcr2` was called in the default trap handler in `trap`, so I figured the cr2 register  
contained the virtual address that I was looking for.
* Page faults caused by kernel writes are handled the same way as user writes.  
Most of my head-scratching time was spent here because I wasn't handling kernel  
page faults.
* The allocating and copying of physical pages from `copyuvm` is moved here.

## Results
Passed:
* `forktest`
* `usertests`
* `cowforktest`
* `wolfietest`

## Author
* **Carlton Yeung** - cayeung - 110681795
