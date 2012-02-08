#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#define MODULE_VERS "1.0"
#define MODULE_NAME "lfprng"

static struct proc_dir_entry *lfprng_file;

static char mydata[1000];


// data structure stuff
struct lf_thread {
  int tid;
  int random_last;
};
struct lf_process {
  // 'a' in the equation
  int mult_n;
  // array of threads
  struct lf_thread **threads;
  int num_threads;
  // next process in list
  struct lf_process *next;
};
static struct lf_process *head;
DEFINE_MUTEX(process_mutex);

static void add_process(struct lf_process *new_process) {
  int i;
  mutex_lock(&process_mutex);
  new_process->next = head;
  head = new_process;
  
  for (i = 0; i < new_process->num_threads; i++) {
    printk("2 - Adding thread tid %d`\n", new_process->threads[i]->tid);
  }
  mutex_unlock(&process_mutex);
}

static struct lf_thread* find_thread(struct lf_process* process, int tid) {
  struct lf_thread* ret = NULL;
  int i;
  for (i = 0; i < process->num_threads; i++) {
    if (process->threads[i]-> tid == tid) {
      ret = process->threads[i];
    }
  }
  return ret;
}


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
  //strcpy(page, data);
  *eof = 1;
  //len = strlen(data) + 1;
  if (find_thread(head, current->pid) || 1)
    len = sprintf(page, "Name:%s pid:%d tgid:%d mynumthreads:%d\n", current->comm, current->pid, current->tgid, head->num_threads);
  else
    len = sprintf(page, "didn't work");
  return len;
}

static int proc_write_lfprng(struct file *file,
                             const char *buffer,
                             unsigned long count,
                             void *data)
{
  int len;
  int num_threads;
  int n;
  struct lf_process *new_process;
  struct list_head *pos, *siblings;
  new_process = vmalloc(sizeof(new_process));
  printk(KERN_INFO "writing\n");
  len = count;
  
  num_threads = 0; //current->group_leader->signal->nr_threads; //get_nr_threads(current->group_leader);
  
  siblings = &(current->thread_group);//&current->sibling; // maybe use current->sibling
  list_for_each(pos, siblings) {
    num_threads++;
  }
  new_process->threads = vmalloc(num_threads * sizeof(struct lf_thread *));
  new_process->num_threads = num_threads;

  n = 0;
  pos = NULL;
  list_for_each(pos, siblings) {
    struct lf_thread *new_thread = vmalloc(sizeof(struct lf_thread));
    struct task_struct *thread = list_entry(pos, struct task_struct, thread_group);
    new_thread->tid = thread->pid;
    new_thread->random_last = n;
    (new_process->threads)[n] = new_thread;
    n++;
    printk("Adding thread tid %d\n", new_thread->tid);
  }
  
  add_process(new_process);

  /*if (count >= 1000)
    len = 999;
  else
    len = count;

  if (copy_from_user(data, buffer, len))
    return -EFAULT;

  ((char*)data)[len] = '\0'; */

  

  return len;
}

static int __init lfprng_start(void)
{
  printk(KERN_INFO "Loading lfprng module...\n");
  
  //head = vmalloc(sizeof(struct lf_process));
  head = NULL;

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

