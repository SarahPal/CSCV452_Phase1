#define DEBUG 1

typedef struct proc_struct proc_struct;

typedef struct proc_struct * proc_ptr;

struct proc_struct {
   proc_ptr       next_proc_ptr;
   proc_ptr       child_proc_ptr;
   proc_ptr       next_sibling_ptr;
   int            parent_pid;
   proc_ptr       quit_child_ptr;
   char           name[MAXNAME];     /* process's name */
   char           start_arg[MAXARG]; /* args passed to process */
   context        state;             /* current context for process */
   short          pid;               /* process id */
   int            priority;
   int (* start_func) (char *);   /* function where process begins -- launch */
   char          *stack;
   unsigned int   stacksize;
   int            status;         /* READY, BLOCKED, QUIT, etc. */
   int            num_child;   /* The number of children the process has */
   //int            return_status;
   int            cur_startTime;
   int            CPUTime;
   int            quit_code;

   /* other fields as needed... */
};

struct psr_bits {
        unsigned int cur_mode:1;
       unsigned int cur_int_enable:1;
        unsigned int prev_mode:1;
        unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
   struct psr_bits bits;
   unsigned int integer_part;
};

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY

#define READY   1                                                   /* starting status of processes */
#define RUNNING 2
#define BLOCKED 3                                                   /* status of blocked process */
#define ZAPPED  4                                                   /* status of zapped process, not yet quit */
#define QUIT    5
#define FINISHED 6
#define UNUSED -1
#define TIMESLICE 80000

#define TRUE    1
#define FALSE   0
//#define LOWEST_PRIORITY 0
