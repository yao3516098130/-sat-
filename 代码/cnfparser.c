// SAT求解器CNF公式核心操作实现
// 功能覆盖：文字/子句节点创建、CNF内存销毁、公式打印、DIMACS格式文件读取
#include "sat.h"

/**
 * @brief 创建一个文字（Literal）节点
 * @param val 文字的数值（正数代表正文字，负数代表负文字）
 * @return 成功返回新节点指针，内存分配失败返回NULL
 */
Literal *create_literal(int val){
    // 为文字节点分配堆内存空间
    Literal *new_literal = (Literal *)malloc(sizeof(Literal));
    // 内存分配失败：打印提示并返回空指针
    if(new_literal == NULL){
        printf("Memory allocation failed!\n");
        return NULL;
    }
    // 初始化文字的取值
    new_literal->val = val;
    // 初始化链表后继指针为空（默认作为尾节点）
    new_literal->next = NULL;
    return new_literal;
}

/**
 * @brief 创建一个子句（Clause）节点
 * @return 成功返回新子句指针，内存分配失败返回NULL
 * @note 子句内部的文字链表初始为空，需后续挂载文字节点
 */
Clause *create_clause(){
    // 为子句节点分配堆内存空间
    Clause *new_clause = (Clause *)malloc(sizeof(Clause));
    // 内存分配失败：打印提示并返回空指针
    if(new_clause == NULL){
        printf("Memory allocation failed!\n");
        return NULL;
    }
    // 初始化子句的文字链表头指针为空
    new_clause->head = NULL;
    // 初始化子句链表后继指针为空（默认作为尾节点）
    new_clause->next = NULL;
    return new_clause;
}

/**
 * @brief 销毁整个CNF公式，释放所有子句与文字的堆内存
 * @param cl 指向CNF子句链表头指针的二级指针
 * @return 执行状态码OK
 * @details 采用双层遍历：外层遍历子句链表，内层遍历每个子句的文字链表，逐个释放节点
 */
status DestroyCnf(Clause **cl){
    // 遍历子句链表，直到所有子句都被释放
    while(*cl!=NULL){
        // 暂存当前待释放的子句节点
        Clause *temp_clause=*cl;
        // 子句链表头指针后移，指向下一个子句
        *cl=(*cl)->next;
        
        // 遍历当前子句内的文字链表，释放所有文字节点
        Literal *p=temp_clause->head;
        while(p!=NULL){
            // 暂存当前待释放的文字节点
            Literal *temp_literal=p;
            // 文字链表指针后移
            p=p->next;
            // 释放当前文字节点内存
            free(temp_literal);
        }
        
        // 释放当前子句节点内存
        free(temp_clause);
    }
    // 置空头指针，避免产生野指针
    *cl=NULL;
    return OK;
}

/**
 * @brief 打印输出CNF公式的完整信息
 * @param cnf 指向待打印的CNF结构体指针
 * @return 打印成功返回OK，CNF为空返回ERROR
 * @note 输出遵循DIMACS格式：先打印变量数与子句数，再逐行打印子句，每行以0结尾
 */
status PrintCnf(CNF *cnf){
    // 空指针校验：CNF为空或无子句时直接返回错误
    if(cnf == NULL || cnf->head == NULL){
        printf("The CNF is empty.\n");
        return ERROR;
    }
    
    // 打印CNF基础信息：变量总数、子句总数
    printf("CNF has %d variables and %d clauses:\n", cnf->var_num, cnf->clause_num);
    
    // 外层遍历：逐个访问所有子句
    Clause *p=cnf->head;
    while(p!=NULL){
        // 内层遍历：打印当前子句内的所有文字
        Literal *q=p->head;
        while(q!=NULL){
            printf("%d ",q->val);
            q=q->next;
        }
        // 每个子句以0作为结束标记（符合DIMACS规范）
        printf("0\n");
        // 移动到下一个子句
        p=p->next;
    }
    return OK;
}

/**
 * @brief 从DIMACS格式文件中读取并构建CNF公式链表
 * @param cnf 指向待填充的CNF结构体指针
 * @param filename 待读取的CNF文件路径
 * @return 读取成功返回OK，读取失败返回ERROR
 * @details 支持文件打开失败3次重试，自动跳过注释行与空行，解析p cnf头部；
 *          出现内存分配错误时自动回滚释放所有已申请内存，防止泄漏
 */
