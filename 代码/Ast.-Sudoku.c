// 星形数独模块：实现基于SAT/DPLL的星形数独求解、随机生成、交互游玩功能
#include "sat.h"

// DPLL算法全局递归调用计数器，统计求解过程中的递归总次数
long long g_recur_cnt=0;

// 星形数独的9个星形单元格坐标（行, 列），构成额外的约束区域
static int star_pos[9][2]={
    {2,5},{3,3},{3,7},{5,2},{5,5},{5,8},{7,3},{7,7},{8,5}
};

/**
 * @brief 检查在指定位置填入数字是否符合星形数独全部规则
 * @param board 当前数独棋盘
 * @param row 目标行（1~9）
 * @param col 目标列（1~9）
 * @param num 待填入的数字（1~9）
 * @return 合法返回TRUE，违反任意规则返回FALSE
 * @details 依次校验：行不重复、列不重复、3x3宫不重复、星形区域不重复
 */
status Is_Valid(int board[SIZE+1][SIZE+1],int row,int col,int num)
{
    // 检查行约束：同一行不能出现相同数字
    for(int i=1;i<=SIZE;i++)
    {
        if(board[row][i]==num)
            return FALSE;
        // 检查列约束：同一列不能出现相同数字
        if(board[i][col]==num)
            return FALSE;
    }
    // 计算当前位置所属3x3宫的左上角坐标
    int start_row=(row-1)/3*3+1;
    int start_col=(col-1)/3*3+1;
    // 检查宫约束：同一3x3宫内不能出现相同数字
    for(int i=start_row;i<start_row+3;i++)
    {
        for(int j=start_col;j<start_col+3;j++)
        {
            if(board[i][j]==num)
                return FALSE;
        }
    }
    // 判断当前单元格是否属于星形区域
    bool is_star_cell=false;
    for(int i=0;i<9;i++)
    {
        if(row==star_pos[i][0] && col==star_pos[i][1])
        {
            is_star_cell=true;
            break;
        }
    }
    // 若为星形单元格，额外校验：所有星形单元格内不能出现相同数字
    if(is_star_cell){
    for(int i=0;i<9;i++)
    {
        int r=star_pos[i][0];
        int c=star_pos[i][1];
        if(board[r][c]==num)
            return FALSE;
    }}
    return TRUE;
}

/**
 * @brief 将星形数独棋盘转化为DIMACS格式的CNF公式文件
 * @param board 数独题目棋盘
 * @param num_hint 提示数字的数量
 * @param filename 输出的CNF文件名
 * @return 写入成功返回OK，失败返回ERROR
 * @details 共编码7类约束子句；变量编码规则：(行-1)*81 + (列-1)*9 + 数字
 */
