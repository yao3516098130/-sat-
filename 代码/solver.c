#include "sat.h"
// DPLL递归调用计数
extern long long g_recur_cnt;

/**
 * @brief 判断子句是否为单子句（仅含1个文字）
 * @param lit_head 子句内文字链表头
 * @return TRUE:是单子句，FALSE:不是
 */
status IsUnitClause(Literal* lit_head)
{
    // 不为空，并且下一个节点为空 → 只有一个文字
    if(lit_head != NULL && lit_head->next == NULL)
    {
        return TRUE;
    }
    return FALSE;
}
/**
 * @brief 遍历整个CNF公式，寻找第一个单子句
 * @param clause_head 子句链表头
 * @return 单子句的文字值；0代表不存在单子句
 */
int FindUnitClause(Clause* clause_head)
{
    Clause* p = clause_head;
    while(p != NULL)
    {
        if(IsUnitClause(p->head) == TRUE)
        {
            return p->head->val;
        }
        p = p->next;
    }
    return 0;
}
/**
 * @brief 单元传播化简：给定文字lit为真，化简整个CNF
 * 1. 所有包含 lit 的子句直接满足，删除整条子句
 * 2. 所有包含 -lit 的子句，删除文字 -lit
 * @param clause_head 子句链表的指针的指针（需要修改链表头）
 * @param lit 当前赋值为真的文字
 */
void Simplify(Clause** clause_head, int lit)
{
    Clause* pre = NULL;
    Clause* p = *clause_head;
    while(p != NULL)
    {
        int clause_remove = 0;
        Literal* lit_pre = NULL;
        Literal* q = p->head;
        while(q != NULL)
        {
            // 当前子句包含lit，该子句恒真，整条删除
            if(q->val == lit)
            {
                if(pre == NULL)
                {
                    *clause_head = p->next;
                }
                else
                {
                    pre->next = p->next;
                }
                // 释放该子句全部文字节点
                Literal* tmp_lit = p->head;
                while(tmp_lit != NULL)
                {
                    Literal* t = tmp_lit;
                    tmp_lit = tmp_lit->next;
                    free(t);
                }
                free(p);
                p = (pre == NULL) ? (*clause_head) : pre->next;
                clause_remove = 1;
                break;
            }
            // 子句包含lit的否定，删除该文字
            else if(q->val == -lit)
            {
                if(lit_pre == NULL)
                {
                    p->head = q->next;
                }
                else
                {
                    lit_pre->next = q->next;
                }
                Literal* t = q;
                q = (lit_pre == NULL) ? p->head : lit_pre->next;
                free(t);
            }
            else
            {
                lit_pre = q;
                q = q->next;
            }
        }
        // 没有删除子句，则pre后移
        if(!clause_remove)
        {
            pre = p;
            p = p->next;
        }
    }
}
/**
 * @brief 复制一条文字链表
 * @param src 源文字链表头
 * @return 新链表头，失败返回NULL
 */
Literal* CopyLiteralList(Literal* src)
{
    if(src == NULL) return NULL;
    Literal* head = NULL;
    Literal* tail = NULL;
    Literal* p = src;
    while(p != NULL)
    {
        Literal* new_node = create_literal(p->val);
        if(new_node == NULL) return NULL;
        if(head == NULL)
        {
            head = new_node;
            tail = new_node;
        }
        else
        {
            tail->next = new_node;
            tail = new_node;
        }
        p = p->next;
    }
    return head;
}
/**
 * @brief 复制整条子句链表（复制完整CNF公式）
 * @param src 源子句链表头
 * @return 新子句链表头，失败释放已分配内存返回NULL
 */
