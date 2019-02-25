#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
//Check if kernel aborts on illegal stacksize given to fork().
*/
int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);


int start1(char *arg)
{
  int pid1, status;

  pid1 = fork1("XXp2", XXp2,"XXp2",20*(USLOSS_MIN_STACK-10),2);
	join( &status );
  quit(-1);
  return 0;
}


int XXp1(char *arg)
{
  console("TEST:");console("start %s\n", arg);
  quit(-2);
  return 0;
}


int XXp2(char *arg)
{
	int i, pid1;
  for (i=0;i<2;i++) {
	  pid1 = fork1("XXp1", XXp1,"XXp1",USLOSS_MIN_STACK-10,2);
	  if(pid1 == -2)
	    console("You got it!\n");
	}
	return(0);
}