status WriteToFile(int board[SIZE+1][SIZE+1], int num_hint, const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if(fp == NULL)
    {
        return ERROR;
    }
    // 先写入占位头部，后续统计完子句总数后回退重写正确数值
    fprintf(fp, "p cnf 729 0\n");
    int clause_cnt = 0;  // 统计子句总数
    int i,j,v,a,b;

    //==================== 1.每个格子至少一个数字 ====================
    // 每个单元格必须填入1~9中的至少一个数字
    for(i = 1; i <= 9; i++)
    {
        for(j = 1; j <= 9; j++)
        {
            for(v = 1; v <= 9; v++)
            {
                int var = (i-1)*81 + (j-1)*9 + v;
                fprintf(fp, "%d ", var);
            }
            fprintf(fp, "0\n");
            clause_cnt++;
        }
    }

    //==================== 2.同一格子互斥:不能同时两个数字 ====================
    // 每个单元格不能同时填入两个不同的数字
    for(i = 1; i <= 9; i++)
    {
        for(j = 1; j <= 9; j++)
        {
            for(a = 1; a <= 9; a++)
            {
                for(b = a+1; b <= 9; b++)
                {
                    int va = (i-1)*81 + (j-1)*9 + a;
                    int vb = (i-1)*81 + (j-1)*9 + b;
                    fprintf(fp, "-%d -%d 0\n", va, vb);
                    clause_cnt++;
                }
            }
        }
    }

    //==================== 3.行约束 ====================
    for(i = 1; i <= 9; i++)
    {
        for(v = 1; v <= 9; v++)
        {
            // 行i中，数字v至少出现一次
            for(j = 1; j <= 9; j++)
            {
                int var = (i-1)*81 + (j-1)*9 + v;
                fprintf(fp, "%d ", var);
            }
            fprintf(fp, "0\n");
            clause_cnt++;

            // 行i中，数字v不能出现两次（同行同数字互斥）
            for(a = 1; a <= 9; a++)
            {
                for(b = a+1; b <= 9; b++)
                {
                    int va = (i-1)*81 + (a-1)*9 + v;
                    int vb = (i-1)*81 + (b-1)*9 + v;
                    fprintf(fp, "-%d -%d 0\n", va, vb);
                    clause_cnt++;
                }
            }
        }
    }

    //==================== 4.列约束 ====================
    for(j = 1; j <= 9; j++)
    {
        for(v = 1; v <= 9; v++)
        {
            // 列j中，数字v至少出现一次
            for(i = 1; i <= 9; i++)
            {
                int var = (i-1)*81 + (j-1)*9 + v;
                fprintf(fp, "%d ", var);
            }
            fprintf(fp, "0\n");
            clause_cnt++;

            // 列j中，数字v不能出现两次（同列同数字互斥）
            for(a = 1; a <= 9; a++)
            {
                for(b = a+1; b <= 9; b++)
                {
                    int va = (a-1)*81 + (j-1)*9 + v;
                    int vb = (b-1)*81 + (j-1)*9 + v;
                    fprintf(fp, "-%d -%d 0\n", va, vb);
                    clause_cnt++;
                }
            }
        }
    }

    //==================== 5.宫约束 ====================
    // 9个3x3宫的左上角坐标数组
    int block_start[9][2] = {
        {1,1},{1,4},{1,7},
        {4,1},{4,4},{4,7},
        {7,1},{7,4},{7,7}
    };
    for(int blk = 0; blk < 9; blk++)
    {
        int si = block_start[blk][0];
        int sj = block_start[blk][1];
        for(v = 1; v <= 9; v++)
        {
            // 宫内数字v至少出现一次
            for(int di = 0; di < 3; di++)
            {
                for(int dj = 0; dj < 3; dj++)
                {
                    int ii = si + di;
                    int jj = sj + dj;
                    int var = (ii-1)*81 + (jj-1)*9 + v;
                    fprintf(fp, "%d ", var);
                }
            }
            fprintf(fp, "0\n");
            clause_cnt++;

            // 宫内数字v不能出现两次（同宫同数字互斥）
            int pos[9][2];  // 存储当前宫内9个单元格的坐标
            int idx = 0;
            for(int di = 0; di < 3; di++)
            {
                for(int dj = 0; dj < 3; dj++)
                {
                    pos[idx][0] = si+di;
                    pos[idx][1] = sj+dj;
                    idx++;
                }
            }
            // 两两组合生成互斥子句
            for(a = 0; a < 9; a++)
            {
                for(b = a+1; b < 9; b++)
                {
                    int i1 = pos[a][0], j1 = pos[a][1];
                    int i2 = pos[b][0], j2 = pos[b][1];
                    int va = (i1-1)*81 + (j1-1)*9 + v;
                    int vb = (i2-1)*81 + (j2-1)*9 + v;
                    fprintf(fp, "-%d -%d 0\n", va, vb);
                    clause_cnt++;
                }
            }
        }
    }

    //==================== 6.星形9格两两互斥 ====================
    // 所有星形单元格中，同一个数字只能出现一次
    for(a = 0; a < 9; a++)
    {
        for(b = a+1; b < 9; b++)
        {
            int i1 = star_pos[a][0], j1 = star_pos[a][1];
            int i2 = star_pos[b][0], j2 = star_pos[b][1];
            for(v = 1; v <= 9; v++)
            {
                int va = (i1-1)*81 + (j1-1)*9 + v;
                int vb = (i2-1)*81 + (j2-1)*9 + v;
                fprintf(fp, "-%d -%d 0\n", va, vb);
                clause_cnt++;
            }
        }
    }

    //==================== 7.提示单子句 ====================
    // 题目中已给出的数字，直接作为单位子句固定变量取值
    for(i = 1; i <= 9; i++)
    {
        for(j = 1; j <= 9; j++)
        {
            if(board[i][j] != 0)
            {
                int v = board[i][j];
                int var = (i-1)*81 + (j-1)*9 + v;
                fprintf(fp, "%d 0\n", var);
                clause_cnt++;
            }
        }
    }

    // 回退文件指针到文件开头，重写正确的p cnf头部（更新子句总数）
    rewind(fp);
    fprintf(fp, "p cnf 729 %d\n", clause_cnt);
    fclose(fp);
    return OK;
}

