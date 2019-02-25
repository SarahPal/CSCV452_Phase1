/*----------------------------------------------------------------*
 * Sarah Palenik & Duncan Emrie                                   *
 * From Duncan: Special thanks to my dog for trying to help by    *
 *              climbing onto my laptop.                          *
 * CSCV 452                                                       *
 * Phase1                                                         *
 *----------------------------------------------------------------*/


/*================================================================*
 *    Include Statements                                          *
 *================================================================*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <usloss.h>

#include <phase1.h>
#include "kernel.h"


/*================================================================*
 *    Constant Declarations                                       *
 *================================================================*/
#define UNUSED  -1                                                  /* default status */
#define RUNNING 0                                                   /* max of one process can have this status */
#define READY   1                                                   /* starting status of processes */
#define BLOCKED 2                                                   /* status of blocked process */
#define ZAPPED  3                                                   /* status of zapped process, not yet quit */
#define QUIT    4                                                   /* status of quit process not yet joined */

#define TRUE    1
#define FALSE   0

#define DEBUG   1


/*================================================================*
 *    Function Prototypes                                         *
 *================================================================*/
int sentinel(void *);
extern int start1(void *);
void dispatcher(void);
void launch();

static void check_kernel_mode(char *caller_name);
static void enable_interrupts(char* caller_name);
static void disable_interrupts(char* caller_name);

static void check_deadlock();

static void update_lists(char *caller_name);
static void update_list_helper(proc_ptr *b_t, proc_ptr *q_t, proc_ptr *r_t, proc_ptr *z_t, proc_ptr found);

void dump_processes(void);
int get_pid(void);
int readtime(void);

int zap(int pid);
int is_zapped(void);

/*For future functionality */
int block_me(int new_status);
int unblock_proc(int pid);
int read_cur_start_time(void);
void time_slice(void);


/*================================================================*
 *    Global Variable Declarations                                *
 *================================================================*/
/* Patrick's Debugging Global Variable... */
int debugflag = 1;

/* Process Table */
proc_struct ProcTable[MAXPROC];

/* Process Lists  */
proc_ptr BlockedList;                                               /* blocked processes */
proc_ptr QuitList;                                                  /* quit processes, not yet joined */
proc_ptr ReadyList[MINPRIORITY - MAXPRIORITY + 1];                  /* ready processes */
proc_ptr ZappedList;                                                /* zapped processes, not yet quit */

/* Current Process ID */
proc_ptr Current;

/* Next Process ID to be Assigned */
unsigned int next_pid = SENTINELPID;

/*================================================================*
 *    Functions                                                   *
 *================================================================*/


/*----------------------------------------------------------------*
 * Name        : startup                                          *
 * Purpose     : Initializes process lists and clock interrupt    *
 *               vector. Start up sentinel process and the test   *
 *               process.                                         *
 * Parameters  : none, called by USLOSS                           *
 * Returns     : nothing                                          *
 * Side Effects: lots, starts the whole thing                     *
 *----------------------------------------------------------------*/
void startup()
{
   int i;      /* loop index */
   int result; /* value returned by call to fork1() */

 /* initialize the Process Table */
   for (i = 1; i <= MAXPROC; i++)
   {
      ProcTable[i].pid = UNUSED;
      ProcTable[i].status = UNUSED;
      ProcTable[i].next_proc_ptr = NULL;
      ProcTable[i].child_proc_ptr = NULL;
      ProcTable[i].next_sibling_ptr = NULL;
   }

 /* initialize all Process Lists */
   if (DEBUG && debugflag)
      console(" - startup(): initializing the blocked, quit, ready, and zapped lists -\n");
   BlockedList = NULL;
   QuitList = NULL;
   for (i = MAXPRIORITY; i < MINPRIORITY; i++)
      ReadyList[i] = NULL;
   ZappedList = NULL;

 /* initialize the Clock Interrupt Handler */
 //Int_vec[CLOCK_INT] = clockHandler;

 /* startup a sentinel process */
   if (DEBUG && debugflag)
       console(" - startup(): calling fork1() for sentinel -\n");

   result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK, SENTINELPRIORITY);

   console("%d\n", result);
   if (result < 0)
   {
      if (DEBUG && debugflag)
         console(" - startup(): fork1 of sentinel returned error, halting... -\n");
      halt(1);
   }

 /* start the test process */
   if (DEBUG && debugflag)
      console(" - startup(): calling fork1() for start1 -\n");

   result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);

   if (result < 0)
   {
      console(" - startup(): fork1 for start1 returned an error, halting... -\n");
      halt(1);
   }
   else
   {
       console("%d\n", result);
   }
   //console("\nBefore Dispatcher\n");
   dispatcher();

   console(" - startup(): Should not see this message! -\n");
   console(" - Returned from fork1 call that created start1 -\n");

   return;
}/* startup */


