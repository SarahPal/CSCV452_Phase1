#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/* The purpose of this test is to demonstrate that
 * an attempt to zap an unexistant process results in
 * a halt.
 *
 * Expected output:
 * start1(): started
 * start1(): after fork of child 3
 * start1(): performing first join
 * XXp1(): started
 * XXp1(): arg = `XXp1'
 * XXp1(): zapping a non existant processes pid, should cause abort, calling zap(4)
 * zap(): process being zapped does not exist.  Halting...
*/

int XXp1(char *);
char buf[256];

int start1(char *arg)
{
  int status, pid1, kidpid;

  printf("start1(): started\n");
  pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
  printf("start1(): after fork of child %d\n", pid1);
  printf("start1(): performing first join\n");
  kidpid = join(&status);
  sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
  printf("%s", buf);
  return 0;
} /* start1 */

int XXp1(char *arg)
{
  int zap_result, to_zap;

  to_zap = 4;
  printf("XXp1(): started pid=%d\n", getpid());
  printf("XXp1(): arg = `%s'\n", arg);
  printf("XXp1(): zapping a non existant processes pid, should cause abort, ");
  printf("calling zap(%d)\n", to_zap);
  zap_result = zap(to_zap);
  quit(-1);
  return 0;
} /* XXp1 */
