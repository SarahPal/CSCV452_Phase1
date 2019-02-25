#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*

//Check if -1 is returned when there are no more process slots.

*/

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);

int start1(char *arg)
{
  int i, pid1;

  console("TEST:");console("start %d processes\n",MAXPROC);
  for (i=0;i<MAXPROC+2;i++) {
	  pid1 = fork1("XXp1", XXp1,"XXp1",USLOSS_MIN_STACK,2);
	  if (pid1== -1) {
		 console("TEST:");console("pid is -1.\n");
	  }
  }
  for (i=0;i<MAXPROC+2;i++) {
	  join(&pid1);
  }  
  quit(-1);
  return 0;
}

int XXp1(char *arg)
{
 
  quit(-2);

  return 0;
}