/*----------------------------------------------------------------*
 * Name        : fork1                                            *
 * Purpose     : Gets a new process from the process table and    *
 *               initializes information of the process. Updates  *
 *               information in the parent process to reflect     *
 *               this child process creation.                     *
 * Parameters  : the process procedure address, the size of the   *
 *               stack and the priority to be assigned to the     *
 *               child process.                                   *
 * Returns     : the process id of the created child -or-         *
 *               -1 if no child could be created or if priority   *
 *               is not between max and min priority -or-         *
 *               -2 if stack size is too small                    *
 * Side Effects: ReadyList is changed, ProcTable is changed,      *
 *               Current process information changed              *
 *----------------------------------------------------------------*/
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority)
{
   int proc_slot;
   proc_ptr ready_temp;

   if (DEBUG && debugflag)
      console(" - fork1(): creating process %s -\n", name);

 /* test if in kernel mode, halts otherwise */
   check_kernel_mode("fork1");

 /* disbale interrupts */
   disable_interrupts("fork1");

 /* find an empty slot in the process table */
   proc_slot = 1;
   while( (ProcTable[proc_slot].pid !=  -1) && (proc_slot < MAXPROC) )
      proc_slot += 1;

 /* return if stack size is too small */
   if (stacksize < USLOSS_MIN_STACK)
   {
    /* enable interrupts */
      enable_interrupts("fork1");
      console(" - fork1(): stack size is too small -\n");
      return -2;
   }

 /* return if no empty slots in the process table, or priority is
  * out of range, or func is NULL, or name is NULL */
   if (proc_slot >= MAXPROC)                                        /* checks if no empty slots in the process table */
   {
    /* enable interrupts */
      enable_interrupts("fork1");
      console(" - fork1(): process table is full -\n");
      return -1;
   }

   if (strcmp(name, "sentinel") )                                   /* checks if process is sentinel */
   {
      if ( (priority > MINPRIORITY) || (priority < MAXPRIORITY) )   /* if not checks if priority is in range */
      {
       /* enable interrupts */
         enable_interrupts("fork1");
         console(" - fork1(): priority out of range -\n");
         return -1;
      }
   }

   if (func == NULL)                                                /* checks if func NULL */
   {
    /* enable interrupts */
      enable_interrupts("fork1");
      console(" - fork1(): null function passed -\n");
      return -1;
   }

   if (name == NULL)                                                /* checks if process name is NULL */
   {
    /* enable interrupts */
      enable_interrupts("fork1");
      console(" - fork1(): null process name passed -\n");
      return -1;
   }

 /* fill-in entry in process table */
   if (strlen(name) >= (MAXNAME - 1) )                              /* checks if the process name is too long */
   {
      console(" - fork1(): process name is too long.  Halting... -\n");
      halt(1);
   }
   strcpy(ProcTable[proc_slot].name, name);                         /* stores the process name */

   ProcTable[proc_slot].start_func = func;                          /* stores the function to be executed */

   if (arg == NULL)                                                 /* checks if the function argument is empty */
      ProcTable[proc_slot].start_arg[0] = '\0';
   else if (strlen(arg) >= (MAXARG - 1) )                           /* checks if the function argument is too long */
   {
      console(" - fork1(): argument too long.  Halting... -\n");
      halt(1);
   }
   else
      strcpy(ProcTable[proc_slot].start_arg, arg);                  /* stores the function argument */

   ProcTable[proc_slot].stacksize = stacksize;                      /* stores the stack size */

   ProcTable[proc_slot].pid = next_pid;                             /* stores the process id */
   next_pid += 1;                                                   /* increments the next_pid */

   ProcTable[proc_slot].status = READY;                             /* sets the process status to ready */

   ProcTable[proc_slot].priority = priority;                        /* sets the process priority */

   ProcTable[proc_slot].stack = (char *)(malloc (stacksize) );      /* allocates and stores the stack */

   ProcTable[proc_slot].cur_startTime = sys_clock();
 /* initialize context for this process, but use launch function
  * pointer for the initial value of the process's program
  * counter (PC) */
   context_init(&(ProcTable[proc_slot].state), psr_get(), ProcTable[proc_slot].stack, ProcTable[proc_slot].stacksize, launch);
   //launch();
 /* updates the ReadyList */
   ready_temp = ReadyList[priority];

   if (ReadyList[priority] == NULL)                                 /* deals with an empty priority level */
   {
      ReadyList[priority] = &ProcTable[proc_slot];
      ReadyList[priority]->next_proc_ptr = NULL;
   }
   else                                                             /* deals with an non empty priority level */
   {
      while (ready_temp->next_proc_ptr != NULL)
         ready_temp = ready_temp->next_proc_ptr;

      ready_temp->next_proc_ptr = &ProcTable[proc_slot];
   }
   Current = &ProcTable[proc_slot]; //Added this. Not sure if correct
 /* for future phase(s) */
   p1_fork(ProcTable[proc_slot].pid);

 /* calls dispatcher if not sentinel */
   if (strcmp("sentinel", name) != 0)
      dispatcher();

 /* enable interrupts */
   enable_interrupts("fork1");
   return (ProcTable[proc_slot].pid);
}/* fork1 */


