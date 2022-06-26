#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klimov Ilya");


#define IRQ 1
static int my_dev_id;
struct tasklet_struct *my_tasklet;

int code;
char *ascii[84] = {
    " ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace", 
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'", "Shift (left)", "|", 
    "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)", 
    "*", "Alt", "Space", "CapsLock", 
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"
};

    
void tasklet_handler(unsigned long data) 
{
    if (code < 84)
		printk(KERN_DEBUG "TASKLET: function data: %s", ascii[code]);
		
	printk(KERN_DEBUG "TASKLET: state: %ld, count: %d, data: %s", 
	       my_tasklet->state, my_tasklet->count, my_tasklet->data);
}


irqreturn_t interrupt_handler(int irq, void *dev)
{
    if (irq == IRQ)
    {
        printk("TASKLET: interrupt_handler is called\n");
        printk("TASKLET: state before planning: %lu\n", my_tasklet->state);
        code = inb(0x60);
        tasklet_schedule(my_tasklet);
        printk("TASKLET: state after  planning: %lu\n", my_tasklet->state);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}


static int __init my_tasklet_init(void)
{
    my_tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    if (!my_tasklet)
    {
        printk(KERN_ERR "TASKLET: kmalloc error");
        return -1;
    }
    
    char *my_tasklet_data = "tasklet function is called";
    tasklet_init(my_tasklet, tasklet_handler, (unsigned long) my_tasklet_data);

    int ret = request_irq(IRQ, interrupt_handler, IRQF_SHARED, "tasklet", &my_dev_id);
    if (ret)
        printk(KERN_ERR "TASKLET: unable to register tasklet_handler\n");
    else
	    printk(KERN_DEBUG "TASKLET: module is loaded.\n");

    return ret;
}


static void __exit my_tasklet_exit(void)
{
    synchronize_irq(IRQ);
    tasklet_kill(my_tasklet);
	
    kfree(my_tasklet);
    free_irq(IRQ, &my_dev_id);

    printk(KERN_DEBUG "TASKLET: module is unloaded.\n");
}


module_init(my_tasklet_init) 
module_exit(my_tasklet_exit)
