// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 1994-1999  Linus Torvalds
 */

#include <linux/fs.h>
#include <linux/mm_types.h>

#ifdef BPM_PAGECACHE_WRITE_BEGIN_AND_END_NOT_PRESENT

int pagecache_write_begin(struct file *file, struct address_space *mapping,
                                loff_t pos, unsigned len, unsigned flags,
                                struct page **pagep, void **fsdata)
{
        const struct address_space_operations *aops = mapping->a_ops;

        return aops->write_begin(file, mapping, pos, len,
                                 pagep, fsdata);
}
EXPORT_SYMBOL(pagecache_write_begin);

int pagecache_write_end(struct file *file, struct address_space *mapping,
                                loff_t pos, unsigned len, unsigned copied,
                                struct page *page, void *fsdata)
{
        const struct address_space_operations *aops = mapping->a_ops;

        return aops->write_end(file, mapping, pos, len, copied, page, fsdata);
}
EXPORT_SYMBOL(pagecache_write_end);

#endif