/*----------------------------------------------------------------*
 * Name        : join                                             *
 * Purpose     : If current process has children, blocks until    *
 *               one quits. Otherwise returns immediately         *
 * Parameters  : pointer to int where termination code of the     *
 *               quitting process will be stored                  *
 * Returns     : process id of the quitting child joined on -or-  *
 *               -1 if the process was zapped in the join -or-    *
 *               -2 if the process has no children                *
 * Side Effects: If no child process has quit before join is      *
 *               called, the parent is removed from the ready     *
 *               list and blocked                                 *
 *----------------------------------------------------------------*/
int join(int *code)
{
   int child_pid = -1;
   proc_ptr join_temp;

 /* test if in kernel mode, halts otherwise */
   check_kernel_mode("join");

 /* disbale interrupts */
   disable_interrupts("join");

 /* check if current process has no children */
   if (Current->child_proc_ptr == NULL)
   {
    /* enable interrupts */
      enable_interrupts("join");
      console(" - join(): current process has no children -\n");
      return -2;
   }

 /* check if current process has been zapped */
   if (Current->status == ZAPPED)
   {
    /* enable interrupts */
      enable_interrupts("join");
      console(" - join(): current process has been zapped -\n");
      return -1;
   }

 /* check if current process has children that in QuitList */
   join_temp = Current->child_proc_ptr;
   while (child_pid == -1)                                          /* sets up loop to check for child forever */
   {
      while (join_temp != NULL)                                     /* checks if any child has quit */
      {
         if(join_temp->status = QUIT)
            child_pid = join_temp->pid;
         join_temp = join_temp->next_sibling_ptr;
      }
      if (child_pid == -1)                                          /* if no child has quit blocks process and calls dispatcher */
      {
         Current->status = BLOCKED;
         dispatcher();
      }
   }
 /* enable interrupts */
   enable_interrupts("join");

   return 0;
}/* join */


/*----------------------------------------------------------------*
 * Name        : quit                                             *
 * Purpose     : Terminates the current process and returns the   *
 *               status to a join by its parent. If the current   *
 *               process has children instead prints appropriate  *
 *               error message and halts the program.             *
 * Parameters  : the code to return to the grieving parent        *
 * Returns     : nothing                                          *
 * Side Effects: changes the parent of pid child completion       *
 *               status list.                                     *
 *----------------------------------------------------------------*/