/**
 * @brief 将DPLL求解得到的变量赋值解码为数独棋盘
 * @param value DPLL求解后的变量赋值数组（1=TRUE, 0=FALSE, 2=未赋值）
 * @param out_board 输出解码后的数独棋盘
 */
void DecodeSolution(int value[],int out_board[SIZE+1][SIZE+1]){
  // 初始化输出棋盘为全0
  memset(out_board,0,sizeof(int)*(SIZE+1)*(SIZE+1));
    // 遍历每个单元格的每个可能数字
    for(int row=1;row<=SIZE;row++){
        for(int col=1;col<=SIZE;col++){
        for(int num=1;num<=SIZE;num++){
            // 计算对应SAT变量编号
            int var=(row-1)*SIZE*SIZE+(col-1)*SIZE+num;
            // 变量为真表示该单元格填入该数字
            if(value[var]==1){
            out_board[row][col]=num;
            break;
            }
        }
        }
    }
}

/**
 * @brief 打印数独棋盘到控制台
 * @param board 待打印的数独棋盘
 * @details 0表示空格，用'.'显示；每3行打印分隔线区分3x3宫
 */
void PrintBoard(int board[SIZE+1][SIZE+1]){
    printf("===== Sudoku Board =====\n");
    for(int i=1;i<=SIZE;i++){
        for(int j=1;j<=SIZE;j++){
            if(board[i][j]==0)
                printf(". ");  // 空格显示为点
            else
                printf("%d ",board[i][j]);
        }
        printf("\n");
        // 每3行打印横向分隔线（最后一行除外）
        if(i%3==0 && i!=SIZE)
            printf("---------------------\n");
    }
}

/**
 * @brief 使用DPLL算法求解星形数独
 * @param question 输入的数独题目棋盘
 * @param sol_board 输出求解得到的答案棋盘
 * @param value DPLL使用的变量赋值数组
 * @return 求解成功返回OK，失败返回ERROR
 */
status Solve_AstSudoku(int question[SIZE+1][SIZE+1],int sol_board[SIZE+1][SIZE+1],int value[]){
    int num_hint=0;
    // 统计题目中的提示数字数量
    for(int i=1;i<=SIZE;i++){
        for(int j=1;j<=SIZE;j++){
            if(question[i][j]!=0)
                num_hint++;
        }
    }
    // 将题目写入CNF文件
    if(WriteToFile(question,num_hint,"ast_sudoku.cnf")!=OK){
        return ERROR;
    }
    // 分配CNF结构体内存
    CNF * cnf = (CNF*)malloc(sizeof(CNF));
    if(!cnf){
        return ERROR;
    }
    // 读取生成的CNF文件
    if(ReadCNFFile(cnf,"ast_sudoku.cnf")!=OK){
        free(cnf);
        return ERROR;
    }
    // 重置递归计数器
    g_recur_cnt=0;
    int* assign = NULL;  // 接收DPLL最终赋值结果
    // 调用最高优化级DPLL求解（flag=3）
    status res = DPLL(cnf, value, &assign, 3, cnf->var_num);
    // 释放CNF相关内存
    DestroyCnf(&cnf->head);
    free(cnf);
    // 求解成功则解码答案并返回
    if(res==OK&&assign!=NULL){
        DecodeSolution(assign,sol_board);
        free(assign);
        return OK;
    }else
        if(assign!=NULL){
        free(assign);
        return ERROR;   
    }
}

