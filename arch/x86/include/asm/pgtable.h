/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PGTABLE_H
#define _ASM_X86_PGTABLE_H

// #include "asm/pgtable_64_types.h"
#include <linux/mem_encrypt.h>
#include <asm/page.h>
#include <asm/pgtable_types.h>

#include <linux/pgtable_enhanced.h>

/*
 * Macro to mark a page protection value as UC-
 */
#define pgprot_noncached(prot)						\
	((boot_cpu_data.x86 > 3)					\
	 ? (__pgprot(pgprot_val(prot) |					\
		     cachemode2protval(_PAGE_CACHE_MODE_UC_MINUS)))	\
	 : (prot))

/*
 * Macros to add or remove encryption attribute
 */
#define pgprot_encrypted(prot)	__pgprot(__sme_set(pgprot_val(prot)))
#define pgprot_decrypted(prot)	__pgprot(__sme_clr(pgprot_val(prot)))

#ifndef __ASSEMBLY__
#include <asm/x86_init.h>
#include <asm/pkru.h>
#include <asm/fpu/api.h>
#include <asm-generic/pgtable_uffd.h>

extern pgd_t early_top_pgt[PTRS_PER_PGD];
bool __init __early_make_pgtable(unsigned long address, pmdval_t pmd);

void ptdump_walk_pgd_level(struct seq_file *m, struct mm_struct *mm);
void ptdump_walk_pgd_level_debugfs(struct seq_file *m, struct mm_struct *mm,
				   bool user);
void ptdump_walk_pgd_level_checkwx(void);
void ptdump_walk_user_pgd_level_checkwx(void);

#ifdef CONFIG_DEBUG_WX
#define debug_checkwx()		ptdump_walk_pgd_level_checkwx()
#define debug_checkwx_user()	ptdump_walk_user_pgd_level_checkwx()
#else
#define debug_checkwx()		do { } while (0)
#define debug_checkwx_user()	do { } while (0)
#endif

/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)]
	__visible;
#define ZERO_PAGE(vaddr) ((void)(vaddr),virt_to_page(empty_zero_page))

extern spinlock_t pgd_lock;
extern struct list_head pgd_list;

extern struct mm_struct *pgd_page_get_mm(struct page *page);

extern pmdval_t early_pmd_flags;

#ifdef CONFIG_PARAVIRT_XXL
#include <asm/paravirt.h>
#else  /* !CONFIG_PARAVIRT_XXL */


#define set_pte(ptep, pte)		native_set_pte(ptep, pte)

#define set_pte_atomic(ptep, pte)					\
	native_set_pte_atomic(ptep, pte)

#define set_pmd(pmdp, pmd)		native_set_pmd(pmdp, pmd)

#ifndef __PAGETABLE_P4D_FOLDED
#define set_pgd(pgdp, pgd)		native_set_pgd(pgdp, pgd)
#define pgd_clear(pgd)			(pgtable_l5_enabled() ? native_pgd_clear(pgd) : 0)
#endif

#ifndef set_p4d
# define set_p4d(p4dp, p4d)		native_set_p4d(p4dp, p4d)
#endif

#ifndef __PAGETABLE_PUD_FOLDED
#define p4d_clear(p4d)			native_p4d_clear(p4d)
#endif

#ifndef set_pud
# define set_pud(pudp, pud)		native_set_pud(pudp, pud)
#endif

#ifndef __PAGETABLE_PUD_FOLDED
#define pud_clear(pud)			native_pud_clear(pud)
#endif

#ifdef CONFIG_X86_64_ECPT
#define pte_clear(mm, addr, ptep) ecpt_native_ptep_get_and_clear(mm, addr, ptep)
#else
#define pte_clear(mm, addr, ptep)	native_pte_clear(mm, addr, ptep)
#endif
#define pmd_clear(pmd)			native_pmd_clear(pmd)

#define pgd_val(x)	native_pgd_val(x)
#define __pgd(x)	native_make_pgd(x)

#ifndef __PAGETABLE_P4D_FOLDED
#define p4d_val(x)	native_p4d_val(x)
#define __p4d(x)	native_make_p4d(x)
#endif

#ifndef __PAGETABLE_PUD_FOLDED
#define pud_val(x)	native_pud_val(x)
#define __pud(x)	native_make_pud(x)
#endif

#ifndef __PAGETABLE_PMD_FOLDED
#define pmd_val(x)	native_pmd_val(x)
#define __pmd(x)	native_make_pmd(x)
#endif

#define pte_val(x)	native_pte_val(x)
#define __pte(x)	native_make_pte(x)

#define arch_end_context_switch(prev)	do {} while(0)
#endif	/* CONFIG_PARAVIRT_XXL */

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE
#define pte_dirty(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_DIRTY)
static inline int native_pte_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_DIRTY;
}

#define pte_young(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_YOUNG)
static inline int native_pte_young(pte_t pte)
{
	return pte_flags(pte) & _PAGE_ACCESSED;
}

#define pmd_dirty(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_DIRTY)
static inline int native_pmd_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_DIRTY;
}

#define pmd_young(pte) \
	gen_pte_read(pte, LEVEL_PMD, PTE_ATTR_YOUNG)
static inline int native_pmd_young(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_ACCESSED;
}

#define pud_dirty(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_DIRTY) 
static inline int native_pud_dirty(pud_t pud)
{
	return pud_flags(pud) & _PAGE_DIRTY;
}

#define pud_young(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_YOUNG)
static inline int native_pud_young(pud_t pud)
{
	return pud_flags(pud) & _PAGE_ACCESSED;
}

#define pte_write(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_WRITE)
static inline int native_pte_write(pte_t pte)
{
	return pte_flags(pte) & _PAGE_RW;
}

#define pte_huge(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_HUGE)
static inline int native_pte_huge(pte_t pte)
{
	return pte_flags(pte) & _PAGE_PSE;
}

#define pte_global(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_GLOBAL)
static inline int native_pte_global(pte_t pte)
{
	return pte_flags(pte) & _PAGE_GLOBAL;
}

#define pte_exec(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_EXEC)
static inline int native_pte_exec(pte_t pte)
{
	return !(pte_flags(pte) & _PAGE_NX);
}

#define pte_special(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_SPECIAL)
static inline int native_pte_special(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SPECIAL;
}

#else
static inline int pte_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_DIRTY;
}

static inline int pte_young(pte_t pte)
{
	return pte_flags(pte) & _PAGE_ACCESSED;
}

static inline int pmd_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_DIRTY;
}

static inline int pmd_young(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_ACCESSED;
}

static inline int pud_dirty(pud_t pud)
{
	return pud_flags(pud) & _PAGE_DIRTY;
}

static inline int pud_young(pud_t pud)
{
	return pud_flags(pud) & _PAGE_ACCESSED;
}

static inline int pte_write(pte_t pte)
{
	return pte_flags(pte) & _PAGE_RW;
}
 
static inline int pte_huge(pte_t pte)
{
	return pte_flags(pte) & _PAGE_PSE;
}

static inline int pte_global(pte_t pte)
{
	return pte_flags(pte) & _PAGE_GLOBAL;
}

static inline int pte_exec(pte_t pte)
{
	return !(pte_flags(pte) & _PAGE_NX);
}

static inline int pte_special(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SPECIAL;
}

#endif

/* Entries that were set to PROT_NONE are inverted */

static inline u64 protnone_mask(u64 val);

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE
#define pte_pfn(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_PFN)
static inline unsigned long native_pte_pfn(pte_t pte)
{
	phys_addr_t pfn = pte_val(pte);
	pfn ^= protnone_mask(pfn);
	return (pfn & PTE_PFN_MASK) >> PAGE_SHIFT;
}

#define pmd_pfn(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_PFN)
static inline unsigned long native_pmd_pfn(pmd_t pmd)
{
	phys_addr_t pfn = pmd_val(pmd);
	pfn ^= protnone_mask(pfn);
	return (pfn & pmd_pfn_mask(pmd)) >> PAGE_SHIFT;
}

#define pud_pfn(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_PFN)
static inline unsigned long native_pud_pfn(pud_t pud)
{
	phys_addr_t pfn = pud_val(pud);
	pfn ^= protnone_mask(pfn);
	return (pfn & pud_pfn_mask(pud)) >> PAGE_SHIFT;
}

#define p4d_pfn(p4dval) \
	gen_pte_read(p4dval, LEVEL_P4D, PTE_ATTR_PFN)
static inline unsigned long native_p4d_pfn(p4d_t p4d)
{
	return (p4d_val(p4d) & p4d_pfn_mask(p4d)) >> PAGE_SHIFT;
}

#define pgd_pfn(pgdval) \
	gen_pte_read(pgdval, LEVEL_PGD, PTE_ATTR_PFN)
static inline unsigned long native_pgd_pfn(pgd_t pgd)
{
	return (pgd_val(pgd) & PTE_PFN_MASK) >> PAGE_SHIFT;
}

#define p4d_large(p4d) \
	gen_pte_read(p4d, LEVEL_P4D, PTE_ATTR_LARGE)
static inline int p4d_large(p4d_t p4d)
{
	/* No 512 GiB pages yet */
	return 0;
}

#define p4d_leaf(p4d) \
	gen_pte_read(p4d, LEVEL_P4D, PTE_ATTR_LEAF)
#define native_p4d_leaf	native_p4d_large

#define pmd_large(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_LARGE)
static inline int pmd_large(pmd_t pte)
{
	return pmd_flags(pte) & _PAGE_PSE;
}

#define pmd_leaf(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_LEAF)
#define native_pmd_leaf	native_pmd_large

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
/* NOTE: when predicate huge page, consider also pmd_devmap, or use pmd_large */

