/*----------------------------display----------------------------*/
// SAT求解器交互显示模块
// 功能：提供命令行式交互主控菜单，支持CNF文件读取、公式遍历打印、
//       DPLL基准版与优化版求解对比、高精度计时、求解结果文件保存、星形数独入口
#include "sat.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>   // Windows系统API，提供高精度计时器、系统命令调用等能力
#include <string.h>

extern long long g_recur_cnt; // DPLL递归调用全局计数器（在其他模块中定义）

/*
 @ 函数名称: DisPlay
 @ 函数功能: 交互主控界面，文件读取、DPLL基准/优化求解、计时、保存结果、星形数独入口
 @ 返回值: void
 */
void DisPlay()
{
    // 分配并初始化CNF公式结构体
    CNF *cnf = (CNF *)malloc(sizeof(CNF));
    cnf->head = NULL;       // 子句链表头指针置空
    cnf->var_num = 0;       // 变量总数初始化为0
    cnf->clause_num = 0;    // 子句总数初始化为0
    char fileName[100];     // 输入CNF文件名缓冲区
    char outFileName[100];  // 输出结果文件名缓冲区
    PrintMenu();            // 首次打印主功能菜单
    int op = 1;             // 用户操作选择变量，初始非0以进入主循环

    // 主交互循环，输入0时退出程序
    while (op)
    {
        // 打印操作提示分隔栏
        printf("\n|--------------------------------------------|\n");
        printf("|--------Please Choose Your Operation--------|\n");
        printf("|--------------------------------------------|\n\n");
        printf("               Your choice: ");
        scanf("%d", &op);      // 读取用户选择的功能编号
        system("cls");         // 调用Windows系统命令清屏
        PrintMenu();           // 清屏后重新打印菜单，保持界面整洁

        switch (op)
        {
        // 选项1：读取CNF文件
        case 1:
        {
            // 若已加载过CNF公式，询问用户是否重新读取
            if (cnf->head != NULL)
            {
                printf(" The CNF has been read.\n");
                printf(" Do you want to read another? (1/0): ");
                int choice;
                scanf("%d", &choice);
                if (choice == 0)
                    break;  // 选择不重新读取，直接退出当前选项
                else
                {
                    // 销毁原有CNF的所有内存，重置计数变量
                    DestroyCnf(&cnf->head);
                    cnf->var_num = 0;
                    cnf->clause_num = 0;
                }
            }
            // 输入文件名并读取CNF文件
            printf(" Please input the cnf file name: ");
            scanf("%s", fileName);
            if (ReadCNFFile(cnf, fileName) == OK)
                printf(" Read cnf file successfully.\n");
            else
                printf(" Read cnf file failed.\n");
            break;
        }

        // 选项2：遍历并输出CNF所有子句
        case 2:
        {
            // 未加载CNF文件时给出提示
            if (cnf->head == NULL)
                printf(" You haven't open the CNF file.\n");
            else
                PrintCnf(cnf);  // 调用打印函数输出完整CNF公式
            break;
        }

        // 选项3：使用DPLL算法求解，对比基准版与优化版性能，支持结果保存
        case 3:
        {
            // 未加载CNF文件时提示并退出当前选项
            if (cnf->head == NULL)
            {
                printf(" You haven't open the CNF file.\n");
                break;
            }
            int var_max = cnf->var_num;  // 获取公式中的变量总数

            // 分配基准版DPLL的变量赋值数组，下标1~var_max对应各个变量
            // 赋值约定：0=FALSE，1=TRUE，2=UNASSIGNED（未赋值）
            int *assign_base = (int *)malloc(sizeof(int) * (var_max + 2));
            for (int i = 1; i <= var_max; i++)
                assign_base[i] = 2;  // 初始化为未赋值状态

            int *out_assign_base = NULL;  // 接收基准版求解后的最终赋值结果指针

            // Windows高精度计时器变量，用于基准版与优化版计时
            LARGE_INTEGER freq_base, freq_opt;
            LARGE_INTEGER start_base, end_base;
            LARGE_INTEGER start_opt, end_opt;
            double time_base_ms = 0.0, time_opt_ms = 0.0;  // 基准/优化版求解耗时（单位：毫秒）
            status res_base;  // 基准版求解结果状态（SAT/UNSAT）

            // ========== flag=1 基准DPLL计时 ==========
            QueryPerformanceFrequency(&freq_base);       // 获取系统高精度计时器的频率
            QueryPerformanceCounter(&start_base);        // 记录求解开始时间戳
            res_base = DPLL(cnf, assign_base, &out_assign_base, 2, var_max);  // 调用基准版DPLL求解
            QueryPerformanceCounter(&end_base);          // 记录求解结束时间戳
            // 根据时间戳与频率计算实际耗时，转换为毫秒单位
            time_base_ms = (double)(end_base.QuadPart - start_base.QuadPart) * 1000.0 / freq_base.QuadPart;

            // 控制台输出基准版求解结果
            printf("\n======== Base DPLL(flag=1) Result ========\n");
            if (res_base == OK)
            {
                printf(" SAT (satisfiable)\n");
                // 逐个输出每个变量的赋值结果
                for (int i = 1; i <= var_max; i++)
                {
                    if(out_assign_base[i] == 1)
                        printf(" %-4d: TRUE\n", i);
                    else if(out_assign_base[i] == 0)
                        printf(" %-4d: FALSE\n", i);
                    else
                        printf(" %-4d: UNASSIGNED\n", i);
                }
            }
            else
            {
                printf(" UNSAT (unsatisfiable)\n");
            }
            printf(" Recursion count: %lld\n", g_recur_cnt);  // 输出DPLL递归调用总次数
            printf(" Time: %.2lf ms (not optimized)\n", time_base_ms);

            // ========== 是否执行优化DPLL flag=2/3 ==========
            int ch;
            printf("\n Do you want to run optimized DPLL(flag=2/3)? (0/2/3): ");
            scanf("%d", &ch);
            int *out_assign_opt = NULL;  // 接收优化版求解后的最终赋值结果指针

            // 用户选择2或3时，运行对应优化级别的DPLL算法
            if (ch == 2||ch== 3)
            {
                // 分配并初始化优化版的变量赋值数组
                int *assign_opt = (int *)malloc(sizeof(int) * (var_max + 2));
                for (int i = 1; i <= var_max; i++)
                    assign_opt[i] = 2;

                g_recur_cnt = 0;  // 重置全局递归计数器，单独统计优化版递归次数
                QueryPerformanceFrequency(&freq_opt);       // 获取计时器频率
                QueryPerformanceCounter(&start_opt);        // 记录优化版求解开始时间
                DPLL(cnf, assign_opt, &out_assign_opt, ch, var_max);  // 调用对应优化级别的DPLL
                QueryPerformanceCounter(&end_opt);          // 记录优化版求解结束时间
                // 计算优化版求解耗时
                time_opt_ms = (double)(end_opt.QuadPart - start_opt.QuadPart) * 1000.0 / freq_opt.QuadPart;

                // 控制台输出优化版性能统计
                printf("\n======== Optimized DPLL(flag=2/3) ========\n");
                printf(" Recursion count: %lld\n", g_recur_cnt);
                printf(" Time: %.2lf ms (optimized)\n", time_opt_ms);

                // 计算性能优化率：(基准时间 - 优化时间) / 基准时间 * 100%
                double rate = 0.0;
                if(time_base_ms > 1e-6)  // 避免基准时间过小时出现除零错误
                {
                    rate = ((time_base_ms - time_opt_ms) / time_base_ms) * 100.0;
                }
                printf(" Optimize rate[(t-to)/t]*100%% : %.2f %% \n", rate);

                free(assign_opt);  // 释放优化版赋值数组的堆内存
            }

            // ========== 保存结果到文件 ==========
            int choiceSave;
            printf("\n Save the result to output file? (1/0): ");
            scanf("%d", &choiceSave);
            if (choiceSave == 1)
            {
                printf(" input output file name(e.g: out.res): ");
                scanf("%s", outFileName);
                // 调用保存函数将求解结果写入指定文件
                if(SaveResult(outFileName, res_base, var_max, out_assign_base, time_base_ms, time_opt_ms)==OK)
                    printf(" Save result successfully.\n");
                else
                    printf(" Save result failed.\n");
            }

            // 释放本次求解过程中申请的所有堆内存，防止内存泄漏
            if(out_assign_base != NULL) free(out_assign_base);
            if(out_assign_opt != NULL) free(out_assign_opt);
            free(assign_base);
            break;
        }

        // 选项4：进入星形数独游戏模块
        case 4:
        {
           X_Sudoku();
           break;
        }

        // 选项0：退出程序
        case 0:
        {
            printf(" Exit successfully.\n");
            // 释放CNF公式所有内存后正常返回
            if(cnf->head != NULL)
                DestroyCnf(&cnf->head);
            free(cnf);
            return;
        }

        // 输入无效选项时的提示
        default:
        {
            printf(" Invalid input.\n");
            break;
        }
        }
    }

    // 循环异常结束路径的内存释放兜底
    if(cnf->head != NULL)
        DestroyCnf(&cnf->head);
    free(cnf);
    return;
}

