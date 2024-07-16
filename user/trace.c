#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];
  //保证trace的参数不少于三个，并且跟踪的系统调用号在0-99之间
  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }
  // 调用 trace 系统调用，并传递第一个命令行参数的整数值
  if (trace(atoi(argv[1])) < 0) {
    // 如果 trace 调用失败，打印错误信息并退出
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  // 将命令行参数从 argv[2] 开始复制到 nargv 数组中
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  // 使用 exec 系统调用执行 nargv[0] 命令，并传递 nargv 数组作为参数
  exec(nargv[0], nargv);
  exit(0);
}