#define pmd_trans_huge(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_TRANS_HUGE)
static inline int native_pmd_trans_huge(pmd_t pmd)
{
	return (pmd_val(pmd) & (_PAGE_PSE|_PAGE_DEVMAP)) == _PAGE_PSE;
}

#ifdef CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD
#define pud_trans_huge(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_TRANS_HUGE)

static inline int native_pud_trans_huge(pud_t pud)
{
	return (pud_val(pud) & (_PAGE_PSE|_PAGE_DEVMAP)) == _PAGE_PSE;
}
#endif

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP
static inline int native_pmd_devmap(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_DEVMAP);
}

#ifdef CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD
static inline int native_pud_devmap(pud_t pud)
{
	return !!(pud_val(pud) & _PAGE_DEVMAP);
}
#else
static inline int native_pud_devmap(pud_t pud)
{
	return 0;
}
#endif

static inline int native_pgd_devmap(pgd_t pgd)
{
	return 0;
}
#endif

#define pmd_devmap(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_DEVMAP)
#define pud_devmap(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_DEVMAP)
#define pgd_devmap(pgd) \
	gen_pte_read(pgd, LEVEL_PGD, PTE_ATTR_DEVMAP)

#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

#else

static inline unsigned long pte_pfn(pte_t pte)
{
	phys_addr_t pfn = pte_val(pte);
	pfn ^= protnone_mask(pfn);
	return (pfn & PTE_PFN_MASK) >> PAGE_SHIFT;
}

static inline unsigned long pmd_pfn(pmd_t pmd)
{
	phys_addr_t pfn = pmd_val(pmd);
	pfn ^= protnone_mask(pfn);
	return (pfn & pmd_pfn_mask(pmd)) >> PAGE_SHIFT;
}

static inline unsigned long pud_pfn(pud_t pud)
{
	phys_addr_t pfn = pud_val(pud);
	pfn ^= protnone_mask(pfn);
	return (pfn & pud_pfn_mask(pud)) >> PAGE_SHIFT;
}

static inline unsigned long p4d_pfn(p4d_t p4d)
{
	return (p4d_val(p4d) & p4d_pfn_mask(p4d)) >> PAGE_SHIFT;
}

static inline unsigned long pgd_pfn(pgd_t pgd)
{
	return (pgd_val(pgd) & PTE_PFN_MASK) >> PAGE_SHIFT;
}

#define p4d_leaf	p4d_large
static inline int p4d_large(p4d_t p4d)
{
	/* No 512 GiB pages yet */
	return 0;
}

#define pmd_leaf	pmd_large
static inline int pmd_large(pmd_t pte)
{
	return pmd_flags(pte) & _PAGE_PSE;
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
/* NOTE: when predicate huge page, consider also pmd_devmap, or use pmd_large */
static inline int pmd_trans_huge(pmd_t pmd)
{
	return (pmd_val(pmd) & (_PAGE_PSE|_PAGE_DEVMAP)) == _PAGE_PSE;
}

#ifdef CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD
static inline int pud_trans_huge(pud_t pud)
{
	return (pud_val(pud) & (_PAGE_PSE|_PAGE_DEVMAP)) == _PAGE_PSE;
}
#endif

#define has_transparent_hugepage has_transparent_hugepage
static inline int has_transparent_hugepage(void)
{
	return boot_cpu_has(X86_FEATURE_PSE);
}

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP
static inline int pmd_devmap(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_DEVMAP);
}

#ifdef CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD
static inline int pud_devmap(pud_t pud)
{
	return !!(pud_val(pud) & _PAGE_DEVMAP);
}
#else
static inline int pud_devmap(pud_t pud)
{
	return 0;
}
#endif

static inline int pgd_devmap(pgd_t pgd)
{
	return 0;
}
#endif
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */


#endif	/* CONFIG_PGTABLE_OP_GENERALIZABLE */

#define pte_page(pte)	pfn_to_page(pte_pfn(pte))

#ifdef CONFIG_TRANSPARENT_HUGEPAGE

#define has_transparent_hugepage has_transparent_hugepage
static inline int has_transparent_hugepage(void)
{
	return boot_cpu_has(X86_FEATURE_PSE);
}

#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE
static inline pte_t pte_set_flags(pte_t pte, pteval_t set)
{
	pteval_t v = native_pte_val(pte);

	return native_make_pte(v | set);
}

static inline pte_t pte_clear_flags(pte_t pte, pteval_t clear)
{
	pteval_t v = native_pte_val(pte);

	return native_make_pte(v & ~clear);
}

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP

#define pte_uffd_wp(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_UFFD_WP)
static inline int native_pte_uffd_wp(pte_t pte)
{
	return pte_flags(pte) & _PAGE_UFFD_WP;
}

#define pte_mkuffd_wp(pteval) \
	gen_pte_update(pteval, LEVEL_PTE, ACTION_MK, PTE_ATTR_UFFD_WP)
static inline pte_t native_pte_mk_uffd_wp(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_UFFD_WP);
}

#define pte_clear_uffd_wp(pteval) \
	gen_pte_update(pteval, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_UFFD_WP)
static inline pte_t native_pte_clear_uffd_wp(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

#define pte_mkclean(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_DIRTY)

// static inline pte_t pte_mkclean(pte_t pte)
static inline pte_t native_pte_clear_dirty(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_DIRTY);
}

#define pte_mkold(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_YOUNG)
// static inline pte_t pte_mkold(pte_t pte)
static inline pte_t native_pte_clear_young(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_ACCESSED);
}

#define pte_wrprotect(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_WRITE)
// static inline pte_t pte_wrprotect(pte_t pte)
static inline pte_t native_pte_clear_write(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_RW);
}

#define pte_mkexec(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_EXEC)
static inline pte_t native_pte_mk_exec(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_NX);
}

#define pte_mkdirty(pte)                                                    \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_DIRTY)
static inline pte_t native_pte_mk_dirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

#define pte_mkyoung(pte)                                                    \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_YOUNG)

static inline pte_t native_pte_mk_young(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_ACCESSED);
}

#define pte_mkwrite(pte)                                                    \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_WRITE)
static inline pte_t native_pte_mk_write(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_RW);
}

#define pte_mkhuge(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_HUGE)
static inline pte_t native_pte_mk_huge(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_PSE);
}

#define pte_clrhuge(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_HUGE)
static inline pte_t native_pte_clear_huge(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_PSE);
}

#define pte_mkglobal(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_GLOBAL)
static inline pte_t native_pte_mk_global(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_GLOBAL);
}

#define pte_clrglobal(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_GLOBAL)
static inline pte_t native_pte_clear_global(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_GLOBAL);
}

#define pte_mkspecial(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_SPECIAL)
static inline pte_t native_pte_mk_special(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SPECIAL);
}

#define pte_mkdevmap(pte) 						       \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_DEVMAP)
static inline pte_t native_pte_mk_devmap(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SPECIAL|_PAGE_DEVMAP);
}

static inline pmd_t pmd_set_flags(pmd_t pmd, pmdval_t set)
{
	pmdval_t v = native_pmd_val(pmd);

	return native_make_pmd(v | set);
}

static inline pmd_t pmd_clear_flags(pmd_t pmd, pmdval_t clear)
{
	pmdval_t v = native_pmd_val(pmd);

	return native_make_pmd(v & ~clear);
}

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP
#define pmd_uffd_wp(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_UFFD_WP)
static inline int pmd_uffd_wp(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_UFFD_WP;
}

#define pmd_mkuffd_wp(pmdval) \
	gen_pte_update(pmdval, LEVEL_PMD, ACTION_MK, PTE_ATTR_UFFD_WP)
static inline pmd_t native_pmd_mk_uffd_wp(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_UFFD_WP);
}

#define pmd_clear_uffd_wp(pmdval) \
	gen_pte_update(pmdval, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_UFFD_WP)
static inline pmd_t native_pmd_clear_uffd_wp(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

#define pmd_mkold(pmd)                                                         \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_YOUNG)
static inline pmd_t native_pmd_clear_young(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_ACCESSED);
}
#define pmd_mkclean(pmd)                                                       \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_DIRTY)
static inline pmd_t native_pmd_clear_dirty(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_DIRTY);
}
#define pmd_wrprotect(pmd)                                                     \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_WRITE)
static inline pmd_t native_pmd_clear_write(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_RW);
}

#define pmd_mkdirty(pmd)                                                       \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_DIRTY)
static inline pmd_t native_pmd_mk_dirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

#define pmd_mkdevmap(pmd)                                                      \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_DEVMAP)
static inline pmd_t native_pmd_mk_devmap(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_DEVMAP);
}

#define pmd_mkhuge(pmd) gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_HUGE)
static inline pmd_t native_pmd_mk_huge(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_PSE);
}

#define pmd_mkyoung(pmd)                                                       \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_YOUNG)
static inline pmd_t native_pmd_mk_young(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_ACCESSED);
}

#define pmd_mkwrite(pmd)                                                       \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_WRITE)
static inline pmd_t native_pmd_mk_write(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_RW);
}

static inline pud_t pud_set_flags(pud_t pud, pudval_t set)
{
	pudval_t v = native_pud_val(pud);

	return native_make_pud(v | set);
}

static inline pud_t pud_clear_flags(pud_t pud, pudval_t clear)
{
	pudval_t v = native_pud_val(pud);

	return native_make_pud(v & ~clear);
}

#define pud_mkold(pud)                                                         \
	gen_pte_update(pud, LEVEL_PUD, ACTION_CLEAR, PTE_ATTR_YOUNG)
static inline pud_t native_pud_clear_young(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_ACCESSED);
}

