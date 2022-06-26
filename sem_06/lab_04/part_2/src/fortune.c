#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klimov Ilya");


#define DIRNAME "fortune_dir"
#define FILENAME "fortune"
#define SYMLINK "fortune_ln"
#define FILEPATH DIRNAME "/" FILENAME


static struct proc_dir_entry *fortune_dir = NULL;
static struct proc_dir_entry *fortune = NULL;
static struct proc_dir_entry *fortune_ln = NULL;
static char *cookie_buffer;
static int next_index;
static int current_fortune;
static char tmp[PAGE_SIZE];


ssize_t read(struct file*, char __user*, size_t count, loff_t *);
ssize_t write(struct file*, const char __user*, size_t len, loff_t *);
int open(struct inode*, struct file*);
int release(struct inode*, struct file*);


static struct proc_ops fops = {
    .proc_read = read,
    .proc_write = write,
    .proc_open = open,
    .proc_release = release
};


ssize_t read(struct file *file, char __user *buf, size_t count, loff_t *offp)
{
    int len;

    printk(KERN_INFO "read called in fortune\n");

    if (*offp > 0 || !next_index)
        return 0;

    if (current_fortune >= next_index)
        current_fortune = 0;

    len = snprintf(tmp, PAGE_SIZE, "%s\n", &cookie_buffer[current_fortune]);
    if (copy_to_user(buf, tmp, len))
    {
        printk(KERN_ERR "copy_to_user error in fortune\n");
        return -EFAULT;
    }

    current_fortune += len;
    *offp += len;

    return len;
}


ssize_t write(struct file *file, const char __user *buf, size_t len, loff_t *offp)
{
    printk(KERN_INFO "write called in fortune\n");
    
    if (len > PAGE_SIZE - next_index + 1)
    {
        printk(KERN_ERR "cookie_buffer overflow in fortune\n");
        return -ENOSPC;
    }

    if (copy_from_user(&cookie_buffer[next_index], buf, len))
    {
        printk(KERN_ERR "copy_from_user error in fortune\n");
        return -EFAULT;
    }

    next_index += len;
    cookie_buffer[next_index - 1] = '\0';

    return len;
}


int open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "open called in fortune\n");
    return 0;
}


int release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "release called in fortune\n");
    return 0;
}


void free_memory(void)
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
    if (!(cookie_buffer = vmalloc(PAGE_SIZE)))
    {
        free_memory();
        printk(KERN_ERR "vmalloc error in fortune\n");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, PAGE_SIZE);

    if (!(fortune_dir = proc_mkdir(DIRNAME, NULL)))
    {
        free_memory();
        printk(KERN_ERR "Directory creation error in fortune\n");
        return -ENOMEM;
    }

    if (!(fortune = proc_create(FILENAME, 0666, fortune_dir, &fops)))
    {
        free_memory();
        printk(KERN_ERR "File creation error in fortune\n");
        return -ENOMEM;
    }

    if (!(fortune_ln = proc_symlink(SYMLINK, NULL, FILEPATH)))
    {
        free_memory();
        printk(KERN_ERR "Symlink creation error in fortune\n");
        return -ENOMEM;
    }

    next_index = 0;
    current_fortune = 0;

    printk(KERN_INFO "fortune: module loaded\n");

    return 0;
}


static void __exit fortune_exit(void)
{
    free_memory();
    printk(KERN_INFO "fortune: module unloaded\n");
}


module_init(fortune_init)
module_exit(fortune_exit)