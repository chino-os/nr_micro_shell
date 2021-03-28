#include <stdio.h>
#include "nr_micro_shell.h"



#define NR_SHELL_DECLARE_ARGS_INFO(x) 

// NR_SHELL_DECLARE_ARGS_INFO(\
//     "
//     $arg1
//     {
//         -h:.........\n
//         -a:.........\n
//         -x:.........\n
//     }
//     $arg2
//     {
//         -h:.........\n
//         -a:.........\n
//         -x:.........\n
//     }
//     $arg3
//     {
//         -h:.........\n
//         -a:.........\n
//         -x:.........\n
//     }
//     "
// )





const char *arg1_info[] = {"-h","-a","-x"};
const char *arg2_info[] = {"abc","def","ghi"};
const char **args_info[] = {arg1_info,arg2_info};

int main(void)
{
    char c;
    shell_init();
    while(1)
    {
        c = getchar();
        shell(c);
    }
}