/**
 * @brief 回溯法随机生成完整的星形数独解
 * @param board 待填充的数独棋盘
 * @return 填充成功返回OK，失败返回ERROR
 * @details 按顺序遍历空格，随机打乱数字顺序尝试填入，递归回溯直到填满
 */
  status Fill_Complete(int board[SIZE+1][SIZE+1])
{
    for(int i=1;i<=9;i++){
        for(int j=1;j<=9;j++){
            // 找到空格位置
            if(board[i][j]==0){
                int arr[9]={1,2,3,4,5,6,7,8,9};
                // 随机打乱数字顺序，保证生成的解具有随机性
                for(int s=0;s<9;s++){
                    int t = rand()%9;
                    int tmp = arr[s];arr[s]=arr[t];arr[t]=tmp;
                }
                // 依次尝试每个数字
                for(int k=0;k<9;k++){
                    int v = arr[k];
                    // 验证数字合法则填入
                    if(Is_Valid(board,i,j,v)){
                        board[i][j]=v;
                        // 递归填充剩余空格，成功则直接返回
                        if(Fill_Complete(board)==OK){
                            return OK;
                        }
                        // 失败则回溯，撤销当前填入的数字
                        board[i][j]=0;
                    }
                }
                // 所有数字都尝试失败，返回上一层回溯
                return ERROR;
            }
        }
    }
    // 所有格子都填满，生成成功
    return OK;
}

/**
 * @brief 挖洞:从完整答案生成题目
 * @param full_board 完整解棋盘
 * @param puzzle 输出题目棋盘
 * @param hint_mark 标记提示格（true为固定提示数字，不可修改）
 * @param keep_cnt 需要保留的提示数
 */
void DigHole(int full_board[SIZE+1][SIZE+1], int puzzle[SIZE+1][SIZE+1], bool hint_mark[SIZE+1][SIZE+1], int keep_cnt)
{
    // 先将完整解复制到题目棋盘
    memcpy(puzzle, full_board, sizeof(int)*(SIZE+1)*(SIZE+1));
    // 初始化提示标记数组为全false
    memset(hint_mark,0,sizeof(bool)*(SIZE+1)*(SIZE+1));
    int total =81;  // 棋盘总格子数
    int pos[81][2]; // 存储所有81个格子的坐标
    int idx=0;
    // 收集所有格子坐标
    for(int i=1;i<=9;i++)
        for(int j=1;j<=9;j++){
            pos[idx][0]=i;pos[idx][1]=j;idx++;
        }
    // 随机打乱格子顺序，保证挖洞位置随机
    for(int s=0;s<81;s++){
        int t=rand()%81;
        int tr=pos[s][0],tc=pos[s][1];
        pos[s][0]=pos[t][0];pos[s][1]=pos[t][1];
        pos[t][0]=tr;pos[t][1]=tc;
    }
    // 需要挖掉的格子数 = 总数 - 保留提示数
    int del = total - keep_cnt;
    // 按打乱顺序挖掉指定数量的格子
    for(int i=0;i<81 && del>0;i++){
        int r=pos[i][0],c=pos[i][1];
        puzzle[r][c]=0;
        del--;
    }
    // 标记剩余的非空格为固定提示格
    for(int i=1;i<=9;i++)
        for(int j=1;j<=9;j++)
            if(puzzle[i][j]!=0) hint_mark[i][j]=true;
}

/**
 * @brief 玩家游玩星形数独的交互逻辑
 * @param board 游戏盘面（玩家操作的副本）
 * @param isFixed true=提示数字不可修改
 */
void Play_Sudoku(int board[SIZE + 1][SIZE + 1], bool isFixed[SIZE + 1][SIZE + 1])
{
    system("cls");
    PrintBoard(board);
    printf("\n游戏操作:输入 行 列 数字;行输入0退出游玩\n");
    while (1)
    {
        int row, col, val;
        printf("\nrow col val: ");
        scanf("%d", &row);
        // 输入行号为0则退出游玩
        if(row == 0)
        {
            system("cls");
            return;
        }
        scanf("%d%d", &col, &val);
        // 输入范围合法性校验
        if(row < 1 || row > SIZE || col <1 || col > SIZE || val <1 || val > SIZE)
        {
            printf("输入非法！\n");
            continue;
        }
        // 检查是否为固定提示数字，禁止修改
        if(isFixed[row][col])
        {
            printf("该位置是题目提示数字,禁止修改！\n");
            continue;
        }
        // 检查填入数字是否符合数独规则
        if(!Is_Valid(board, row, col, val))
        {
            printf("填入数字违反星形数独规则！\n");
            continue;
        }
        // 合法则填入数字，刷新界面
        board[row][col] = val;
        system("cls");
        PrintBoard(board);
        printf("\n游戏操作:输入 行 列 数字;行输入0退出游玩\n");
    }
}

