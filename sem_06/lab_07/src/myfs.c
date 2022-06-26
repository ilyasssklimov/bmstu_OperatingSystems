#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>


#define MYFS_MAGIC 0xBADCAFE
#define MAX_CACHE_SIZE 128
#define SLAB_NAME "myfs_slab"
#define OK 0


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klimov Ilya");


struct myfs_inode 
{
    int           i_mode;
    unsigned long i_ino;
};


// cache
static struct kmem_cache *inode_cache = NULL;
static struct myfs_inode **inode_pointers = NULL;
static int cached_count = 0;


static struct myfs_inode *cache_get_inode(void)
{
    if (cached_count == MAX_CACHE_SIZE)
        return NULL;

    return inode_pointers[cached_count++] = kmem_cache_alloc(inode_cache, GFP_KERNEL);
}


static struct inode *myfs_make_inode(struct super_block *sb, int mode)
{
    struct inode *ret = new_inode(sb);
    struct myfs_inode *inode_cache = NULL;

    if (ret) 
    {
        inode_init_owner(&init_user_ns, ret, NULL, mode);

        ret->i_size = PAGE_SIZE;
        ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);

        if ((inode_cache = cache_get_inode()) != NULL)
        {
            inode_cache->i_mode = ret->i_mode;
            inode_cache->i_ino = ret->i_ino;
        }

        ret->i_private = inode_cache;
    }
    return ret;
}


static void myfs_put_super(struct super_block *sb)
{
    printk(KERN_DEBUG "MYFS: putsuper\n");
}


static struct super_operations const myfs_super_ops =
{
    .put_super = myfs_put_super,
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};


static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC;
    sb->s_op = &myfs_super_ops;
    struct inode *root = myfs_make_inode(sb, S_IFDIR | 0755);

    if (!root)
    {
        printk(KERN_ERR "MYFS: inode allocation failed...\n");
        return -ENOMEM;
    }

    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;
    root->i_ino = 1;

    if (!(sb->s_root = d_make_root(root)))
    {
        printk(KERN_ERR "MYFS: root creation failed\n");
        iput(root);

        return -ENOMEM;
    }

    return OK;
}


static struct dentry *myfs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
    struct dentry *const entry = mount_nodev(type, flags, data, myfs_fill_sb);

    if (IS_ERR(entry))
        printk(KERN_ERR "MYFS: mounting failed\n");
    else
        printk(KERN_DEBUG "MYFS: mounted\n");

    return entry;
}


static void myfs_kill_sb(struct super_block *sb)
{
	printk(KERN_DEBUG "MYFS: kill super block\n");
	kill_anon_super(sb);
}


static struct file_system_type myfs_type =
{
	.owner = THIS_MODULE,
	.name = "myfs",
	.mount = myfs_mount,
	.kill_sb = myfs_kill_sb
};


static int __init myfs_init(void)
{
    int ret = register_filesystem(&myfs_type);

    if (ret != 0)
    {
        printk(KERN_ERR "MYFS: Failed to register filesystem\n");
        return ret;
    }

    if ((inode_pointers = kmalloc(sizeof(struct myfs_inode *) * MAX_CACHE_SIZE, GFP_KERNEL)) == NULL)
    {
        printk(KERN_ERR "MYFS: Failed to allocate memory\n");
        return -ENOMEM;
    }

    if ((inode_cache = kmem_cache_create(SLAB_NAME, sizeof(struct myfs_inode), 0, 0, NULL)) == NULL)
    {
        kfree(inode_pointers);
        printk(KERN_ERR "MYFS: Failed to create cache\n");

        return -ENOMEM;
    }

    printk(KERN_DEBUG "MYFS: module loaded\n");
    return OK;
}


static void __exit myfs_exit(void)
{
    int i;
    for (i = 0; i < cached_count; i++)
        kmem_cache_free(inode_cache, inode_pointers[i]);

    kmem_cache_destroy(inode_cache);
    kfree(inode_pointers);

    if (unregister_filesystem(&myfs_type) != 0)
        printk(KERN_ERR "MYFS: Can't unregister filesystem\n");
    else
        printk(KERN_DEBUG "MYFS: module unloaded\n");
}


module_init(myfs_init);
module_exit(myfs_exit);