#define pud_mkclean(pud)                                                       \
	gen_pte_update(pud, LEVEL_PUD, ACTION_CLEAR, PTE_ATTR_DIRTY)
static inline pud_t native_pud_clear_dirty(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_DIRTY);
}

#define pud_wrprotect(pud)                                                     \
	gen_pte_update(pud, LEVEL_PUD, ACTION_CLEAR, PTE_ATTR_WRITE)
static inline pud_t native_pud_clear_write(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_RW);
}

#define pud_mkdirty(pud)                                                       \
	gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_DIRTY)
static inline pud_t native_pud_mk_dirty(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

#define pud_mkdevmap(pud)                                                      \
	gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_DEVMAP)
static inline pud_t native_pud_mk_devmap(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_DEVMAP);
}

#define pud_mkhuge(pud) gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_HUGE)
static inline pud_t native_pud_mk_huge(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_PSE);
}

#define pud_mkyoung(pud)                                                       \
	gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_YOUNG)
static inline pud_t native_pud_mk_young(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_ACCESSED);
}

#define pud_mkwrite(pud)                                                       \
	gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_WRITE)
static inline pud_t native_pud_mk_write(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_RW);
}

#ifdef CONFIG_HAVE_ARCH_SOFT_DIRTY
#define pte_soft_dirty(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_SOFT_DIRTY)
static inline int pte_soft_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SOFT_DIRTY;
}

#define pmd_soft_dirty(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_SOFT_DIRTY)
static inline int pmd_soft_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SOFT_DIRTY;
}

#define pud_soft_dirty(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_SOFT_DIRTY)
static inline int pud_soft_dirty(pud_t pud)
{
	return pud_flags(pud) & _PAGE_SOFT_DIRTY;
}

#define pte_mksoft_dirty(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_SOFT_DIRTY)
static inline pte_t native_pte_mk_soft_dirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SOFT_DIRTY);
}

#define pmd_mksoft_dirty(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_SOFT_DIRTY)
static inline pmd_t native_pmd_mk_soft_dirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SOFT_DIRTY);
}

#define pud_mksoft_dirty(pud) \
	gen_pte_update(pud, LEVEL_PUD, ACTION_MK, PTE_ATTR_SOFT_DIRTY)
static inline pud_t native_pud_mk_soft_dirty(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_SOFT_DIRTY);
}

#define pte_clear_soft_dirty(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_SOFT_DIRTY)

static inline pte_t native_pte_clear_soft_dirty(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SOFT_DIRTY);
}

#define pmd_clear_soft_dirty(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_SOFT_DIRTY)
static inline pmd_t native_pmd_clear_soft_dirty(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SOFT_DIRTY);
}

#define pud_clear_soft_dirty(pud) \
	gen_pte_update(pud, LEVEL_PUD, ACTION_CLEAR, PTE_ATTR_SOFT_DIRTY)
static inline pud_t native_pud_clear_soft_dirty(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_SOFT_DIRTY);
}

#endif /* CONFIG_HAVE_ARCH_SOFT_DIRTY */

#else

static inline pte_t pte_set_flags(pte_t pte, pteval_t set)
{
	pteval_t v = native_pte_val(pte);

	return native_make_pte(v | set);
}

static inline pte_t pte_clear_flags(pte_t pte, pteval_t clear)
{
	pteval_t v = native_pte_val(pte);

	return native_make_pte(v & ~clear);
}

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP
static inline int pte_uffd_wp(pte_t pte)
{
	return pte_flags(pte) & _PAGE_UFFD_WP;
}

static inline pte_t pte_mkuffd_wp(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_UFFD_WP);
}

static inline pte_t pte_clear_uffd_wp(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

static inline pte_t pte_mkclean(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_DIRTY);
}

static inline pte_t pte_mkold(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_ACCESSED);
}

static inline pte_t pte_wrprotect(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_RW);
}

static inline pte_t pte_mkexec(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_NX);
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_ACCESSED);
}

static inline pte_t pte_mkwrite(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_RW);
}

static inline pte_t pte_mkhuge(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_PSE);
}

static inline pte_t pte_clrhuge(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_PSE);
}

static inline pte_t pte_mkglobal(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_GLOBAL);
}

static inline pte_t pte_clrglobal(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_GLOBAL);
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SPECIAL);
}

static inline pte_t pte_mkdevmap(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SPECIAL|_PAGE_DEVMAP);
}

static inline pmd_t pmd_set_flags(pmd_t pmd, pmdval_t set)
{
	pmdval_t v = native_pmd_val(pmd);

	return native_make_pmd(v | set);
}

static inline pmd_t pmd_clear_flags(pmd_t pmd, pmdval_t clear)
{
	pmdval_t v = native_pmd_val(pmd);

	return native_make_pmd(v & ~clear);
}

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP
static inline int pmd_uffd_wp(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_UFFD_WP;
}

static inline pmd_t pmd_mkuffd_wp(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_UFFD_WP);
}

static inline pmd_t pmd_clear_uffd_wp(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

static inline pmd_t pmd_mkold(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_ACCESSED);
}

static inline pmd_t pmd_mkclean(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_DIRTY);
}

static inline pmd_t pmd_wrprotect(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_RW);
}

static inline pmd_t pmd_mkdirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

static inline pmd_t pmd_mkdevmap(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_DEVMAP);
}

static inline pmd_t pmd_mkhuge(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_PSE);
}

static inline pmd_t pmd_mkyoung(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_ACCESSED);
}

static inline pmd_t pmd_mkwrite(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_RW);
}

static inline pud_t pud_set_flags(pud_t pud, pudval_t set)
{
	pudval_t v = native_pud_val(pud);

	return native_make_pud(v | set);
}

static inline pud_t pud_clear_flags(pud_t pud, pudval_t clear)
{
	pudval_t v = native_pud_val(pud);

	return native_make_pud(v & ~clear);
}

static inline pud_t pud_mkold(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_ACCESSED);
}

static inline pud_t pud_mkclean(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_DIRTY);
}

static inline pud_t pud_wrprotect(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_RW);
}

static inline pud_t pud_mkdirty(pud_t pud)
{w
	return pud_set_flags(pud, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);
}

static inline pud_t pud_mkdevmap(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_DEVMAP);
}

static inline pud_t pud_mkhuge(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_PSE);
}

static inline pud_t pud_mkyoung(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_ACCESSED);
}

static inline pud_t pud_mkwrite(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_RW);
}

#ifdef CONFIG_HAVE_ARCH_SOFT_DIRTY
static inline int pte_soft_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SOFT_DIRTY;
}

static inline int pmd_soft_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SOFT_DIRTY;
}

static inline int pud_soft_dirty(pud_t pud)
{
	return pud_flags(pud) & _PAGE_SOFT_DIRTY;
}

static inline pte_t pte_mksoft_dirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SOFT_DIRTY);
}

static inline pmd_t pmd_mksoft_dirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SOFT_DIRTY);
}

static inline pud_t pud_mksoft_dirty(pud_t pud)
{
	return pud_set_flags(pud, _PAGE_SOFT_DIRTY);
}

static inline pte_t pte_clear_soft_dirty(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SOFT_DIRTY);
}

static inline pmd_t pmd_clear_soft_dirty(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SOFT_DIRTY);
}

static inline pud_t pud_clear_soft_dirty(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_SOFT_DIRTY);
}

#endif /* CONFIG_HAVE_ARCH_SOFT_DIRTY */

#endif

/*
 * Mask out unsupported bits in a present pgprot.  Non-present pgprots
 * can use those bits for other purposes, so leave them be.
 */
static inline pgprotval_t massage_pgprot(pgprot_t pgprot)
{
	pgprotval_t protval = pgprot_val(pgprot);

	if (protval & _PAGE_PRESENT)
		protval &= __supported_pte_mask;

	return protval;
}

static inline pgprotval_t check_pgprot(pgprot_t pgprot)
{
	pgprotval_t massaged_val = massage_pgprot(pgprot);

	/* mmdebug.h can not be included here because of dependencies */
#ifdef CONFIG_DEBUG_VM
	WARN_ONCE(pgprot_val(pgprot) != massaged_val,
		  "attempted to set unsupported pgprot: %016llx "
		  "bits: %016llx supported: %016llx\n",
		  (u64)pgprot_val(pgprot),
		  (u64)pgprot_val(pgprot) ^ massaged_val,
		  (u64)__supported_pte_mask);
#endif

	return massaged_val;
}

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE

#define pfn_pte(pfn, prot) \
	gen_pte_update(pfn, LEVEL_PTE, ACTION_MK, PTE_ATTR_FROM_PFN_PROT, prot)

static inline pte_t native_pte_mk_from_pfn_prot(unsigned long page_nr,
						pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PTE_PFN_MASK;
	return __pte(pfn | check_pgprot(pgprot));
}

#define pfn_pmd(pfn, prot) \
	gen_pte_update(pfn, LEVEL_PMD, ACTION_MK, PTE_ATTR_FROM_PFN_PROT, prot)
static inline pmd_t native_pmd_mk_from_pfn_prot(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PHYSICAL_PMD_PAGE_MASK;
	return __pmd(pfn | check_pgprot(pgprot));
}

#define pfn_pud(pfn, prot) \
	gen_pte_update(pfn, LEVEL_PUD, ACTION_MK, PTE_ATTR_FROM_PFN_PROT, prot)
static inline pud_t native_pud_mk_from_pfn_prot(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PHYSICAL_PUD_PAGE_MASK;
	return __pud(pfn | check_pgprot(pgprot));
}

