/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *
 */

#ifndef _BACKPORT_LINUX_SLAB_H
#define _BACKPORT_LINUX_SLAB_H
#include <linux/version.h>
#include_next <linux/slab.h>

/*
 * Since kmem_cache_get_slabinfo() got introduced in KV5.10.0,
 * added check here. May need to change in future.
 */
#if LINUX_VERSION_IS_LESS(5,10,0)


#define slabinfo LINUX_I915_BACKPORT(slabinfo)
struct slabinfo {
       unsigned long active_objs;
       unsigned long num_objs;
       unsigned long active_slabs;
       unsigned long num_slabs;
       unsigned long shared_avail;
       unsigned int limit;
       unsigned int batchcount;
       unsigned int shared;
       unsigned int objects_per_slab;
       unsigned int cache_order;
};

#ifdef CONFIG_SLAB
/*
 * struct array_cache
 *
 * Purpose:
 * - LIFO ordering, to hand out cache-warm objects from _alloc
 * - reduce the number of linked list operations
 * - reduce spinlock operations
 *
 * The limit is stored in the per-cpu structure to reduce the data cache
 * footprint.
 *
 */
#define array_cache LINUX_I915_BACKPORT(array_cache)
struct array_cache {
        unsigned int avail;
        unsigned int limit;
        unsigned int batchcount;
        unsigned int touched;
        void *entry[];  /*
                         * Must have this definition in here for the proper
                         * alignment of array_cache. Also simplifies accessing
                         * the entries.
                         */
};
#endif /* CONFIG_SLAB */

#define kmem_cache_get_slabinfo  LINUX_I915_BACKPORT(kmem_cache_get_slabinfo)

#if defined(CONFIG_SLUB_DEBUG) || defined(CONFIG_SLAB)

int kmem_cache_get_slabinfo(struct kmem_cache *cachep, struct slabinfo *sinfo);

#else
static inline int kmem_cache_get_slabinfo(struct kmem_cache *cachep,
                                       struct slabinfo *sinfo)
{
      return -EINVAL;
}
#endif

#define kmem_cache_node LINUX_I915_BACKPORT(kmem_cache_node)

#ifndef CONFIG_SLOB
/*
 * The slab lists for all objects.
 */
struct kmem_cache_node {
        spinlock_t list_lock;

#ifdef CONFIG_SLAB
        struct list_head slabs_partial; /* partial list first, better asm code */
        struct list_head slabs_full;
        struct list_head slabs_free;
        unsigned long total_slabs;      /* length of all slab lists */
        unsigned long free_slabs;       /* length of free slab list only */
        unsigned long free_objects;
        unsigned int free_limit;
        unsigned int colour_next;       /* Per-node cache coloring */
        struct array_cache *shared;     /* shared per node */
        struct alien_cache **alien;     /* on other nodes */
        unsigned long next_reap;        /* updated without locking */
        int free_touched;               /* updated without locking */
#endif

#ifdef CONFIG_SLUB
        unsigned long nr_partial;
        struct list_head partial;
#ifdef CONFIG_SLUB_DEBUG
        atomic_long_t nr_slabs;
        atomic_long_t total_objects;
        struct list_head full;
#endif
#endif

};

/*
 * Iterator over all nodes. The body will be executed for each node that has
 * a kmem_cache_node structure allocated (which is true for all online nodes)
 */

#define for_each_kmem_cache_node(__s, __node, __n) \
        for (__node = 0; __node < nr_node_ids; __node++) \
                 if ((__n = get_node(__s, __node)))

/* struct memcg_cache_params is deprecated */
#define memcg_cache_params LINUX_I915_BACKPORT(memcg_cache_params)

struct memcg_cache_params {
        struct kmem_cache *root_cache;
        union {
                struct {
                        struct memcg_cache_array __rcu *memcg_caches;
                        struct list_head __root_caches_node;
                        struct list_head children;
                        bool dying;
                };
                struct {
                        struct mem_cgroup *memcg;
                        struct list_head children_node;
                        struct list_head kmem_caches_node;
                        void (*deact_fn)(struct kmem_cache *);
                        union {
                                struct rcu_head deact_rcu_head;
                                struct work_struct deact_work;
                        };
                };
        };
};
#endif /* !CONFIG_SLOB */
#endif /* LINUX_VERSION_IS_LESS(5,10,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0)
#define SLAB_TYPESAFE_BY_RCU    0x00080000UL    /* Defer freeing slabs to RCU */
#endif
#endif