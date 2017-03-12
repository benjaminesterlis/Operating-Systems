#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "kci.h"

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/delay.h>


#define BUF_SIZE (PAGE_SIZE << 2)

/**
FOR UNDERSTANDING
http://www.kneuro.net/cgi-bin/lxr/http/source/include/asm-i386/system.h?a=sparc#L104
https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html

#define read_cr0() ({ \
        unsigned int __dummy; \
        __asm__( \
                "movl %%cr0,%0\n\t"  // write long to register cr0 \
                :"=r" (__dummy)); \
        __dummy; \
})
#define write_cr0(x) \
      __asm__("movl %0,%%cr0": :"r" (x));

**/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benjamin Esterlis <beni.esterlis@gmail.com>");

static struct dentry *file;
static struct dentry *subdir;

unsigned long **sys_call_table;
unsigned long original_cr0;

static int pid = -1;
static int glob_fd = -1;
static int encryption_flag = 0;

static void fill_buff_log(int, int, int, int, int);

asmlinkage long (*ref_read)(int fd, char __user *pathname, umode_t mode);
asmlinkage long (*ref_write)(int fd, char __user *pathname, umode_t mode);

asmlinkage long new_write(int fd, 
				char __user * buffer, size_t count)
{
    int i, j;
    // the call should remain as-is????
    if (fd != glob_fd || current->pid != pid || encryption_flag == 0)
    {
        return ref_write(fd, buffer, count);
    }
    //printk("a=%c\n", (char)buffer[0]);
    for (j = 0; j < count; j+=1)
    {
    	*(buffer + j) += 1;
    	// get_user(Message[j],buffer + j);
    	// Message[j] += 1;
    }
    i = ref_write(fd, buffer, count);
    for (j = 0; j < count; j+=1)
    {
    	*(buffer + j) -= 1;
    	// get_user(Message[j],buffer + j);
    	// Message[j] += 1;
    }
    if(i < 0)
    {
    	return i;
    }
   	fill_buff_log(fd, pid, count, i, 0);
    //printk("device_write(%d,%d)\n", fd, i);
 
    /* return the number of input characters used */
    return i;
}

asmlinkage long new_read(int fd,
                char __user * buffer, size_t count)
{
    int i, j;
    // the call should remain as-is????
    if (fd != glob_fd || current->pid != pid || encryption_flag == 0)
    {
        return ref_read(fd, buffer, count);
    }
    if((i = ref_read(fd, buffer, count)) < 0)
    	return i;
    for (j = 0; j < count; j+=1)
    {
    	*(buffer + j) -= 1;
    	// Message[j] -= 1;
    	// put_user(Message[j],buffer + j);
    }
	fill_buff_log(fd, pid, count, i, 1);
    //printk("device_read(%d,%d)\n", fd, i);
 
    /* return the number of input characters used */
    return i;
}

static 
long device_ioctl(struct file*   file, 
                unsigned int ioctl_num, unsigned long ioctl_param)
{
    /* Switch according to the ioctl called */
    if(IOCTL_CHIPER == ioctl_num) 
    {
        /* Get the parameter given to ioctl by the process */
        printk("kci_kmod, ioctl: setting encryption flag to %ld\n", ioctl_param);
        encryption_flag = ioctl_param;
    }
    else if (IOCTL_SET_PID == ioctl_num)
    {
        printk("kci_kmod, ioctl: setting pid flag to %ld", ioctl_param);
        pid = ioctl_param;
    }
    else if (IOCTL_SET_FD == ioctl_num)
    {
        printk("kci_kmod, ioctl: setting file descriptor to %ld", ioctl_param);
        glob_fd = ioctl_param;
    }

    return SUCCESS;
}

static unsigned long **aquire_sys_call_table(void)
{
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;

    while (offset < ULLONG_MAX) {
        sct = (unsigned long **)offset;

        if (sct[__NR_close] == (unsigned long *) sys_close) 
            return sct;

        offset += sizeof(void *);
    }
    return NULL;
}

struct file_operations Fops = {
    .unlocked_ioctl= device_ioctl,
};

static size_t buf_pos;
static char buf[BUF_SIZE] = {0};
static char str_buf[BUF_SIZE] ={0};

static void fill_buff_log(int fd, int pid, int count, int i, int read)
{
	int len;
	if (read)
	{
		sprintf(str_buf, "log - pid:%d, fd:%d, bytes to read:%d, successfully read:%d\n",
				pid, fd, count, i);
	} 
	else 
	{
		sprintf(str_buf, "log - pid:%d, fd:%d, bytes to write:%d, successfully wrote:%d\n", 
                pid, fd, count, i);
	}
	len = strlen(str_buf);
	if (buf_pos + len <= BUF_SIZE)
	{
		strncpy(buf + buf_pos, str_buf, len);
		buf_pos += len;
	}
	else
	{
		printk(KERN_WARNING "Log: cant write to the log, buffer full\n");
	}
	
	
}

static ssize_t file_read(struct file *filp, char *buffer, size_t len, loff_t *offset)
{
	return simple_read_from_buffer(buffer, len, offset, buf, buf_pos);
}

struct file_operations DBG_Fops = {
	.read= file_read,
};

// http://www.gilgalab.com.br/hacking/programming/linux/2013/01/11/Hooking-Linux-3-syscalls/
static int __init rootkit_init(void)
{
    printk("Starting kernel module: kcikmod\n");    
    
    if(!(sys_call_table = aquire_sys_call_table()))
    {
        printk(KERN_ALERT "can't aquire_sys_call_table\n");
        return -1;
    }

    printk("Intercepting syscall: read/write\n");
    
    original_cr0 = read_cr0();
    // https://en.wikipedia.org/wiki/Control_register
    write_cr0(original_cr0 & (~0x00010000));  //  ~CR0_WP 
    // When set, the CPU can't write to read-only pages when privilege level is 0
    
    ref_write = (void *)sys_call_table[__NR_write];
    sys_call_table[__NR_write] = (unsigned long *)new_write;
    ref_read = (void *)sys_call_table[__NR_read];
    sys_call_table[__NR_read] = (unsigned long *)new_read;

    write_cr0(original_cr0);

    if (register_chrdev(MAJOR_NUM, DEVICE_R_NAME, &Fops) < 0)
    {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", MAJOR_NUM);
        return -1;
    }
    
    printk("Chrdev has been registerd\n");
    subdir = debugfs_create_dir("kcikmod", NULL);
    if (IS_ERR(subdir))
        return PTR_ERR(subdir);
    if (!subdir)
        return -ENOENT;

    file = debugfs_create_file("calls", 0600, subdir, NULL, &DBG_Fops);
    if (!file) {
        debugfs_remove_recursive(subdir);
        return -ENOENT;
    }

    printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
    printk("If you want to talk to the device driver, use the kci controller\n");
    printk("Dont forget to rm the device file and rmmod using the controller\n");
    
    return SUCCESS;
}

static void __exit rootkit_exit(void)
{
    if(!sys_call_table) {
        return;
    }

    printk("Restoring syscalls: read/write\n");
    write_cr0(original_cr0 & ~0x00010000);  //  ~CR0_WP

    sys_call_table[__NR_write] = (unsigned long *)ref_write;
    sys_call_table[__NR_read] = (unsigned long *)ref_read;
    
    printk("syscalls has been restord\n");
    write_cr0(original_cr0);

    unregister_chrdev(MAJOR_NUM, DEVICE_R_NAME);
    
    debugfs_remove_recursive(subdir);

    printk("module has been removed\n");
    msleep(2000);
}

module_init(rootkit_init);
module_exit(rootkit_exit);