#define pmd_mkinvalid(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_INVALID)
static inline pmd_t native_pmd_mk_invalid(pmd_t pmd)
{
	return pfn_pmd(pmd_pfn(pmd),
		      __pgprot(pmd_flags(pmd) & ~(_PAGE_PRESENT|_PAGE_PROTNONE)));
}

static inline u64 flip_protnone_guard(u64 oldval, u64 val, u64 mask);

#define pte_modify(pte, newprot) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_MODIFY, newprot)
static inline pte_t native_pte_mk_modify(pte_t pte, pgprot_t newprot)
{
	pteval_t val = pte_val(pte), oldval = val;

	/*
	 * Chop off the NX bit (if present), and add the NX portion of
	 * the newprot (if present):
	 */
	val &= _PAGE_CHG_MASK;
	val |= check_pgprot(newprot) & ~_PAGE_CHG_MASK;
	val = flip_protnone_guard(oldval, val, PTE_PFN_MASK);
	return __pte(val);
}

#define pmd_modify(pmd, newprot) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_MODIFY, newprot)
static inline pmd_t native_pmd_mk_modify(pmd_t pmd, pgprot_t newprot)
{
	pmdval_t val = pmd_val(pmd), oldval = val;

	val &= _HPAGE_CHG_MASK;
	val |= check_pgprot(newprot) & ~_HPAGE_CHG_MASK;
	val = flip_protnone_guard(oldval, val, PHYSICAL_PMD_PAGE_MASK);
	return __pmd(val);
}

/*
 * mprotect needs to preserve PAT and encryption bits when updating
 * vm_page_prot
 */

#define pgprot_modify(pgprot, newprot) \
	gen_pte_update(pgprot, LEVEL_PTE, ACTION_MK, PTE_ATTR_PGPROT_MODIFY, newprot)
static inline pgprot_t native_pte_mk_pgprot_modify(pgprot_t oldprot, pgprot_t newprot)
{
	pgprotval_t preservebits = pgprot_val(oldprot) & _PAGE_CHG_MASK;
	pgprotval_t addbits = pgprot_val(newprot) & ~_PAGE_CHG_MASK;
	return __pgprot(preservebits | addbits);
}

#define native_pte_pgprot(x) __pgprot(pte_flags(x))
#define native_pmd_pgprot(x) __pgprot(pmd_flags(x))
#define native_pud_pgprot(x) __pgprot(pud_flags(x))
#define native_p4d_pgprot(x) __pgprot(p4d_flags(x))

#define pte_pgprot(x) gen_pte_read(x, LEVEL_PTE, PTE_ATTR_PGPROT)
#define pmd_pgprot(x) gen_pte_read(x, LEVEL_PMD, PTE_ATTR_PGPROT)
#define pud_pgprot(x) gen_pte_read(x, LEVEL_PUD, PTE_ATTR_PGPROT)
#define p4d_pgprot(x) gen_pte_read(x, LEVEL_P4D, PTE_ATTR_PGPROT)

#else
static inline pte_t pfn_pte(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PTE_PFN_MASK;
	return __pte(pfn | check_pgprot(pgprot));
}

static inline pmd_t pfn_pmd(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PHYSICAL_PMD_PAGE_MASK;
	return __pmd(pfn | check_pgprot(pgprot));
}

static inline pud_t pfn_pud(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PHYSICAL_PUD_PAGE_MASK;
	return __pud(pfn | check_pgprot(pgprot));
}

#define pmd_mkinvalid(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_INVALID)
static inline pmd_t native_pmd_mk_invalid(pmd_t pmd)
{
	return pfn_pmd(pmd_pfn(pmd),
		      __pgprot(pmd_flags(pmd) & ~(_PAGE_PRESENT|_PAGE_PROTNONE)));
}

static inline u64 flip_protnone_guard(u64 oldval, u64 val, u64 mask);

#define pte_modify(pte, newprot) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_MODIFY, newprot)
static inline pte_t native_pte_mk_modify(pte_t pte, pgprot_t newprot)
{
	pteval_t val = pte_val(pte), oldval = val;

	/*
	 * Chop off the NX bit (if present), and add the NX portion of
	 * the newprot (if present):
	 */
	val &= _PAGE_CHG_MASK;
	val |= check_pgprot(newprot) & ~_PAGE_CHG_MASK;
	val = flip_protnone_guard(oldval, val, PTE_PFN_MASK);
	return __pte(val);
}

#define pmd_modify(pmd, newprot) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_MODIFY, newprot)
static inline pmd_t native_pmd_mk_modify(pmd_t pmd, pgprot_t newprot)
{
	pmdval_t val = pmd_val(pmd), oldval = val;

	val &= _HPAGE_CHG_MASK;
	val |= check_pgprot(newprot) & ~_HPAGE_CHG_MASK;
	val = flip_protnone_guard(oldval, val, PHYSICAL_PMD_PAGE_MASK);
	return __pmd(val);
}

/*
 * mprotect needs to preserve PAT and encryption bits when updating
 * vm_page_prot
 */
#define pgprot_modify pgprot_modify
static inline pgprot_t pgprot_modify(pgprot_t oldprot, pgprot_t newprot)
{
	pgprotval_t preservebits = pgprot_val(oldprot) & _PAGE_CHG_MASK;
	pgprotval_t addbits = pgprot_val(newprot) & ~_PAGE_CHG_MASK;
	return __pgprot(preservebits | addbits);
}

#define pte_pgprot(x) __pgprot(pte_flags(x))
#define pmd_pgprot(x) __pgprot(pmd_flags(x))
#define pud_pgprot(x) __pgprot(pud_flags(x))
#define p4d_pgprot(x) __pgprot(p4d_flags(x))

#endif


#define canon_pgprot(p) __pgprot(massage_pgprot(p))

static inline pgprot_t arch_filter_pgprot(pgprot_t prot)
{
	return canon_pgprot(prot);
}

static inline int is_new_memtype_allowed(u64 paddr, unsigned long size,
					 enum page_cache_mode pcm,
					 enum page_cache_mode new_pcm)
{
	/*
	 * PAT type is always WB for untracked ranges, so no need to check.
	 */
	if (x86_platform.is_untracked_pat_range(paddr, paddr + size))
		return 1;

	/*
	 * Certain new memtypes are not allowed with certain
	 * requested memtype:
	 * - request is uncached, return cannot be write-back
	 * - request is write-combine, return cannot be write-back
	 * - request is write-through, return cannot be write-back
	 * - request is write-through, return cannot be write-combine
	 */
	if ((pcm == _PAGE_CACHE_MODE_UC_MINUS &&
	     new_pcm == _PAGE_CACHE_MODE_WB) ||
	    (pcm == _PAGE_CACHE_MODE_WC &&
	     new_pcm == _PAGE_CACHE_MODE_WB) ||
	    (pcm == _PAGE_CACHE_MODE_WT &&
	     new_pcm == _PAGE_CACHE_MODE_WB) ||
	    (pcm == _PAGE_CACHE_MODE_WT &&
	     new_pcm == _PAGE_CACHE_MODE_WC)) {
		return 0;
	}

	return 1;
}

pmd_t *populate_extra_pmd(unsigned long vaddr);
pte_t *populate_extra_pte(unsigned long vaddr);

#ifdef CONFIG_PAGE_TABLE_ISOLATION
pgd_t __pti_set_user_pgtbl(pgd_t *pgdp, pgd_t pgd);

/*
 * Take a PGD location (pgdp) and a pgd value that needs to be set there.
 * Populates the user and returns the resulting PGD that must be set in
 * the kernel copy of the page tables.
 */
static inline pgd_t pti_set_user_pgtbl(pgd_t *pgdp, pgd_t pgd)
{
	if (!static_cpu_has(X86_FEATURE_PTI))
		return pgd;
	return __pti_set_user_pgtbl(pgdp, pgd);
}
#else   /* CONFIG_PAGE_TABLE_ISOLATION */
static inline pgd_t pti_set_user_pgtbl(pgd_t *pgdp, pgd_t pgd)
{
	return pgd;
}
#endif  /* CONFIG_PAGE_TABLE_ISOLATION */

#endif	/* __ASSEMBLY__ */


#ifdef CONFIG_X86_32
# include <asm/pgtable_32.h>
#else
# include <asm/pgtable_64.h>
#endif

#ifndef __ASSEMBLY__
#include <linux/mm_types.h>
#include <linux/mmdebug.h>
#include <linux/log2.h>
#include <asm/fixmap.h>

#ifdef CONFIG_X86_64_ECPT
	#include <asm/ECPT.h>
	#include <asm/ECPT_interface.h>
#endif

/* we don't change this gen_pte_read since it should represent address space mapping property */
static inline int pte_none(pte_t pte)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pte_none(pte);
#else
	return !(pte.pte & ~(_PAGE_KNL_ERRATUM_MASK));
#endif
}

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE

#define __HAVE_ARCH_PTE_SAME
#define pte_same(a, b) \
	gen_pte_read(a, LEVEL_PTE, PTE_ATTR_SAME, b)

static inline int native_pte_same(pte_t a, pte_t b)
{
	return a.pte == b.pte;
}

#define __HAVE_ARCH_PMD_SAME
#define pmd_same(a, b) \
	gen_pte_read(a, LEVEL_PMD, PTE_ATTR_SAME, b)

static inline int pmd_same(pmd_t pmd_a, pmd_t pmd_b)
{
	return pmd_val(pmd_a) == pmd_val(pmd_b);
}

#define pud_same(pud_a, pud_b) \
	gen_pte_read(pud_a, LEVEL_PUD, PTE_ATTR_SAME, pud_b)