void quit(int status)
{
   if (DEBUG && debugflag)
      console(" - quit(): started -\n");

 /* test if in kernel mode, halts otherwise */
   check_kernel_mode("quit");

 /* disbale interrupts */
   disable_interrupts("quit");

 /* checks if current process has any children */
   if (Current->child_proc_ptr != NULL)
   {
      console(" - quit(): current process has children and cannot quit. Halting... -\n");
      halt(1);
   }

 /* sets status of current process to quit */
   Current->status = QUIT;

 /* for future phase(s) */
   p1_quit(Current->pid);

 /* enable interrupts */
   enable_interrupts("quit");
}/* quit */


/*----------------------------------------------------------------*
 * Name        : finish                                           *
 * Purpose     : Required by USLOSS                               *
 * Parameters  : none                                             *
 * Returns     : nothing                                          *
 * Side Effects: none                                             *
 *               status list.                                     *
 *----------------------------------------------------------------*/
void finish()
{
    check_kernel_mode("finish");
   if (DEBUG && debugflag)
      console(" - in finish... -\n");
}/* finish */


/*----------------------------------------------------------------*
 * Name        : sentinel                                         *
 * Purpose     : Keeps the system going when all other processes  *
 *               are blocked. Detects and reports simple deadlock *
 *               states.                                          *
 * Parameters  : dummy variable                                   *
 * Returns     : nothing                                          *
 * Side Effects: if system is in deadlock, prints appropriate     *
 *               error and halt                                   *
 *----------------------------------------------------------------*/
int sentinel (void *unused)
{
   if (DEBUG && debugflag)
      console(" - sentinel(): called -\n");
   while (1)
   {
      check_deadlock();
      waitint();
   }
   halt(0);
}/* sentinel */


/*----------------------------------------------------------------*
 * Name        : check_deadlock                                   *
 * Purpose     : Checks for simple deadlocks.                     *
 * Parameters  : none                                             *
 * Returns     : nothing                                          *
 * Side Effects: if system is in deadlock, prints appropriate     *
 *               error and halt                                   *
 *----------------------------------------------------------------*/
static void check_deadlock()
{
    int numReady = 0;
    int numActive = 0;

    for(int i = 0; i < MAXPROC; i++)
    {
        if(ProcTable[i].status == READY)
        {
            numReady++;
            numActive++;
        }

        if(ProcTable[i].status == BLOCKED || ProcTable[i].status == ZAPPED)
        {
            numActive++;
        }
    }

    if(numReady == 1)
    {
        if(numActive == 1)
        {
            console("All Processes have been completed.\n");
            halt(0);
        }
        else
        {
            console("checkdeadlock(): numProc = %d\n", numActive);
            console("checkdeadlock(): processes still present. Halting...\n");
            halt(1);
        }
        return;
    }
}/* check_deadlock */


/*----------------------------------------------------------------*
 * Name        : launch                                           *
 * Purpose     : Dummy function to enable interrupts and launch a *
 *               given process upon startup.                      *
 * Parameters  : none                                             *
 * Returns     : nothing                                          *
 * Side Effects: enables interupts                                *
 *----------------------------------------------------------------*/
void launch()
{
   int result;

   if (DEBUG && debugflag)
      console(" - launch(): started -\n");

 /* enable interrupts */
   enable_interrupts("launch");

console("here1\n");
 /* call the function passed to fork1, and capture its return value */
   result = Current->start_func(Current->start_arg);
console("here2\n");
   if (DEBUG && debugflag)
      console(" - Process %d returned to launch -\n", Current->pid);

   quit(result);
}/* launch */