/**
 * @brief 星形数独交互总入口 X_Sudoku
 * @details 功能菜单：case1生成题目 | case2开始游玩 | case3查看原题+答案 | case0返回上级菜单
 */
void X_Sudoku()
{
    // 初始化随机数种子
    srand((unsigned int)time(NULL));
    system("cls");
    printf("\n--------星形数独菜单--------\n");
    bool isFixed[SIZE + 1][SIZE + 1];     // 标记固定提示格
    int fullAns[SIZE + 1][SIZE + 1];      // 完整标准答案棋盘
    int puzzleBoard[SIZE + 1][SIZE + 1];  // 挖洞后的题目棋盘
    int gameBoard[SIZE + 1][SIZE + 1];    // 玩家游玩的棋盘副本
    // DPLL求解数组,2代表未赋值
    int dpll_value[SIZE*SIZE*SIZE + 2];
    for(int i = 1; i <= SIZE*SIZE*SIZE; i++)
    {
        dpll_value[i] = 2;
    }
    int op = 1;       // 用户操作选择
    int hasGen = 0;   // 标记是否已生成题目
    while(op != 0)
    {
        printf("\n--------星形数独菜单--------\n");
        printf("1:生成星形数独题目\n");
        printf("2:开始游玩填数\n");
        printf("3:查看原题与标准答案\n");
        printf("0:返回上级菜单\n");
        printf("请选择操作:");
        scanf("%d",&op);
        system("cls");
        printf("\n--------星形数独菜单--------\n");
        switch(op)
        {
        case 1:
        {
            int hintNum;
            printf("输入提示数字个数 [18~81]:");
            scanf("%d",&hintNum);
            // 输入范围校验
            while(hintNum <18 || hintNum >81)
            {
                printf("数值非法,请输入18-81:");
                scanf("%d",&hintNum);
            }
            // 清空答案棋盘
            memset(fullAns,0,sizeof(fullAns));
            // 回溯生成完整星形数独解
            status retFill = Fill_Complete(fullAns);
            if(retFill != OK)
            {
                printf("生成完整星形数独失败！\n");
                hasGen = 0;
                break;
            }
            // 挖洞生成题目
            DigHole(fullAns, puzzleBoard, isFixed, hintNum);
            // 复制题目到游戏棋盘
            memcpy(gameBoard, puzzleBoard, sizeof(gameBoard));
            printf("星形数独生成成功！提示数:%d\n",hintNum);
            hasGen = 1;
            break;
        }
        case 2:
        {
            // 未生成题目则提示
            if(!hasGen)
            {
                printf("请先生成星形数独！\n");
                break;
            }
            // 进入游玩模式
            Play_Sudoku(gameBoard, isFixed);
            system("cls");
            printf("\n--------星形数独菜单--------\n");
            break;
        }
        case 3:
        {
            // 未生成题目则提示
            if(!hasGen)
            {
                printf("请先生成星形数独！\n");
                break;
            }
            // 打印题目
            printf("----------星形数独题目----------\n");
            PrintBoard(puzzleBoard);
            // 调用DPLL求解并打印答案
            printf("\n----------DPLL求解标准答案----------\n");
            int solveOut[SIZE+1][SIZE+1];
            if(Solve_AstSudoku(puzzleBoard, solveOut, dpll_value) == OK)
            {
                PrintBoard(solveOut);
            }
            else
            {
                printf("求解失败,无答案！\n");
            }
            break;
        }
        case 0:
        {
            system("cls");
            printf("退出星形数独模块\n");
            break;
        }
        default:
        {
            printf("无效输入！\n");
            break;
        }
        }
    }
}