static inline int pud_same(pud_t pud_a, pud_t pud_b)
{
	return pud_val(pud_a) == pud_val(pud_b);
}

#define __HAVE_ARCH_P4D_SAME

#define p4d_same(p4d_a, p4d_b) \
	gen_pte_read(p4d_a, LEVEL_P4D, PTE_ATTR_SAME, p4d_b)
static inline int p4d_same(p4d_t p4d_a, p4d_t p4d_b)
{
	return p4d_val(p4d_a) == p4d_val(p4d_b);
}

#define __HAVE_ARCH_PGD_SAME
#define pgd_same(a, b) \
	gen_pte_read(a, LEVEL_PGD, PTE_ATTR_SAME, b)
static inline int pgd_same(pgd_t pgd_a, pgd_t pgd_b)
{
	return pgd_val(pgd_a) == pgd_val(pgd_b);
}


#define pte_present(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_PRESENT)

static inline int native_pte_present(pte_t a)
{
	return pte_flags(a) & (_PAGE_PRESENT | _PAGE_PROTNONE);
}

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP

#define pte_devmap(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_DEVMAP)
static inline int native_pte_devmap(pte_t a)
{
	return (pte_flags(a) & _PAGE_DEVMAP) == _PAGE_DEVMAP;
}
#endif

// #define pte_accessible pte_accessible
#define pte_accessible(mm, pte) \
	gen_pte_read(mm, LEVEL_PTE, PTE_ATTR_ACCESSIBLE, pte)
static inline bool native_pte_accessible(struct mm_struct *mm, pte_t a)
{
	if (pte_flags(a) & _PAGE_PRESENT)
		return true;

	if ((pte_flags(a) & _PAGE_PROTNONE) &&
			mm_tlb_flush_pending(mm))
		return true;

	return false;
}

#define pmd_present(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_PRESENT) 

static inline int native_pmd_present(pmd_t pmd)
{
	/*
	 * Checking for _PAGE_PSE is needed too because
	 * split_huge_page will temporarily clear the present bit (but
	 * the _PAGE_PSE flag will remain set at all times while the
	 * _PAGE_PRESENT bit is clear).
	 */
	return pmd_flags(pmd) & (_PAGE_PRESENT | _PAGE_PROTNONE | _PAGE_PSE);
}

#ifdef CONFIG_NUMA_BALANCING
/*
 * These work without NUMA balancing but the kernel does not care. See the
 * comment in include/linux/pgtable.h
 */
#define pte_protnone(pte) \
	gen_pte_read(pte, LEVEL_PTE, PTE_ATTR_PROTNONE)	

static inline int pte_protnone(pte_t pte)
{
	return (pte_flags(pte) & (_PAGE_PROTNONE | _PAGE_PRESENT))
		== _PAGE_PROTNONE;
}

#define pmd_protnone(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_PROTNONE)
static inline int pmd_protnone(pmd_t pmd)
{
	return (pmd_flags(pmd) & (_PAGE_PROTNONE | _PAGE_PRESENT))
		== _PAGE_PROTNONE;
}

#endif /* CONFIG_NUMA_BALANCING */

static inline int pmd_none(pmd_t pmd)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pmd_none(pmd);
#else
	/* Only check low word on 32-bit platforms, since it might be
	   out of sync with upper half. */
	unsigned long val = native_pmd_val(pmd);
	return (val & ~_PAGE_KNL_ERRATUM_MASK) == 0;
#endif
}

#define pmd_page_vaddr(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_PAGE_VADDR)

static inline unsigned long native_pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_val(pmd) & pmd_pfn_mask(pmd));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pmd_page(pmd)	pfn_to_page(pmd_pfn(pmd))

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 *
 * (Currently stuck as a macro because of indirect forward reference
 * to linux/mm.h:page_to_nid())
 */
#define mk_pte(page, pgprot)   pfn_pte(page_to_pfn(page), (pgprot))

#define pmd_bad(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_BAD) 

static inline int native_pmd_bad(pmd_t pmd)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pmd_bad(pmd);
#else
	return (pmd_flags(pmd) & ~_PAGE_USER) != _KERNPG_TABLE;
#endif

}

static inline unsigned long pages_to_mb(unsigned long npg)
{
	return npg >> (20 - PAGE_SHIFT);
}

#if CONFIG_PGTABLE_LEVELS > 2
static inline int pud_none(pud_t pud)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pud_none(pud);
#else
	return (native_pud_val(pud) & ~(_PAGE_KNL_ERRATUM_MASK)) == 0;
#endif
	
}

#define pud_present(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_PRESENT) 

static inline int native_pud_present(pud_t pud)
{
	return pud_flags(pud) & _PAGE_PRESENT;
}


#define pud_pgtable(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_PGTABLE)
static inline pmd_t * native_pud_pgtable(pud_t pud)
{
	return (pmd_t *)__va(pud_val(pud) & pud_pfn_mask(pud));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pud_page(pud)	pfn_to_page(pud_pfn(pud))


#define pud_leaf(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_LEAF)
#define native_pud_leaf	native_pud_large

#define pud_large(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_LARGE)
static inline int native_pud_large(pud_t pud)
{
	return (pud_val(pud) & (_PAGE_PSE | _PAGE_PRESENT)) ==
		(_PAGE_PSE | _PAGE_PRESENT);
}

#define pud_bad(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_BAD)  

static inline int native_pud_bad(pud_t pud)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pud_bad(pud);
#else
	return (pud_flags(pud) & ~(_KERNPG_TABLE | _PAGE_USER)) != 0;
#endif
}
#else
#define pud_leaf	pud_large
static inline int native_pud_large(pud_t pud)
{
	return 0;
}
#endif	/* CONFIG_PGTABLE_LEVELS > 2 */

#if CONFIG_PGTABLE_LEVELS > 3
static inline int p4d_none(p4d_t p4d)
{
	return (native_p4d_val(p4d) & ~(_PAGE_KNL_ERRATUM_MASK)) == 0;
}

#define p4d_present(p4dval) \
	gen_pte_read(p4dval, LEVEL_P4D, PTE_ATTR_PRESENT) 

static inline int native_p4d_present(p4d_t p4d)
{
	return p4d_flags(p4d) & _PAGE_PRESENT;
}

#define p4d_pgtable(p4d) \
	gen_pte_read(p4d, LEVEL_P4D, PTE_ATTR_PGTABLE)

static inline pud_t *native_p4d_pgtable(p4d_t p4d)
{
	return (pud_t *)__va(p4d_val(p4d) & p4d_pfn_mask(p4d));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define p4d_page(p4d)	pfn_to_page(p4d_pfn(p4d))


#define p4d_bad(p4dval) \
	gen_pte_read(p4dval, LEVEL_P4D, PTE_ATTR_BAD) 

static inline int native_p4d_bad(p4d_t p4d)
{
	unsigned long ignore_flags = _KERNPG_TABLE | _PAGE_USER;

	if (IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION))
		ignore_flags |= _PAGE_NX;

	return (p4d_flags(p4d) & ~ignore_flags) != 0;
}
#endif  /* CONFIG_PGTABLE_LEVELS > 3 */

static inline unsigned long p4d_index(unsigned long address)
{
	return (address >> P4D_SHIFT) & (PTRS_PER_P4D - 1);
}

#if CONFIG_PGTABLE_LEVELS > 4
#define pgd_present(pgdval) \
	gen_pte_read(pgdval, LEVEL_PGD, PTE_ATTR_PRESENT) 

static inline int native_pgd_present(pgd_t pgd)
{
	if (!pgtable_l5_enabled())
		return 1;
	return pgd_flags(pgd) & _PAGE_PRESENT;
}

#define pgd_page_vaddr(pgd) \
	gen_pte_read(pgd, LEVEL_PGD, PTE_ATTR_PAGE_VADDR)

static inline unsigned long native_pgd_page_vaddr(pgd_t pgd)
{
	return (unsigned long)__va((unsigned long)pgd_val(pgd) & PTE_PFN_MASK);
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pgd_page(pgd)	pfn_to_page(pgd_pfn(pgd))

/* to find an entry in a page-table-directory. */
static inline p4d_t *p4d_offset(pgd_t *pgd, unsigned long address)
{
	if (!pgtable_l5_enabled())
		return (p4d_t *)pgd;
	return (p4d_t *)pgd_page_vaddr(*pgd) + p4d_index(address);
}

#define pgd_bad(pgdval) \
	gen_pte_read(pgdval, LEVEL_PGD, PTE_ATTR_BAD)  

static inline int native_pgd_bad(pgd_t pgd)
{
	unsigned long ignore_flags = _PAGE_USER;

	if (!pgtable_l5_enabled())
		return 0;

	if (IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION))
		ignore_flags |= _PAGE_NX;

	return (pgd_flags(pgd) & ~ignore_flags) != _KERNPG_TABLE;
}

static inline int pgd_none(pgd_t pgd)
{
	if (!pgtable_l5_enabled())
		return 0;
	/*
	 * There is no need to do a workaround for the KNL stray
	 * A/D bit erratum here.  PGDs only point to page tables
	 * except on 32-bit non-PAE which is not supported on
	 * KNL.
	 */
	return !native_pgd_val(pgd);
}
#endif	/* CONFIG_PGTABLE_LEVELS > 4 */

#else
#define __HAVE_ARCH_PTE_SAME
static inline int pte_same(pte_t a, pte_t b)
{
	return a.pte == b.pte;
}

#define pte_present(pteval) \
	gen_pte_read(pteval, LEVEL_PTE, PTE_ATTR_PRESENT)

