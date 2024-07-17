#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// 启用或禁用调试信息
#define DEBUG 0

// 宏来处理调试打印
#define debug(codes) if(DEBUG) {codes}

// 函数：fork并执行给定参数的命令
void xargs_exec(char* program, char** paraments);

// 函数：读取输入行并执行带参数的命令
void xargs(char** first_arg, int size, char* program_name, int n)
// 包含命令行初始参数列表的字符串数组,数组的大小,要执行的程序名称,'xargs'一次应该传递给命令的参数数量
{
    char buf[1024]; // 缓冲区，用于存储输入行
    debug(
        for (int i = 0; i < size; ++i) {
            printf("first_arg[%d] = %s\n", i, first_arg[i]);
        }
    )
    char *arg[MAXARG]; // 命令的参数列表
    int m = 0; // 缓冲区中的位置索引

    // 从标准输入读取，一个字符一个字符地读取
    while (read(0, buf + m, 1) == 1) {
        if (m >= 1024) {
            fprintf(2, "xargs: arguments too long.\n");
            exit(1);
        }
        // 如果遇到换行符，处理缓冲区
        if (buf[m] == '\n') {
            buf[m] = '\0'; // 字符串结束符
            debug(printf("this line is %s\n", buf);)
            memmove(arg, first_arg, sizeof(char*) * size); // 将初始参数复制到arg数组

            // 在初始参数之后设置参数索引
            int argIndex = size;
            if (argIndex == 0) {
                arg[argIndex] = program_name;
                argIndex++;
            }

            // 为新参数分配内存并复制
            arg[argIndex] = buf;
            arg[argIndex + 1] = 0; // 确保参数列表以NULL结尾
            debug(
                for (int j = 0; j <= argIndex; ++j)
                    printf("arg[%d] = *%s*\n", j, arg[j]);
            )
            
            xargs_exec(program_name, arg); // 执行带参数的命令
            m = 0; // 重置缓冲区索引以读取下一输入行
        } else {
            m++; // 移动到缓冲区中的下一个字符位置
        }
    }
}

// 函数：fork新进程并执行命令
void xargs_exec(char* program, char** paraments)
{
    if (fork() > 0) {
        // 父进程等待子进程完成
        wait(0);
    } else {
        // 子进程执行命令
        debug(
            printf("child process\n");
            printf("    program = %s\n", program);
            for (int i = 0; paraments[i] != 0; ++i) {
                printf("    paraments[%d] = %s\n", i, paraments[i]);
            }
        )
        if (exec(program, paraments) == -1) {
            // 如果exec失败，打印错误信息
            fprintf(2, "xargs: Error exec %s\n", program);
        }
        debug(printf("child exit");)
    }
}

// 主函数：处理命令行参数并调用xargs
int main(int argc, char* argv[])
{
    debug(printf("main func\n");)
    if (argc < 2) {
        // 如果提供的参数不足，打印使用信息
        fprintf(2, "Usage: xargs <command> [args...]\n");
        exit(1);
    }

    int n = 1; // 默认传递给命令的参数数量
    char *name = argv[1]; // 默认要执行的命令
    int first_arg_index = 1; // argv中第一个参数的索引

    // 检查是否提供了-n选项
    if (argc >= 4 && strcmp(argv[1], "-n") == 0) {
        n = atoi(argv[2]); // 获取n的值
        name = argv[3]; // 要执行的命令
        first_arg_index = 3; // 调整第一个参数索引
    }

    debug(
        printf("command = %s\n", name);
    )

    // 调用xargs函数，传递命令和参数
    xargs(argv + first_arg_index, argc - first_arg_index, name, n);
    exit(0);
}

