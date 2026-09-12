#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<windows.h>
#include<math.h>
#include<stdbool.h>
#include<ctype.h>
//常量
#define TRUE 1
#define FALSE 0
#define SIZE 9

#undef OK
#undef ERROR
typedef enum {
    OK,
    ERROR
} status;
typedef struct Literal{
    int val;
    struct Literal *next;
}Literal;
typedef struct Clause{
    Literal *head;
    struct Clause *next;
}Clause;
typedef struct CNF{
    int var_num;
    int clause_num;
    Clause *head;
}CNF;
// 全局变量
extern long long g_recur_cnt;
//======== cnfparser 解析模块 ========
Literal *create_literal(int val);
Clause *create_clause();
status DestroyCnf(Clause **cl);
status PrintCnf(CNF *cnf);
status ReadCNFFile (CNF *cnf, const char *filename);
//======== solver DPLL模块 ========
status DPLL(CNF* cnf, int* assignment, int** out_assignment, int flag, int var_max);
status IsUnitClause(Literal* lit_head);
int FindUnitClause(Clause* clause_head);
status EmptyClause(Clause* clause_head);
status Satisfy(Clause* clause_head);
int ChooseLiteral_1(CNF* cnf);
int ChooseLiteral_2(CNF* cnf);
int ChooseLiteral_3(CNF* cnf);
void Simplify(Clause **cl_head, int lit);
Literal* CopyLiteralList(Literal *src);
Clause* CopyClauseList(Clause *src);
Clause* MakeUnitClause(int lit);
//======== display交互模块 ========
void DisPlay(void);
void PrintMenu(void);status 
SaveResult(const char *outFile, status res_status, int var_max, int assign[], double t_base, double t_opt);
//======== 星形数独模块 ========
status Is_Valid(int board[SIZE+1][SIZE+1],int row,int col,int num);
status WriteToFile(int board[SIZE+1][SIZE+1], int num_hint, const char* filename);
void DecodeSolution(int value[],int out_board[SIZE+1][SIZE+1]);
void PrintBoard(int board[SIZE+1][SIZE+1]);
status Solve_AstSudoku(int question[SIZE+1][SIZE+1],int sol_board[SIZE+1][SIZE+1],int value[]);
status Fill_Complete(int board[SIZE+1][SIZE+1]);
void DigHole(int full_board[SIZE+1][SIZE+1], int puzzle[SIZE+1][SIZE+1], bool hint_mark[SIZE+1][SIZE+1], int keep_cnt);
void X_Sudoku();
void Play_Sudoku(int board[SIZE + 1][SIZE + 1], bool isFixed[SIZE + 1][SIZE + 1]);
