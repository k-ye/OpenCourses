// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR)) {
		panic("pgfault(): not a write, utrapframe eip: %08x, va: %08x.\n", utf->utf_eip, (uint32_t)addr);
	}
	// This is super important!
	addr = (void *)(ROUNDDOWN(addr, PGSIZE));
	// virtual page number
	uint32_t addr_pn = ((uint32_t)(addr) >> PGSHIFT);
	// Check two things here:
	//	1. uvpd[PDX(addr)] is the PDE telling if entry @ _PDX(addr)_ stores 
	//		the physical address of the page table page containing `addr`
	//	2. uvpt[addr_pn] is the PTE of the physical page containing addr.
	if (((uvpd[PDX(addr)] & PTE_P) == 0) || ((uvpt[addr_pn] & PTE_COW) == 0))
		panic("pgfault(): not write to a copy-on-write page.\n");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	envid_t envid = 0; // sys_getenvid();
	// allocate the physical page
	r = sys_page_alloc(envid, PFTEMP, PTE_P | PTE_U | PTE_W);
	if (r) 
		panic("pgfault(): sys_page_alloc() failed, %e.\n", r);
	// copy what is stored in `addr` to PFTEMP
	memcpy((void *)PFTEMP, addr, PGSIZE);
	// remap `addr` to the physical page which PFTEMP maps to.
	// We don't need to unmap PFTEMP, b/c sys_page_alloc() automatically
	// handles it (in `page_insert` in kern/pmap.c).
	r = sys_page_map(envid, (void *)PFTEMP, envid, addr, PTE_P | PTE_U | PTE_W);
	if (r)
		panic("pgfault(): sys_page_map() failed, %e.\n", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t dst_envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// Some validness checking
	uintptr_t addr = pn * PGSIZE;
	assert(uvpd[PDX(addr)] & PTE_P);

	pte_t pte = uvpt[pn];
	assert(pte & PTE_P);

	int perm = PTE_P | PTE_U;

	bool src_share = (pte & PTE_SHARE);
	// Check if `addr` is writable or copy-on-write
	bool src_w_cow = (!src_share) && ((pte & PTE_W) || (pte & PTE_COW));
	// If it's true, make the permission COW (can never be PTE_W!).
	if (src_share) {
		perm |= (pte & PTE_SYSCALL);
	} else if (src_w_cow) {
		perm |= PTE_COW;
	}
	// perm = src_w_cow ? (perm | PTE_COW) : perm;
	// get current env_id
	envid_t envid = 0;// sys_getenvid();
	// make the `addr` in both our `envid` and `dst_envid` map to the same p_addr.
	r = sys_page_map(envid, (void *)addr, dst_envid, (void *)addr, perm);
	if (r)
		panic("duppage(): sys_page_map() failed, %e.\n", r);
	// remap our own with permission COW as suggested
	if (src_w_cow) {
		// groun truth checking
		assert((perm & PTE_P) && (perm & PTE_U) && (perm && PTE_COW));
		assert((perm & PTE_W) == 0);

		r = sys_page_map(envid, (void *)addr, envid, (void *)addr, perm);
		if (r)
			panic("duppage(): sys_page_map() failed when remapping self, %e.\n", r);
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// set our pgfault handler
	set_pgfault_handler(pgfault);

	envid_t envid;
	uint8_t *addr;
	int r;
	// Allocate a new child environment.
	envid = sys_exofork();

	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// We're the child. Fix 'thisenv'
		thisenv = &envs[ENVX(sys_getenvid())];
		// We cannot set up the pgfault handler in child's own env!
		// Why? 
		// At this moment, child's env does not have a user pgfault handler
		// at all. All the virtual addresses in child env are COW (but not
		// writable_, including its stack! Then whenever the stack is used,
		// the page fault exception will be generated. Kernel code finds out
		// it cannot handle this type of trap yet, and kills the child env.
		return 0;
	}
	// Since user exception stack should not be mapped COW, the loop
	// ends at USTACKTOP instead of UTOP.
	uint32_t pn;
	for (addr = (uint8_t*)UTEXT; addr < (uint8_t *)USTACKTOP; addr += PGSIZE) {
		pn = (uint32_t)(addr) >> PGSHIFT;
		// `addr` is not mapped, skip
		if (((uvpd[PDX(addr)] & PTE_P) == 0) || ((uvpt[pn] & PTE_P) == 0)) continue;

		duppage(envid, pn);
	}
	// allocate page for child env exception stack
	r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_P | PTE_W);
	if (r)
		panic("fork(): sys_page_alloc() failed for child exception stack.\n");
	// set up child's pgfault_upcall by using thisenv's pgfault_upcall
	r = sys_env_set_pgfault_upcall(envid, envs[ENVX(sys_getenvid())].env_pgfault_upcall);
	if (r)
		panic("fork(): failed to setup child's user pgfault_upcall.\n");
	// set the child environment status to be runnable
	r = sys_env_set_status(envid, ENV_RUNNABLE);
	if (r)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
