#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MODULE_VERS "1.0"
#define MODULE_NAME "lfprng"

static struct proc_dir_entry *lfprng_file;

static char mydata[1000];


static int proc_read_lfprng(char *page, char **start,
                            off_t off, int count,
                            int *eof, void *data)
{
  int len = 0;
  printk(KERN_INFO "reading\n");
  if (data == NULL) {
    strcpy(page,"null\n");
    return strlen("null\n") + 1;
  }
  strcpy(page, data);
  *eof = 1;
  len = strlen(data) + 1;
  return len;
}

static int proc_write_lfprng(struct file *file,
                             const char *buffer,
                             unsigned long count,
                             void *data)
{
  int len;
  printk(KERN_INFO "writing\n");

  if (count >= 1000)
    len = 999;
  else
    len = count;

  if (copy_from_user(data, buffer, len))
    return -EFAULT;

  ((char*)data)[len] = '\0';

  return len;
}

static int __init lfprng_start(void)
{
  printk(KERN_INFO "Loading lfprng module...\n");

  strcpy(mydata, "just initialized\n");

  printk(KERN_INFO "%s\n", mydata);

  lfprng_file = create_proc_entry("lfprng", 0666, NULL);
  
  lfprng_file->data = &mydata;
  lfprng_file->read_proc = proc_read_lfprng;
  lfprng_file->write_proc = proc_write_lfprng;
  lfprng_file->owner = THIS_MODULE;

  printk(KERN_INFO "Hello world lfprng\n");
  return 0;
}
static void __exit lfprng_end(void)
{
  remove_proc_entry("lfprng", NULL);
  printk(KERN_INFO "Goodbye lfprng.\n");
}
module_init(lfprng_start);
module_exit(lfprng_end);

MODULE_DESCRIPTION("LeapFrog Psuedorandom Number Generator");