/*
 @ 函数名称: PrintMenu
 @ 函数功能: 打印主功能菜单
 @ 返回值: void
 */
void PrintMenu()
{
    printf("|================Menu for SAT================|\n");
    printf("|--------------------------------------------|\n");
    printf("|            1. Open the CNF file            |\n");
    printf("|     2. Traverse and output each clause     |\n");
    printf("|   3. Solve using DPLL and save the result  |\n");
    printf("|      4. Asterisk-Sudoku game module        |\n");
    printf("|                 0.  EXIT                   |\n");
    printf("|============================================|\n\n");
}

/*
 @ 函数名称: SaveResult
 @ 函数功能: 将求解结果、时间、优化率写入文件
 @ 参数 outFile:输出文件名；res_status:求解结果；var_max变量数；assign赋值；t_base基准ms；t_opt优化ms
 @ 返回值: OK成功，ERROR失败
 */
status SaveResult(const char *outFile, status res_status, int var_max, int assign[], double t_base, double t_opt)
{
    // 以写入模式打开输出文件
    FILE *fp = fopen(outFile, "w");
    if(fp == NULL)
        return ERROR;  // 文件打开失败直接返回错误

    // 写入输出文件头部标题
    fprintf(fp,"======== SAT-DPLL Solver Output ========\n");

    // 写入求解结果状态与变量赋值详情
    if(res_status == OK)
    {
        fprintf(fp,"Result: SAT\n");
        fprintf(fp,"Assignment:\n");
        // 逐行写入每个变量的赋值状态
        for(int i=1;i<=var_max;i++)
        {
            if(assign[i]==1)
                fprintf(fp,"%-4d: TRUE\n",i);
            else if(assign[i]==0)
                fprintf(fp,"%-4d: FALSE\n",i);
            else
                fprintf(fp,"%-4d: UNASSIGNED\n",i);
        }
    }
    else
    {
        fprintf(fp,"Result: UNSAT\n");
    }

    // 写入基准版与优化版的求解耗时
    fprintf(fp,"Base time t(ms): %.2lf\n", t_base);
    fprintf(fp,"Optimized time to(ms): %.2lf\n", t_opt);

    // 计算并写入性能优化率
    double rate = 0.0;
    if(t_base>1e-6)  // 避免基准时间过小时出现除零错误
    {
        rate = ((t_base-t_opt)/t_base)*100.0;
    }
    fprintf(fp,"Optimize rate [(t-to)/t]*100%%: %.2f %% \n", rate);

    // 写入DPLL递归调用总次数
    fprintf(fp,"Recursion count: %lld\n", g_recur_cnt);

    fclose(fp);  // 关闭文件，释放文件资源
    return OK;
}