static inline int native_pte_present(pte_t a)
{
	return pte_flags(a) & (_PAGE_PRESENT | _PAGE_PROTNONE);
}

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP
static inline int pte_devmap(pte_t a)
{
	return (pte_flags(a) & _PAGE_DEVMAP) == _PAGE_DEVMAP;
}
#endif

#define pte_accessible pte_accessible
static inline bool pte_accessible(struct mm_struct *mm, pte_t a)
{
	if (pte_flags(a) & _PAGE_PRESENT)
		return true;

	if ((pte_flags(a) & _PAGE_PROTNONE) &&
			mm_tlb_flush_pending(mm))
		return true;

	return false;
}

#define pmd_present(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_PRESENT) 

static inline int native_pmd_present(pmd_t pmd)
{
	/*
	 * Checking for _PAGE_PSE is needed too because
	 * split_huge_page will temporarily clear the present bit (but
	 * the _PAGE_PSE flag will remain set at all times while the
	 * _PAGE_PRESENT bit is clear).
	 */
	return pmd_flags(pmd) & (_PAGE_PRESENT | _PAGE_PROTNONE | _PAGE_PSE);
}

#ifdef CONFIG_NUMA_BALANCING
/*
 * These work without NUMA balancing but the kernel does not care. See the
 * comment in include/linux/pgtable.h
 */
static inline int pte_protnone(pte_t pte)
{
	return (pte_flags(pte) & (_PAGE_PROTNONE | _PAGE_PRESENT))
		== _PAGE_PROTNONE;
}

static inline int pmd_protnone(pmd_t pmd)
{
	return (pmd_flags(pmd) & (_PAGE_PROTNONE | _PAGE_PRESENT))
		== _PAGE_PROTNONE;
}
#endif /* CONFIG_NUMA_BALANCING */

static inline int pmd_none(pmd_t pmd)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pmd_none(pmd);
#else
	/* Only check low word on 32-bit platforms, since it might be
	   out of sync with upper half. */
	unsigned long val = native_pmd_val(pmd);
	return (val & ~_PAGE_KNL_ERRATUM_MASK) == 0;
#endif
}

/* #define pmd_none(pmd)  \
({	\
	if (!(__FILE__[0] == 'a' && __FILE__[1] == 'r' && __FILE__[2] == 'c')) { \
		WARN(1, "%s:%s not implemented with generalized interface (pmd_none)\n", \
	 		__FILE__ ,__func__); \
	}							\
	pmd_none(pmd);	\
}) */

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_val(pmd) & pmd_pfn_mask(pmd));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pmd_page(pmd)	pfn_to_page(pmd_pfn(pmd))

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 *
 * (Currently stuck as a macro because of indirect forward reference
 * to linux/mm.h:page_to_nid())
 */
#define mk_pte(page, pgprot)   pfn_pte(page_to_pfn(page), (pgprot))

#define pmd_bad(pmdval) \
	gen_pte_read(pmdval, LEVEL_PMD, PTE_ATTR_BAD) 

static inline int native_pmd_bad(pmd_t pmd)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pmd_bad(pmd);
#else
	return (pmd_flags(pmd) & ~_PAGE_USER) != _KERNPG_TABLE;
#endif

}

static inline unsigned long pages_to_mb(unsigned long npg)
{
	return npg >> (20 - PAGE_SHIFT);
}

#if CONFIG_PGTABLE_LEVELS > 2
static inline int pud_none(pud_t pud)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pud_none(pud);
#else
	return (native_pud_val(pud) & ~(_PAGE_KNL_ERRATUM_MASK)) == 0;
#endif
	
}

#define pud_present(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_PRESENT) 

static inline int native_pud_present(pud_t pud)
{
	return pud_flags(pud) & _PAGE_PRESENT;
}

static inline pmd_t *pud_pgtable(pud_t pud)
{
	return (pmd_t *)__va(pud_val(pud) & pud_pfn_mask(pud));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pud_page(pud)	pfn_to_page(pud_pfn(pud))

#define pud_leaf	pud_large
static inline int pud_large(pud_t pud)
{
	return (pud_val(pud) & (_PAGE_PSE | _PAGE_PRESENT)) ==
		(_PAGE_PSE | _PAGE_PRESENT);
}

#define pud_bad(pudval) \
	gen_pte_read(pudval, LEVEL_PUD, PTE_ATTR_BAD)  

static inline int native_pud_bad(pud_t pud)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_pud_bad(pud);
#else
	return (pud_flags(pud) & ~(_KERNPG_TABLE | _PAGE_USER)) != 0;
#endif
}
#else
#define pud_leaf	pud_large
static inline int pud_large(pud_t pud)
{
	return 0;
}
#endif	/* CONFIG_PGTABLE_LEVELS > 2 */

#if CONFIG_PGTABLE_LEVELS > 3
static inline int p4d_none(p4d_t p4d)
{
	return (native_p4d_val(p4d) & ~(_PAGE_KNL_ERRATUM_MASK)) == 0;
}

#define p4d_present(p4dval) \
	gen_pte_read(p4dval, LEVEL_P4D, PTE_ATTR_PRESENT) 

static inline int native_p4d_present(p4d_t p4d)
{
	return p4d_flags(p4d) & _PAGE_PRESENT;
}

static inline pud_t *p4d_pgtable(p4d_t p4d)
{
	return (pud_t *)__va(p4d_val(p4d) & p4d_pfn_mask(p4d));
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define p4d_page(p4d)	pfn_to_page(p4d_pfn(p4d))


#define p4d_bad(p4dval) \
	gen_pte_read(p4dval, LEVEL_P4D, PTE_ATTR_BAD) 

static inline int native_p4d_bad(p4d_t p4d)
{
	unsigned long ignore_flags = _KERNPG_TABLE | _PAGE_USER;

	if (IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION))
		ignore_flags |= _PAGE_NX;

	return (p4d_flags(p4d) & ~ignore_flags) != 0;
}
#endif  /* CONFIG_PGTABLE_LEVELS > 3 */

static inline unsigned long p4d_index(unsigned long address)
{
	return (address >> P4D_SHIFT) & (PTRS_PER_P4D - 1);
}

#if CONFIG_PGTABLE_LEVELS > 4
#define pgd_present(pgdval) \
	gen_pte_read(pgdval, LEVEL_PGD, PTE_ATTR_PRESENT) 

static inline int native_pgd_present(pgd_t pgd)
{
	if (!pgtable_l5_enabled())
		return 1;
	return pgd_flags(pgd) & _PAGE_PRESENT;
}

static inline unsigned long pgd_page_vaddr(pgd_t pgd)
{
	return (unsigned long)__va((unsigned long)pgd_val(pgd) & PTE_PFN_MASK);
}

/*
 * Currently stuck as a macro due to indirect forward reference to
 * linux/mmzone.h's __section_mem_map_addr() definition:
 */
#define pgd_page(pgd)	pfn_to_page(pgd_pfn(pgd))

/* to find an entry in a page-table-directory. */
static inline p4d_t *p4d_offset(pgd_t *pgd, unsigned long address)
{
	if (!pgtable_l5_enabled())
		return (p4d_t *)pgd;
	return (p4d_t *)pgd_page_vaddr(*pgd) + p4d_index(address);
}

#define pgd_bad(pgdval) \
	gen_pte_read(pgdval, LEVEL_PGD, PTE_ATTR_BAD)  

static inline int native_pgd_bad(pgd_t pgd)
{
	unsigned long ignore_flags = _PAGE_USER;

	if (!pgtable_l5_enabled())
		return 0;

	if (IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION))
		ignore_flags |= _PAGE_NX;

	return (pgd_flags(pgd) & ~ignore_flags) != _KERNPG_TABLE;
}

static inline int pgd_none(pgd_t pgd)
{
	if (!pgtable_l5_enabled())
		return 0;
	/*
	 * There is no need to do a workaround for the KNL stray
	 * A/D bit erratum here.  PGDs only point to page tables
	 * except on 32-bit non-PAE which is not supported on
	 * KNL.
	 */
	return !native_pgd_val(pgd);
}
#endif	/* CONFIG_PGTABLE_LEVELS > 4 */

#endif /* CONFIG_PGTABLE_OP_GENERALIZABLE */
#endif	/* __ASSEMBLY__ */

#define KERNEL_PGD_BOUNDARY	pgd_index(PAGE_OFFSET)
#define KERNEL_PGD_PTRS		(PTRS_PER_PGD - KERNEL_PGD_BOUNDARY)

#ifndef __ASSEMBLY__

extern int direct_gbpages;
void init_mem_mapping(void);
void early_alloc_pgt_buf(void);
extern void memblock_find_dma_reserve(void);
void __init poking_init(void);
unsigned long init_memory_mapping(unsigned long start,
				  unsigned long end, pgprot_t prot);

#ifdef CONFIG_X86_64
extern pgd_t trampoline_pgd_entry;
#endif

/* local pte updates need not use xchg for locking */
static inline pte_t native_local_ptep_get_and_clear(pte_t *ptep)
{
	pte_t res = *ptep;

	/* Pure native function needs no input for mm, addr */
	native_pte_clear(NULL, 0, ptep);
	return res;
}

static inline pmd_t native_local_pmdp_get_and_clear(pmd_t *pmdp)
{
	pmd_t res = *pmdp;

	native_pmd_clear(pmdp);
	return res;
}

static inline pud_t native_local_pudp_get_and_clear(pud_t *pudp)
{
	pud_t res = *pudp;

	native_pud_clear(pudp);
	return res;
}

static inline int set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pte)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_set_pte_at(mm, addr, ptep, pte);
#else
	set_pte(ptep, pte);
	return 0;
#endif
	
}

