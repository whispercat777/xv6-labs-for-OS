#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 实现质数筛选逻辑的函数
void prime_sieve(int p[2]) {
    int prime, num;
    close(p[1]); // 关闭当前管道的写端
    
    // 从管道读取第一个数，它是一个质数
    if (read(p[0], &prime, sizeof(prime)) == 0) {
        // 如果没有读到数，关闭读端并退出
        close(p[0]);
        exit(0);
    }
    printf("prime %d\n", prime);
   
    // 创建一个新管道
    int next_pipe[2];
    pipe(next_pipe);

    // 创建一个新进程
    if (fork() == 0) {
        // 子进程
        close(p[0]);
        prime_sieve(next_pipe); // 递归调用
    } else {
        // 父进程
        close(next_pipe[0]);
        
        // 从当前管道读取数字，过滤掉质数的倍数，将其余的写入下一个管道
        while (read(p[0], &num, sizeof(num)) > 0) {
            if (num % prime != 0) {
                write(next_pipe[1], &num, sizeof(num));
            }
        }
        
        close(p[0]);
        close(next_pipe[1]);
        wait(0); // 等待子进程完成
    }
}

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    
    // 创建第一个筛选进程
    if (fork() == 0) {
        prime_sieve(p); // 子进程开始筛选
    } else {
        // 父进程
        close(p[0]); // 关闭初始管道的读端
        
        // 写入数字2到35到管道
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }

        close(p[1]); // 关闭初始管道的写端
        wait(0); // 等待筛选进程完成
        exit(0);
    }
    return 0;
}

