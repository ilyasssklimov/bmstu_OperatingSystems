#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klimov Ilya");


#define IRQ 1
static int my_dev_id;

struct workqueue_struct *workqueue;


void queue_function_1(struct work_struct *work)
{
	msleep(10);
	printk(KERN_DEBUG "WORK: first function data = %d\n", work->data);
}


void queue_function_2(struct work_struct *work)
{
    int code = inb(0x60);
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

    if (code < 84)
		printk(KERN_DEBUG "WORK: second function data: %s", ascii[code]);
}


struct work_struct work_1;
struct work_struct work_2;


irqreturn_t interrupt_handler(int irq, void *dev)
{
    if (irq == IRQ)
    {
        printk("WORK: interrupt_handler is called");
        queue_work(workqueue, &work_1);
        queue_work(workqueue, &work_2);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}


static int __init work_queue_init(void)
{
    int ret = request_irq(IRQ, interrupt_handler, IRQF_SHARED, "workqueue", &my_dev_id);
					
    if (ret)
    {
        printk(KERN_ERR "WORK: interrupt_handler wasn't registered\n");
        return ret;
    }

    if (!(workqueue = create_workqueue("my_queue")))
    {
        free_irq(IRQ, &my_dev_id);
        printk(KERN_DEBUG "WORK: workqueue wasn't created");
        return -ENOMEM;
    }

    INIT_WORK(&work_1, queue_function_1);
    INIT_WORK(&work_2, queue_function_2);

	printk(KERN_DEBUG "WORK: module is loaded.\n");
    return 0;
}


static void __exit work_queue_exit(void)
{
    flush_workqueue(workqueue);
    destroy_workqueue(workqueue);
    free_irq(IRQ, &my_dev_id);
	
	printk(KERN_DEBUG "WORK: module is unloaded.\n");;
}


module_init(work_queue_init) 
module_exit(work_queue_exit)