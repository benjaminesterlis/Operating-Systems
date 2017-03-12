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
#include <linux/debugfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benjamin Esterlis <beni.esterlis@gmail.com>");

static struct dentry *file;
static struct dentry *subdir;

static char Message[BUF_LEN];

static int pid = -1;
static int glob_fd = -1;
static int encryption_flag = 0;

static ssize_t device_write(int fd,
         		const char __user * buffer, size_t count)
{
	int i;
	// the call should remain as-is????
	// 
	if (fd != glob_fd || current->pid != pid)
	{
		return -1;
	}
	//???????????????????????????????????? - TODO
  	for (i = 0; i < count && i < BUF_LEN; i++)
  	{
    	get_user(Message[i], buffer + i);
		if(1 == encryption_flag)
      		Message[i] += 1;
  	}
  	pr_debug("log - pid:%d, fd:%d, bytes to write:%ld, successfully wrote:%d", 
  					pid, fd, count, i);
  	printk("device_write(%d,%d)\n", fd, i);
 
  	/* return the number of input characters used */
  	return i;
}

static ssize_t device_read(int fd,
         		char __user * buffer, size_t count)
{
	int i;
	// the call should remain as-is????
	// 
	if (fd != glob_fd || current->pid != pid)
	{
		return -1;
	}
	//???????????????????????????????????? - TODO
  	for (i = 0; i < count && i < BUF_LEN; i++)
  	{
		if(1 == encryption_flag)
      		Message[i] -= 1;
    	put_user(Message[i], buffer + i);
  	}
  	pr_debug("log - pid:%d, fd:%d, bytes to read:%ld, successfully read:%d", 
  					pid, fd, count, i);
  	printk("device_read(%d,%d)\n", fd, i);
 
  	/* return the number of input characters used */
  	return i;
}

static long device_ioctl(struct file*   file, 
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


struct file_operations Fops = {
    // .read = device_read,
    // .write = device_write,
    .unlocked_ioctl= device_ioctl,
    // .open = device_open, //need to chack if necessary
    // .release = device_release,  
};


static int __init kcimod_init(void)
{
	unsigned int rc = 0;
	if ((rc = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &Fops)) < 0)
	{
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", MAJOR_NUM);
        return -1;
	}

	subdir = debugfs_create_dir("kcikmod", NULL);
	if (IS_ERR(subdir))
		return PTR_ERR(subdir);
	if (!subdir)
		return -ENOENT;

	file = debugfs_create_file("calls", 0600, subdir, NULL, &Fops);
	if (!file) {
		debugfs_remove_recursive(subdir);
		return -ENOENT;
	}

	printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
    printk("If you want to talk to the device driver,\n");
    printk("use the kci controller\n");
    printk("Dont forget to rm the device file and rmmod using the controller\n");

    return 0;
}

static void __exit kcimod_exit(void)
{
	unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
	// if (ret < 0)
	// 	printk(KERN_ALERT "Error in unregister_chrdev:%d\n",ret);
	debugfs_remove_recursive(subdir);
}

module_init(kcimod_init);
module_exit(kcimod_exit);