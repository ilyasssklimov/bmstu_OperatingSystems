#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klimov Ilya");


#define MAX_BUF_SIZE PAGE_SIZE
#define DIRNAME "seqfile_dir"
#define FILENAME "seqfile"
#define SYMLINK "seqfile_ln"
#define FILEPATH DIRNAME "/" FILENAME


static struct proc_dir_entry *fortune_dir = NULL;
static struct proc_dir_entry *fortune = NULL;
static struct proc_dir_entry *fortune_ln = NULL;

static char *cookie_buffer;
static int next_index;
static int current_fortune;

static char tmp[MAX_BUF_SIZE];


ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *offp);
int open(struct inode *inode, struct file *file);
int release(struct inode *inode, struct file *file);



static struct proc_ops fops = 
{
    .proc_read = seq_read,
    .proc_write = write,
    .proc_open = open,
    .proc_release = release
};


int show(struct seq_file *m, void *v)
{
    int len;

    if (!next_index)
        return 0;

    printk(KERN_INFO "show called in sequence\n");
    
    len = snprintf(tmp, MAX_BUF_SIZE, "%s", &cookie_buffer[current_fortune]);
    seq_printf(m, "%s", tmp);
    current_fortune += len;

    return 0;
}


ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *offp)
{
    if (len > MAX_BUF_SIZE - next_index + 1)
    {
        printk(KERN_ERR "cookie_buffer overflow error in sequence\n");
        return -ENOSPC;
    }

    if (copy_from_user(&cookie_buffer[next_index], buf, len))
    {
        printk(KERN_ERR "copy_to_user error in sequence\n");
        return -EFAULT;
    }

    printk(KERN_INFO "write called in sequence\n");

    next_index += len - 1;
    cookie_buffer[next_index] = '\n';

    return len;
}


int open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "open called in seqauence\n");
    return single_open(file, show, NULL);
}


int release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "release called in sequence\n");
    return single_release(inode, file);
}


static void free_memory(void)
{
    if (fortune_ln)
        remove_proc_entry(SYMLINK, NULL);

    if (fortune)
        remove_proc_entry(FILENAME, fortune_dir);

    if (fortune_dir)
        remove_proc_entry(DIRNAME, NULL);

    if (cookie_buffer)
        vfree(cookie_buffer);
}


static int __init fortune_init(void)
{
    if (!(cookie_buffer = vmalloc(MAX_BUF_SIZE)))
    {
        free_memory();
        printk(KERN_ERR "vmalloc error in sequence\n");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, MAX_BUF_SIZE);

    if (!(fortune_dir = proc_mkdir(DIRNAME, NULL)))
    {
        free_memory();
        printk(KERN_ERR "Directory creation error in sequence\n");
        return -ENOMEM;
    }
    else if (!(fortune = proc_create(FILENAME, 0666, fortune_dir, &fops)))
    {
        free_memory();
        printk(KERN_ERR "File creation error in sequence\n");
        return -ENOMEM;
    }
    else if (!(fortune_ln = proc_symlink(SYMLINK, NULL, FILEPATH)))
    {
        free_memory();
        printk(KERN_ERR "Symlink creation error in sequence\n");
        return -ENOMEM;
    }

    next_index = 0;
    current_fortune = 0;

    printk(KERN_INFO "sequence: module loaded\n");

    return 0;
}


static void __exit fortune_exit(void)
{
    free_memory();
    printk(KERN_INFO "sequence: module unloaded\n");
}


module_init(fortune_init)
module_exit(fortune_exit)