static inline void set_pmd_at(struct mm_struct *mm, unsigned long addr,
			      pmd_t *pmdp, pmd_t pmd)
{
#ifdef CONFIG_X86_64_ECPT
	ecpt_set_pmd_at(mm, addr, pmdp, pmd);
#else
	set_pmd(pmdp, pmd);
#endif
}

static inline void set_pud_at(struct mm_struct *mm, unsigned long addr,
			      pud_t *pudp, pud_t pud)
{
#ifdef CONFIG_X86_64_ECPT
	ecpt_set_pud_at(mm, addr, pudp, pud);
#else
	native_set_pud(pudp, pud);
#endif	
}

/*
 * We only update the dirty/accessed state if we set
 * the dirty bit by hand in the kernel, since the hardware
 * will do the accessed bit for us, and we don't want to
 * race with other CPU's that might be updating the dirty
 * bit at the same time.
 */
struct vm_area_struct;

#define  __HAVE_ARCH_PTEP_SET_ACCESS_FLAGS
extern int ptep_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pte_t *ptep,
				 pte_t entry, int dirty);

#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
extern int ptep_test_and_clear_young(struct vm_area_struct *vma,
				     unsigned long addr, pte_t *ptep);

#define __HAVE_ARCH_PTEP_CLEAR_YOUNG_FLUSH
extern int ptep_clear_flush_young(struct vm_area_struct *vma,
				  unsigned long address, pte_t *ptep);

#define __HAVE_ARCH_PTEP_GET_AND_CLEAR
static inline pte_t ptep_get_and_clear(struct mm_struct *mm, unsigned long addr,
				       pte_t *ptep)
{
	pte_t pte;
#ifdef CONFIG_X86_64_ECPT
	pte = ecpt_native_ptep_get_and_clear(mm, addr, ptep);
#else
	pte = native_ptep_get_and_clear(ptep);
#endif
	return pte;
}

#define __HAVE_ARCH_PTEP_GET_AND_CLEAR_FULL
static inline pte_t ptep_get_and_clear_full(struct mm_struct *mm,
					    unsigned long addr, pte_t *ptep,
					    int full)
{
	pte_t pte;
	if (full) {
		/*
		 * Full address destruction in progress; paravirt does not
		 * care about updates and native needs no locking
		 */
#ifdef CONFIG_X86_64_ECPT
	pte = ecpt_native_ptep_get_and_clear(mm, addr, ptep);
#else
	pte = native_local_ptep_get_and_clear(ptep);
#endif
	} else {
#ifdef CONFIG_X86_64_ECPT
	pte = ecpt_native_ptep_get_and_clear(mm, addr, ptep);
#else
	pte = ptep_get_and_clear(mm, addr, ptep);
#endif
		
	}
	return pte;
}

#define __HAVE_ARCH_PTEP_SET_WRPROTECT
static inline void ptep_set_wrprotect(struct mm_struct *mm,
				      unsigned long addr, pte_t *ptep)
{
	clear_bit(_PAGE_BIT_RW, (unsigned long *)&ptep->pte);
}

#define flush_tlb_fix_spurious_fault(vma, address) do { } while (0)

#define mk_pmd(page, pgprot)   pfn_pmd(page_to_pfn(page), (pgprot))

#define  __HAVE_ARCH_PMDP_SET_ACCESS_FLAGS
extern int pmdp_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pmd_t *pmdp,
				 pmd_t entry, int dirty);
extern int pudp_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pud_t *pudp,
				 pud_t entry, int dirty);

#define __HAVE_ARCH_PMDP_TEST_AND_CLEAR_YOUNG
extern int pmdp_test_and_clear_young(struct vm_area_struct *vma,
				     unsigned long addr, pmd_t *pmdp);
extern int pudp_test_and_clear_young(struct vm_area_struct *vma,
				     unsigned long addr, pud_t *pudp);

#define __HAVE_ARCH_PMDP_CLEAR_YOUNG_FLUSH
extern int pmdp_clear_flush_young(struct vm_area_struct *vma,
				  unsigned long address, pmd_t *pmdp);


#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE

#define pmd_write(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_WRITE)
static inline int native_pmd_write(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_RW;
}

#define pud_write(pud) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_WRITE)
static inline int native_pud_write(pud_t pud)
{
	return pud_flags(pud) & _PAGE_RW;
}

#else
#define pmd_write pmd_write
static inline int pmd_write(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_RW;
}

#define pud_write pud_write
static inline int pud_write(pud_t pud)
{
	return pud_flags(pud) & _PAGE_RW;
}

#endif

#define __HAVE_ARCH_PMDP_HUGE_GET_AND_CLEAR
static inline pmd_t pmdp_huge_get_and_clear(struct mm_struct *mm, unsigned long addr,
				       pmd_t *pmdp)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_native_pmdp_get_and_clear(mm, addr, pmdp);
#endif
	return native_pmdp_get_and_clear(pmdp);
}

#define __HAVE_ARCH_PUDP_HUGE_GET_AND_CLEAR
static inline pud_t pudp_huge_get_and_clear(struct mm_struct *mm,
					unsigned long addr, pud_t *pudp)
{
#ifdef CONFIG_X86_64_ECPT
	return ecpt_native_pudp_get_and_clear(mm, addr, pudp);
#endif

	return native_pudp_get_and_clear(pudp);
}

#define __HAVE_ARCH_PMDP_SET_WRPROTECT
static inline void pmdp_set_wrprotect(struct mm_struct *mm,
				      unsigned long addr, pmd_t *pmdp)
{
	clear_bit(_PAGE_BIT_RW, (unsigned long *)pmdp);
}



#ifndef pmdp_establish
#define pmdp_establish pmdp_establish
static inline pmd_t pmdp_establish(struct vm_area_struct *vma,
		unsigned long address, pmd_t *pmdp, pmd_t pmd)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		return xchg(pmdp, pmd);
	} else {
		pmd_t old = *pmdp;
		WRITE_ONCE(*pmdp, pmd);
		return old;
	}
}
#endif
/*
 * Page table pages are page-aligned.  The lower half of the top
 * level is used for userspace and the top half for the kernel.
 *
 * Returns true for parts of the PGD that map userspace and
 * false for the parts that map the kernel.
 */
static inline bool pgdp_maps_userspace(void *__ptr)
{
	unsigned long ptr = (unsigned long)__ptr;

	return (((ptr & ~PAGE_MASK) / sizeof(pgd_t)) < PGD_KERNEL_START);
}


#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE


static inline int native_pgd_large(pgd_t pgd) { return 0; }
#define native_pgd_leaf native_pgd_large

#define pgd_leaf(pgd) \
	gen_pte_read(pgd, LEVEL_PGD, PTE_ATTR_LEAF)
#define pgd_large(pgd) \
	gen_pte_read(pgd, LEVEL_PGD, PTE_ATTR_LARGE)

#else
#define pgd_leaf	pgd_large
static inline int pgd_large(pgd_t pgd) { return 0; }
#endif /* CONFIG_PGTABLE_OP_GENERALIZABLE */

#ifdef CONFIG_PAGE_TABLE_ISOLATION
/*
 * All top-level PAGE_TABLE_ISOLATION page tables are order-1 pages
 * (8k-aligned and 8k in size).  The kernel one is at the beginning 4k and
 * the user one is in the last 4k.  To switch between them, you
 * just need to flip the 12th bit in their addresses.
 */
#define PTI_PGTABLE_SWITCH_BIT	PAGE_SHIFT

/*
 * This generates better code than the inline assembly in
 * __set_bit().
 */
static inline void *ptr_set_bit(void *ptr, int bit)
{
	unsigned long __ptr = (unsigned long)ptr;

	__ptr |= BIT(bit);
	return (void *)__ptr;
}
static inline void *ptr_clear_bit(void *ptr, int bit)
{
	unsigned long __ptr = (unsigned long)ptr;

	__ptr &= ~BIT(bit);
	return (void *)__ptr;
}

static inline pgd_t *kernel_to_user_pgdp(pgd_t *pgdp)
{
	return ptr_set_bit(pgdp, PTI_PGTABLE_SWITCH_BIT);
}

static inline pgd_t *user_to_kernel_pgdp(pgd_t *pgdp)
{
	return ptr_clear_bit(pgdp, PTI_PGTABLE_SWITCH_BIT);
}

static inline p4d_t *kernel_to_user_p4dp(p4d_t *p4dp)
{
	return ptr_set_bit(p4dp, PTI_PGTABLE_SWITCH_BIT);
}

static inline p4d_t *user_to_kernel_p4dp(p4d_t *p4dp)
{
	return ptr_clear_bit(p4dp, PTI_PGTABLE_SWITCH_BIT);
}
#endif /* CONFIG_PAGE_TABLE_ISOLATION */

/*
 * clone_pgd_range(pgd_t *dst, pgd_t *src, int count);
 *
 *  dst - pointer to pgd range anywhere on a pgd page
 *  src - ""
 *  count - the number of pgds to copy.
 *
 * dst and src can be on the same page, but the range must not overlap,
 * and must not cross a page boundary.
 */
static inline void clone_pgd_range(pgd_t *dst, pgd_t *src, int count)
{
	memcpy(dst, src, count * sizeof(pgd_t));
#ifdef CONFIG_PAGE_TABLE_ISOLATION
	if (!static_cpu_has(X86_FEATURE_PTI))
		return;
	/* Clone the user space pgd as well */
	memcpy(kernel_to_user_pgdp(dst), kernel_to_user_pgdp(src),
	       count * sizeof(pgd_t));
#endif
}