Clause* CopyClauseList(Clause* src)
{
    if(src == NULL) return NULL;
    Clause* head = NULL;
    Clause* tail = NULL;
    Clause* p = src;
    while(p != NULL)
    {
        Clause* new_cls = create_clause();
        if(new_cls == NULL)
        {
            DestroyCnf(&head);
            return NULL;
        }
        new_cls->head = CopyLiteralList(p->head);
        new_cls->next = NULL;
        if(head == NULL)
        {
            head = new_cls;
            tail = new_cls;
        }
        else
        {
            tail->next = new_cls;
            tail = new_cls;
        }
        p = p->next;
    }
    return head;
}
/**
 * @brief 构造一个单子句：只包含一个文字lit
 * @param lit 文字值
 * @return 新建子句指针，失败返回NULL
 */
Clause* MakeUnitClause(int lit)
{
    Clause* cls = create_clause();
    if(cls == NULL)
        return NULL;
    Literal* l = create_literal(lit);
    if(l == NULL){
        free(cls);
        return NULL;
    }
    cls->head = l;
    cls->next = NULL;
    return cls;
}
/**
 * @brief 检测CNF中是否存在空子句（空子句代表冲突，公式不可满足）
 * @param clause_head 子句链表头
 * @return TRUE 存在空子句；FALSE 无空子句
 */
status EmptyClause(Clause* clause_head)
{
    Clause* p = clause_head;
    while(p != NULL)
    {
        if(p->head == NULL)
        {
            return TRUE;
        }
        p = p->next;
    }
    return FALSE;
}
/**
 * @brief 判断CNF公式是否全部满足：子句链表为空代表全部子句满足
 * @param clause_head 子句链表头
 * @return OK 全部满足；ERROR 还有剩余子句
 */
status Satisfy(Clause* clause_head)
{
    if(clause_head == NULL)
        return OK;
    return ERROR;
}
/**
 * @brief 分支选择策略1：简单选取第一个子句的第一个文字
 * @param cnf 当前CNF公式
 * @return 选中待分支的文字；0异常
 */
int ChooseLiteral_1(CNF* cnf)
{
    if(cnf == NULL || cnf->head == NULL){
        return 0;
    }
    return cnf->head->head->val;
}
/**
 * @brief 分支选择策略2 JW策略：统计文字出现频次，选出现最多文字（返回带符号文字）
 * @param cnf 当前CNF公式
 * @return 选中待分支文字
 */
int ChooseLiteral_2(CNF* cnf)
{
    int var_cnt = cnf->var_num;
    int* count = (int*)calloc(var_cnt * 2 + 10, sizeof(int));
    if(count == NULL) return 0;

    Clause* cp = cnf->head;
    while(cp != NULL)
    {
        Literal* lp = cp->head;
        while(lp != NULL)
        {
            int v = lp->val;
            int idx;
            if(v > 0)
                idx = v;
            else
                idx = var_cnt - v;
            count[idx]++;
            lp = lp->next;
        }
        cp = cp->next;
    }

    int max_cnt = 0;
    int select_lit = 0;
    //优先正文字
    for(int i = 1; i <= var_cnt; i++)
    {
        if(count[i] > max_cnt)
        {
            max_cnt = count[i];
            select_lit = i;
        }
    }
    //再看负文字
    for(int i = var_cnt + 1; i <= var_cnt*2; i++)
    {
        if(count[i] > max_cnt)
        {
            max_cnt = count[i];
            select_lit = -(i - var_cnt);
        }
    }
    free(count);
    return select_lit;
}

/**
 * @brief 分支选择策略3 MOMS：最短子句中出现次数最多文字
 * @param cnf 当前CNF公式
 * @return 选中待分支文字
 */
int ChooseLiteral_3(CNF* cnf)
{
    int var_cnt = cnf->var_num;
    Clause* p = cnf->head;
    int min_size = 0x7fffffff;
    Clause* min_cls = NULL;
    // 找到长度最短的子句
    while(p != NULL)
    {
        int sz = 0;
        Literal* q = p->head;
        while(q != NULL)
        {
            sz++;
            q = q->next;
        }
        if(sz < min_size)
        {
            min_size = sz;
            min_cls = p;
        }
        p = p->next;
    }
    if(min_cls == NULL)
        return 0;
    // 在最短子句内部统计文字频次
    int* cnt = (int*)calloc(var_cnt*2+4, sizeof(int));
    Literal* q = min_cls->head;
    while(q != NULL)
    {
        cnt[q->val + var_cnt]++;
        q = q->next;
    }
    int maxc = 0;
    int lit = 0;
    for(int i = 0; i < var_cnt*2+4; i++)
    {
        if(cnt[i] > maxc)
        {
            maxc = cnt[i];
            lit = i - var_cnt;
        }
    }
    free(cnt);
    return lit;
}

