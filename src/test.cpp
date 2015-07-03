#include "mailman.h"

int main(void)
{
  char *p = "iunix@sohu.com";
  Mailman man("221.215.1.149", "yuxiaofeng3@hisense.com", "yuxiaofeng3@hisense.com","eXV4aWFvZmVuZzM=","QWJjZDEyMzQt");
  man.sendmail();
  return 0;
}