#define PTE_SHIFT ilog2(PTRS_PER_PTE)
static inline int page_level_shift(enum pg_level level)
{
	return (PAGE_SHIFT - PTE_SHIFT) + level * PTE_SHIFT;
}
static inline unsigned long page_level_size(enum pg_level level)
{
	return 1UL << page_level_shift(level);
}
static inline unsigned long page_level_mask(enum pg_level level)
{
	return ~(page_level_size(level) - 1);
}

/*
 * The x86 doesn't have any external MMU info: the kernel page
 * tables contain all the necessary information.
 */
static inline void update_mmu_cache(struct vm_area_struct *vma,
		unsigned long addr, pte_t *ptep)
{
}
static inline void update_mmu_cache_pmd(struct vm_area_struct *vma,
		unsigned long addr, pmd_t *pmd)
{
}
static inline void update_mmu_cache_pud(struct vm_area_struct *vma,
		unsigned long addr, pud_t *pud)
{
}

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE

#ifdef CONFIG_HAVE_ARCH_SOFT_DIRTY
#define pte_swp_mksoft_dirty(pte)                                              \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_SWP_SOFT_DIRTY)
static inline pte_t native_pte_mk_swp_soft_dirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SWP_SOFT_DIRTY);
}

#define pte_swp_soft_dirty(pte)                                                \
	gen_pte_read(pte, LEVEL_PTE, PTE_ATTR_SWP_SOFT_DIRTY)
static inline int native_pte_swp_soft_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SWP_SOFT_DIRTY;
}

#define pte_swp_clear_soft_dirty(pte)                                          \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_SWP_SOFT_DIRTY)
static inline pte_t native_pte_clear_swp_soft_dirty(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SWP_SOFT_DIRTY);
}

#ifdef CONFIG_ARCH_ENABLE_THP_MIGRATION
#define pmd_swp_mksoft_dirty(pmd)                                              \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_SWP_SOFT_DIRTY)

static inline pmd_t native_pmd_mk_swp_soft_dirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SWP_SOFT_DIRTY);
}

#define pmd_swp_soft_dirty(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_SWP_SOFT_DIRTY)
static inline int pmd_swp_soft_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SWP_SOFT_DIRTY;
}

#define pmd_swp_clear_soft_dirty(pmd)                                          \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_SWP_SOFT_DIRTY)

static inline pmd_t native_pmd_clear_swp_soft_dirty(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SWP_SOFT_DIRTY);
}
#endif
#endif

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP
#define pte_swp_mkuffd_wp(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_MK, PTE_ATTR_SWP_UFFD_WP)

static inline pte_t  native_pte_mk_swp_uffd_wp(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SWP_UFFD_WP);
}

#define pte_swp_uffd_wp(pte) \
	gen_pte_read(pte, LEVEL_PTE, PTE_ATTR_SWP_UFFD_WP)
static inline int pte_swp_uffd_wp(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SWP_UFFD_WP;
}

#define pte_swp_clear_uffd_wp(pte) \
	gen_pte_update(pte, LEVEL_PTE, ACTION_CLEAR, PTE_ATTR_SWP_UFFD_WP)
static inline pte_t native_pte_clear_swp_uffd_wp(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SWP_UFFD_WP);
}

#define pmd_swp_mkuffd_wp(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_MK, PTE_ATTR_SWP_UFFD_WP)
static inline pmd_t native_pmd_mk_swp_uffd_wp(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SWP_UFFD_WP);
}

#define pmd_swp_uffd_wp(pmd) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_SWP_UFFD_WP)
static inline int pmd_swp_uffd_wp(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SWP_UFFD_WP;
}

#define pmd_swp_clear_uffd_wp(pmd) \
	gen_pte_update(pmd, LEVEL_PMD, ACTION_CLEAR, PTE_ATTR_SWP_UFFD_WP)
static inline pmd_t native_pmd_clear_swp_uffd_wp(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SWP_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

#else

#ifdef CONFIG_HAVE_ARCH_SOFT_DIRTY
static inline pte_t pte_swp_mksoft_dirty(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SWP_SOFT_DIRTY);
}

static inline int pte_swp_soft_dirty(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SWP_SOFT_DIRTY;
}

static inline pte_t pte_swp_clear_soft_dirty(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SWP_SOFT_DIRTY);
}

#ifdef CONFIG_ARCH_ENABLE_THP_MIGRATION
static inline pmd_t pmd_swp_mksoft_dirty(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SWP_SOFT_DIRTY);
}

static inline int pmd_swp_soft_dirty(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SWP_SOFT_DIRTY;
}

static inline pmd_t pmd_swp_clear_soft_dirty(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SWP_SOFT_DIRTY);
}
#endif
#endif

#ifdef CONFIG_HAVE_ARCH_USERFAULTFD_WP
static inline pte_t pte_swp_mkuffd_wp(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_SWP_UFFD_WP);
}

static inline int pte_swp_uffd_wp(pte_t pte)
{
	return pte_flags(pte) & _PAGE_SWP_UFFD_WP;
}

static inline pte_t pte_swp_clear_uffd_wp(pte_t pte)
{
	return pte_clear_flags(pte, _PAGE_SWP_UFFD_WP);
}

static inline pmd_t pmd_swp_mkuffd_wp(pmd_t pmd)
{
	return pmd_set_flags(pmd, _PAGE_SWP_UFFD_WP);
}

static inline int pmd_swp_uffd_wp(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_SWP_UFFD_WP;
}

static inline pmd_t pmd_swp_clear_uffd_wp(pmd_t pmd)
{
	return pmd_clear_flags(pmd, _PAGE_SWP_UFFD_WP);
}
#endif /* CONFIG_HAVE_ARCH_USERFAULTFD_WP */

#endif

static inline u16 pte_flags_pkey(unsigned long pte_flags)
{
#ifdef CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
	/* ifdef to avoid doing 59-bit shift on 32-bit values */
	return (pte_flags & _PAGE_PKEY_MASK) >> _PAGE_BIT_PKEY_BIT0;
#else
	return 0;
#endif
}

static inline bool __pkru_allows_pkey(u16 pkey, bool write)
{
	u32 pkru = read_pkru();

	if (!__pkru_allows_read(pkru, pkey))
		return false;
	if (write && !__pkru_allows_write(pkru, pkey))
		return false;

	return true;
}

/*
 * 'pteval' can come from a PTE, PMD or PUD.  We only check
 * _PAGE_PRESENT, _PAGE_USER, and _PAGE_RW in here which are the
 * same value on all 3 types.
 */
static inline bool __pte_access_permitted(unsigned long pteval, bool write)
{
	unsigned long need_pte_bits = _PAGE_PRESENT|_PAGE_USER;

	if (write)
		need_pte_bits |= _PAGE_RW;

	if ((pteval & need_pte_bits) != need_pte_bits)
		return 0;

	return __pkru_allows_pkey(pte_flags_pkey(pteval), write);
}

#ifdef CONFIG_PGTABLE_OP_GENERALIZABLE

#define pte_access_permitted(pte, write) \
	gen_pte_read(pte, LEVEL_PTE, PTE_ATTR_ACCESS_PERMITTED, write)
static inline bool native_pte_access_permitted(pte_t pte, bool write)
{
	return __pte_access_permitted(pte_val(pte), write);
}

#define pmd_access_permitted(pmd, write) \
	gen_pte_read(pmd, LEVEL_PMD, PTE_ATTR_ACCESS_PERMITTED, write)
static inline bool native_pmd_access_permitted(pmd_t pmd, bool write)
{
	return __pte_access_permitted(pmd_val(pmd), write);
}

#define pud_access_permitted(pud, write) \
	gen_pte_read(pud, LEVEL_PUD, PTE_ATTR_ACCESS_PERMITTED, write)
static inline bool native_pud_access_permitted(pud_t pud, bool write)
{
	return __pte_access_permitted(pud_val(pud), write);
}


#define native_p4d_access_permitted(p4d, write) \
	(p4d_present(p4d) && (!(write) || p4d_write(p4d)))
#define p4d_access_permitted(p4d, write) \
	gen_pte_read(p4d, LEVEL_P4D, PTE_ATTR_ACCESS_PERMITTED, write)

/* for some reason if we define native_pgd_access_permitted as a macro, 
	pgd_present will not be properly expanded to native_pgd_present  */
#define native_pgd_access_permitted(pgd, write) \
	(native_pgd_present(pgd) && (!(write) || pgd_write(pgd)))

#define pgd_access_permitted(pgd, write) \
	gen_pte_read(pgd, LEVEL_PGD, PTE_ATTR_ACCESS_PERMITTED, write)

#else
#define pte_access_permitted pte_access_permitted
static inline bool pte_access_permitted(pte_t pte, bool write)
{
	return __pte_access_permitted(pte_val(pte), write);
}

#define pmd_access_permitted pmd_access_permitted
static inline bool pmd_access_permitted(pmd_t pmd, bool write)
{
	return __pte_access_permitted(pmd_val(pmd), write);
}

#define pud_access_permitted pud_access_permitted
static inline bool pud_access_permitted(pud_t pud, bool write)
{
	return __pte_access_permitted(pud_val(pud), write);
}
#endif /* CONFIG_PGTABLE_OP_GENERALIZABLE */
#define __HAVE_ARCH_PFN_MODIFY_ALLOWED 1
extern bool pfn_modify_allowed(unsigned long pfn, pgprot_t prot);

static inline bool arch_has_pfn_modify_check(void)
{
	return boot_cpu_has_bug(X86_BUG_L1TF);
}

#define arch_faults_on_old_pte arch_faults_on_old_pte
static inline bool arch_faults_on_old_pte(void)
{
	return false;
}

#endif	/* __ASSEMBLY__ */

#endif /* _ASM_X86_PGTABLE_H */
