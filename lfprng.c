#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#define MODULE_VERS "1.0"
#define MODULE_NAME "lfprng"

static unsigned long long MULTIPLIER = 764261123;
static unsigned long long PMOD = 2147483647;

static struct proc_dir_entry *lfprng_file;

static char mydata[1000];


// data structure stuff
struct lf_thread {
    int tid;
    unsigned long long random_last;
};
struct lf_process {
    int pid;
    // 'a' in the equation
    int mult_n;
    double random_low, random_hi;
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

static struct lf_process* find_process(int pid) {
    struct lf_process* ret = head;
    while (ret != NULL && ret->pid != pid) {
        ret = ret->next;
    }
    return ret;
}

static struct lf_thread* find_thread(struct lf_process* process, int tid) {
    //struct lf_thread* ret = NULL;
    int i;
    for (i = 0; i < process->num_threads; i++) {
        if (process->threads[i]-> tid == tid) {
            return process->threads[i];
        }
    }
    return NULL;
}

/*static unsigned long long offset_luu(unsigned long long n, off_t off) {
 int mod = 1;
 int i = n;
 int num_digits = 0;
 while (n >= 10) {
 num_digits++;
 i /= 10;
 }
 num_digits++;
 // pow(10, num_digits - off)
 for (i = 10; i < num_digits-off; i++) {
 mod *= 10;
 }
 return n % mod;
 //for (i = 0; i < off; i
 }*/

static void seed(unsigned long long iseed, double random_low, double random_hi, int count_parent) {
    int num_threads = 0;
    struct lf_process *new_process;
    struct list_head *pos, *siblings;
    
    new_process = vmalloc(sizeof(struct lf_process));
    
    siblings = &(current->thread_group);//&current->sibling; // maybe use current->sibling
    list_for_each(pos, siblings) {
        num_threads++;
    }
    new_process->threads = vmalloc(num_threads * sizeof(struct lf_thread *));
    new_process->num_threads = num_threads;
    new_process->pid = current->tgid;
    
    n = 0;
    pos = NULL;
    new_process->mult_n = MULTIPLIER;
    //iseed = 0;
    list_for_each(pos, siblings) {
        struct lf_thread *new_thread = vmalloc(sizeof(struct lf_thread));
        struct task_struct *thread = list_entry(pos, struct task_struct, thread_group);
        if (n != 0) {
            iseed = (unsigned long long)((MULTIPLIER * iseed) % PMOD);
        }
        new_process->mult_n = (new_process->mult_n * MULTIPLIER) % PMOD;
        new_thread->tid = thread->pid;
        new_thread->random_last = iseed;
        (new_process->threads)[n] = new_thread;
        n++;
        printk("Adding thread tid %d seed:%llu iseed:%llu\n", new_thread->tid, new_thread->random_last, iseed);
    }
    
    add_process(new_process);
}

static int proc_read_lfprng(char *page, char **start,
                            off_t off, int count,
                            int *eof, void *data)
{
    int len = 0;
    //unsigned long long random_next;
    struct lf_thread* thread;
    struct lf_process* process;
    char temp_string[1000];
    
    printk(KERN_INFO "reading pid:%d tid:%d\n", current->tgid, current->pid);
    /*if (data == NULL) {
     strcpy(page,"null\n");
     return strlen("null\n") + 1;
     }*/
    //strcpy(page, data);
    *eof = 1;
    //len = strlen(data) + 1;
    /*if (find_thread(head, current->pid) || 1)
     len = sprintf(page, "Name:%s pid:%d tgid:%d mynumthreads:%d\n", current->comm, current->pid, current->tgid, head->num_threads);
     else
     len = sprintf(page, "didn't work");
     */
    process = find_process(current->tgid);
    thread = find_thread(process, current->pid);
    if (process == NULL || thread == NULL)
        return 0;
    if (off == 0)
        thread->random_last = (unsigned long long)((process->mult_n * thread->random_last) % PMOD);
    double double_val = (double)(thread->random_last);
    printk("tid: %d -outputting: llu:%llu d:%f\n", current->pid, thread->random_last, double_val);
    //len = sprintf(temp_string, "module prints %llu", thread->random_last
    len = sprintf(page, "module prints %llu", thread->random_last);
    
    return len;
}




static int proc_write_lfprng(struct file *file,
                             const char *buffer,
                             unsigned long count,
                             void *data)
{
    int len;
    //int num_threads;
    int n;
    unsigned long long iseed = 0;
    double random_low, random_hi;
    //struct lf_process *new_process;
    //struct list_head *pos, *siblings;
    char temp[1000];
    int count_parent = 0;
    //new_process = vmalloc(sizeof(new_process));
    printk(KERN_INFO "writing\n");
    len = count;
    if (len > 999) len = 999;
    
    if (copy_from_user(temp, buffer, len))
        return -EFAULT;
    
    ((char*)temp)[len] = '\0'; 
    
    sscanf((const char*)temp, "%llu %f %f %d", &iseed, &random_low, &random_hi, &count_parent);
    printk("read iseed: %llu\n", iseed); 
    //num_threads = 0; //current->group_leader->signal->nr_threads; //get_nr_threads(current->group_leader);
    
    seed(iseed, random_low, random_hi, count_parent);
    
    return len;
}

static int __init lfprng_start(void)
{
    printk(KERN_INFO "Loading lfprng module...\n");
    
    //head = vmalloc(sizeof(struct lf_process));
    head = NULL;
    
    //strcpy(mydata, "just initialized\n");
    
    printk(KERN_INFO "%s\n", mydata);
    
    lfprng_file = create_proc_entry("lfprng", 0666, NULL);
    
    //lfprng_file->data = &mydata;
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