status ReadCNFFile (CNF *cnf, const char *filename) {
    // 防护：传入空CNF直接返回
    if(cnf == NULL){
        return ERROR;
    }
    // 初始化结构体，避免脏数据、悬垂指针
    cnf->var_num = 0;
    cnf->clause_num = 0;
    cnf->head = NULL;

    // 以只读模式打开目标文件
    FILE *fp = fopen(filename, "r");
    char buf[256];
    int retry_count = 0;

    // 最多重试3次，防止无限循环
    while(fp == NULL && retry_count < 3){
        printf("File not found. Please enter a valid filename: ");
        // 限制读取长度防止缓冲区溢出，读取失败直接返回错误
        if(scanf("%255s", buf) != 1){
            return ERROR;
        }
        // 用新输入的文件名重新尝试打开
        fp = fopen(buf, "r");
        retry_count++;
    }
    // 重试后仍无法打开文件，返回错误
    if(fp == NULL){
        return ERROR;
    }

    char line[1024];
    int find_p_line = 0;  // 标记是否找到p cnf格式头部行

    // 逐行读取，跳过注释、空行，找到p cnf 头部
    while(fgets(line, sizeof(line), fp)){
        char *ptr = line;
        // 跳过行首空白字符（空格、制表符、换行符等）
        while(*ptr != '\0' && isspace((unsigned char)*ptr)){
            ptr++;
        }
        // 空行直接跳过
        if(*ptr == '\0'){
            continue;
        }
        // 注释行（以字符'c'开头）直接跳过
        if(*ptr == 'c'){
            continue;
        }
        // 解析 p cnf 变量数 子句数 格式的头部行
        if(*ptr == 'p'){
            int scan_ret = sscanf(line, "p cnf %d %d", &cnf->var_num, &cnf->clause_num);
            // 解析失败（未读到2个整数），关闭文件返回错误
            if(scan_ret != 2){
                fclose(fp);
                return ERROR;
            }
            // 标记已找到合法头部，退出头部查找循环
            find_p_line = 1;
            break;
        }
    }

    // 文件缺少p cnf 头部，格式非法
    if(!find_p_line){
        fclose(fp);
        return ERROR;
    }

    Clause *last_clause = NULL;  // 记录子句链表尾节点，用于尾插法构建链表
    // 按头部声明的子句数量，循环读取所有子句
    for(int i = 0; i < cnf->clause_num; i++){
        // 创建一个新的空子句
        Clause *new_clause = create_clause();
        // 子句创建失败：关闭文件、释放已构建的CNF并返回错误
        if(new_clause == NULL){
            fclose(fp);
            DestroyCnf(&cnf->head);
            return ERROR;
        }
        new_clause->head = NULL;

        Literal *last_Literal = NULL;  // 记录文字链表尾节点，用于尾插法
        int number;

        // 逐个读取文字，直到读到0（子句结束标记）
        while(fscanf(fp,"%d",&number) == 1 && number != 0){
            // 根据读取的数值创建文字节点
            Literal *new_Literal = create_literal(number);
            // 文字创建失败：回滚释放当前子句、已构建CNF，关闭文件并返回错误
            if(new_Literal == NULL){
                // 释放当前未挂载完成的子句内部结点
                Literal *p = new_clause->head;
                while(p != NULL){
                    Literal *tmp = p;
                    p = p->next;
                    free(tmp);
                }
                free(new_clause);
                fclose(fp);
                DestroyCnf(&cnf->head);
                return ERROR;
            }

            // 将新文字插入当前子句的文字链表尾部
            if(new_clause->head==NULL){
                // 子句为空时，新文字作为头节点
                new_clause->head=new_Literal;
            }else{
                // 子句非空时，挂载到尾节点之后
                last_Literal->next=new_Literal;
            }
            // 更新文字链表尾指针
            last_Literal=new_Literal;
        }

        // 将当前子句挂载到整体CNF的子句链表尾部
        if(cnf->head==NULL){
            // CNF为空时，新子句作为头节点
            cnf->head=new_clause;
        }else{
            // CNF非空时，挂载到尾节点之后
            last_clause->next=new_clause;
        }
        // 更新子句链表尾指针
        last_clause=new_clause;
    }

    // 读取完成，关闭文件
    fclose(fp);
    return OK;
}