/*----------------------------------------------------------------*
 * Name        : dispatcher                                       *
 * Purpose     : Dispatches ready processes. Process with highest *
 *               priority is scheduled to run. The old and new    *
 *               process are swapped.                             *
 * Parameters  : none                                             *
 * Returns     : nothing                                          *
 * Side Effects: the context of the machine is changed            *
 *----------------------------------------------------------------*/
 void dispatcher(void)
 {
    proc_ptr next_process = NULL;
    proc_ptr disp_temp = NULL;
    int i = MAXPRIORITY;

    if (DEBUG && debugflag)
       console(" - dispatcher(): called -\n");

  /* test if in kernel mode, halts otherwise */
    check_kernel_mode("dispatcher");

  /* disable interrupts */
    disable_interrupts("dispatcher");

    update_lists("dispatcher");

    if(Current->status == RUNNING)
    {
        Current->status = READY;

    }
  /* sets next_process to next highest priority process */
    while ( (i <= MINPRIORITY) && (next_process == NULL) )
    {
       disp_temp = ReadyList[i];
       while ( (disp_temp != NULL) && (next_process == NULL))
          next_process = disp_temp;
    }

  /* context switch to next process */
    if (Current == NULL)                                             /* First time Current is NULL */
    {
     /* for future phase(s) */
       p1_switch(0, next_process->pid);
       Current = next_process;
       context_switch( NULL, &next_process->state);
    }

    else                                                             /* All other times Current is defined */
    {
     /* for future phase(s) */
       p1_switch(Current->pid, next_process->pid);
       disp_temp = Current;
       Current = next_process;
    }

    if(disp_temp != Current)
    {
        if(disp_temp->pid > -1)
            disp_temp->CPUTime += sys_clock() - disp_temp->cur_startTime;
        //Current->time_slice
        Current->cur_startTime = sys_clock();
    }

  /* enable interrupts */
  enable_interrupts("dispatcher");
  context_switch( &disp_temp->state, &Current->state);
 } /* dispatcher */


/*----------------------------------------------------------------*
 * Name        : update_lists                                     *
 * Purpose     : Updates all the process lists.                   *
 * Parameters  : name of calling function                         *
 * Returns     : nothing                                          *
 * Side Effects: the lists are all updated                        *
 *----------------------------------------------------------------*/
static void update_lists(char *caller_name)
{
   if (DEBUG && debugflag)
      console("    - update_lists(): called for function %s -\n", caller_name);
   int i;
   proc_ptr block_temp = NULL;
   proc_ptr quit_temp = NULL;
   proc_ptr ready_temp = NULL;
   proc_ptr zapped_temp = NULL;
   proc_ptr found_temp;

   for (i = MAXPRIORITY; i < MINPRIORITY; i++)
   {
      found_temp = ReadyList[i];
      while (found_temp != NULL)
      {
         if (found_temp->status != READY)
            update_list_helper(&block_temp, &quit_temp, &ready_temp, &zapped_temp, found_temp);
         found_temp = found_temp->next_proc_ptr;
      }
   }
}/* update_lists */


/*----------------------------------------------------------------*
 * Name        : update_helper                                    *
 * Purpose     : Helper function to update_lists                  *
 * Parameters  : temp lists from update_lists and process to be   *
 *               added                                            *
 * Returns     : nothing                                          *
 * Side Effects: none                                             *
 *----------------------------------------------------------------*/
static void update_list_helper(proc_ptr *b_t, proc_ptr *q_t, proc_ptr *r_t, proc_ptr *z_t, proc_ptr found)
{
   if (DEBUG && debugflag)
      console("       - update_lists_helper(): called -\n");

   proc_ptr helper_temp;
   switch (found->status)
   {
      case READY  : helper_temp = *r_t;
                    break;
      case BLOCKED: helper_temp = *b_t;
                    break;
      case ZAPPED : helper_temp = *z_t;
                    break;
      case QUIT   : helper_temp = *q_t;
                    break;
   }

   while(helper_temp != NULL)
      helper_temp = helper_temp->next_proc_ptr;

   helper_temp = found;
   found->next_proc_ptr = NULL;
}

/*----------------------------------------------------------------*
 * Name        : check_kernel_mode                                *
 * Purpose     : Checks the current kernel mode.                  *
 * Parameters  : name of calling function                         *
 * Returns     : nothing                                          *
 * Side Effects: halts process if in user mode                    *
 *----------------------------------------------------------------*/