/**
 * @brief DPLL递归求解核心函数
 * @param cnf 当前递归使用的CNF公式副本
 * @param assignment 入参：上层传递赋值数组，2未赋值，1真，0假
 * @param out_assignment 输出参数：SAT返回动态分配完整赋值数组；UNSAT置NULL，调用方free
 * @param flag 分支选择策略：1基线；2‑JW；3‑MOMS
 * @param var_max CNF最大变量编号
 * @return OK(SAT可满足) / ERROR(UNSAT不可满足)
 */
status DPLL(CNF* cnf, int* assignment, int** out_assignment, int flag, int var_max) {
    g_recur_cnt++;
    *out_assignment = NULL;
    // ==========单元传播==========
    int unit_lit = FindUnitClause(cnf->head);
    while(unit_lit != 0) {
        int var = abs(unit_lit);
        assignment[var] = (unit_lit > 0) ? 1 : 0;
        Simplify(&cnf->head, unit_lit);
        if(EmptyClause(cnf->head) == TRUE) {
            return ERROR;
        }
        unit_lit = FindUnitClause(cnf->head);
    }
    if(Satisfy(cnf->head) == OK) {
        int* res = (int*)malloc(sizeof(int) * (var_max + 2));
        memcpy(res, assignment, sizeof(int)*(var_max+2));
        *out_assignment = res;
        return OK;
    }
    // =========选择分支文字=========
    int lit;
    if(flag ==1) lit = ChooseLiteral_1(cnf);
    else if(flag ==2) lit = ChooseLiteral_2(cnf);
    else lit = ChooseLiteral_3(cnf);
    if(lit == 0) return ERROR;

    // =========分支一 lit为真 =========
    int* assign_copy1 = (int*)malloc(sizeof(int)*(var_max+2));
    memcpy(assign_copy1, assignment, sizeof(int)*(var_max+2));
    CNF cnf1;
    cnf1.var_num = cnf->var_num;
    cnf1.clause_num = cnf->clause_num + 1;
    cnf1.head = CopyClauseList(cnf->head);
    Clause* u1 = MakeUnitClause(lit);
    u1->next = cnf1.head;
    cnf1.head = u1;

    int* assign1 = NULL;
    status res1 = DPLL(&cnf1, assign_copy1, &assign1, flag, var_max);
    free(assign_copy1);
    DestroyCnf(&cnf1.head);
    if(res1 == OK) {
        *out_assignment = assign1;
        return OK;
    }else{
        if(assign1 != NULL) free(assign1);
    }

    // =========分支二 lit为假 =========
    int* assign_copy2 = (int*)malloc(sizeof(int)*(var_max+2));
    memcpy(assign_copy2, assignment, sizeof(int)*(var_max+2));
    CNF cnf2;
    cnf2.var_num = cnf->var_num;
    cnf2.clause_num = cnf->clause_num +1;
    cnf2.head = CopyClauseList(cnf->head);
    Clause* u2 = MakeUnitClause(-lit);
    u2->next = cnf2.head;
    cnf2.head = u2;

    int* assign2 = NULL;
    status res2 = DPLL(&cnf2, assign_copy2, &assign2, flag, var_max);
    free(assign_copy2);
    DestroyCnf(&cnf2.head);
    if(res2 == OK){
        *out_assignment = assign2;
        return OK;
    }else{
        if(assign2 != NULL) free(assign2);
    }
    return ERROR;
}
