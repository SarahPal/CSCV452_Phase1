/* A simple check for zap() and is_zapped() */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);


int
start1( char *arg ) 
{

  int status, kidpid, zap_result, join_result;

  console("START1: calling fork1 for XXp1\n");
  kidpid = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 5);

  console("START1: calling zap\n");
  zap_result = zap(kidpid);

  console("START1: zap_result = %d\n", zap_result);
  join_result = join(&status);

  quit(0);
  return 0;

} /* start1 */


int
XXp1( char *arg )
{

  console("XXp1: started\n");

  while ( is_zapped() == 0 ) {
  }

  console("XXp1: calling quit\n");
  quit(5);
  return 0;

} /* XXp1 */