static void check_kernel_mode(char *caller_name)
{
   union psr_values caller_psr;                                     /* holds the current psr values */
   if (DEBUG && debugflag)
      console("    - check_kernel_mode(): called for function %s -\n", caller_name);

 /* checks if in kernel mode, halts otherwise */
   caller_psr.integer_part = psr_get();                             /* stores current psr values into structure */
   if (caller_psr.bits.cur_mode != 1)
   {
      console("    - %s(): called while not in kernel mode, by process %d. Halting... -\n", caller_name, Current->pid);
      halt(1);
   }
}/* check_kernel_mode */


/*----------------------------------------------------------------*
 * Name        : disable_interrupts                               *
 * Purpose     : Disables all interupts.                          *
 * Parameters  : name of calling function                         *
 * Returns     : nothing                                          *
 * Side Effects: halts process if in user mode, disables          *
 *               interupts                                        *
 *----------------------------------------------------------------*/
void disable_interrupts(char *caller_name)
{
   if (DEBUG && debugflag)
      console("    - disable_interrupts(): interupts turned off for %s -\n", caller_name);
}/* disable_interrupts */


/*----------------------------------------------------------------*
 * Name        : enable_interrupts                                *
 * Purpose     : Enables all interupts.                           *
 * Parameters  : name of calling function                         *
 * Returns     : nothing                                          *
 * Side Effects: halts process if in user mode, enables interupts *
 *----------------------------------------------------------------*/
void enable_interrupts(char *caller_name)
{
    check_kernel_mode("enable_interrupts");
    psr_set(psr_get() || PSR_CURRENT_INT);
   if (DEBUG && debugflag)
      console("    - enable_interrupts(): interupts turned on for %s -\n", caller_name);
}/* enable_interrupts */
void dump_processes()
{
    /*Prints process information to the console. For each PCB in the process table,
    print it's PID, parent's PID, priority, process status, # of children,
    CPU time consumed, and name. */
    for(int i = 0; i < MAXPROC; i++)
    {
        if((ProcTable[i].pid) = UNUSED) //Process slot is unsused
        {
            console("There are currently no processes.\n");
        }
        console("Current Process name: %s\n", ProcTable[i].name);
        console("Current Process PID: %s\n", ProcTable[i].pid);
        //console("Current Process Parent PID: %s", ProcTable[i]->);
        console("Current Process Priority: %s \n", ProcTable[i].priority);
        console("Current Process Status: %s\n", ProcTable[i].status);
        console("Current Process Number of Children: %s\n", ProcTable[i].numChildren);
        console("Current Process CPU Time: %d\n", readtime);
    }

}
int get_pid()
{
    //Return process ID of currently running proccess
    return Current->pid;
}

int readtime()
{
    check_kernel_mode("readtime");
    return Current->CPUTime/1000;
}


int block_me(int new_status)
{
    /*Blocks the calling process. Returns -1 if process was zapped while Blocked
    Returns 0 otherwise. */
    if(is_zapped())
    {
        return -1;
    }
    return 0;
}

int zap(int pid)
{
    if(Current->pid == pid)
    {
        console("Process cannot zap itself! Halting.\n");
        halt(1);
        return -1;
    }
    for(int i = 0; i < MAXPROC; i++)
    {
        if(ProcTable[i].pid = pid)
        {
            ProcTable[i].status = ZAPPED;
            return 0;
        }
    }
    console("Process does not exist\n");
    return -1;
}

int is_zapped()
{
    if(Current->status = ZAPPED)
    {
        return 1;
    }
    return 0;
}

int unblock_proc(int pid)
{
    /*Unblocks the process with pid that had been previously blocked via block_me()
    The status of the process is changed to READY and is put on the Ready list
    Dispatcher will be called as a side effect.
    Returns -2 if process was not blocked, does not existm is the current process,
        or is blocked on a status less than or equal to 10.
    Returns -1 if the calling process was zapped.
    Returns 0 otherwise. */

    return 0;
}
int read_cur_start_time()
{
    /*Returns the time in microseconds at which the currently executing process
    began its current time slice */
    return Current->cur_startTime;
}

void time_slice()
{
    int run_time = sys_clock() - read_cur_start_time();

    if(run_time >= TIMESLICE)
    {
        dispatcher();/*Calls dispatcher if currently executing process
        has exceeded its time slice*/
    }
}
//fish
