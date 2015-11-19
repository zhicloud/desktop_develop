#include <stdlib.h>
#include <stdio.h>
#include "ringlist.h"                                                                                                                                                                  
#define OVERFLOW -1
#define ERROR    -2
#define OK       1
#define true     1
#define false    0
 
void list_init(dulinklist *l)
{ /* 产生空的双向循环链表l */
  *l=(dulinklist)malloc(sizeof(dulnode));
  if (*l)
  {
	  (*l)->next = (*l)->prior = *l;
	  (*l)->data = NULL;
  }
  else
    exit(OVERFLOW);
}
void list_destory(dulinklist *l)
{
  /* 操作结果：销毁双向循环链表l */
  dulinklist q,p=(*l)->next; /* p指向第一个结点 */
  while(p!=*l) /* p没到表头 */
  {
    q=p->next;
    free(p);
    p=q;
  }
  free(*l);
  *l=NULL;
}
void list_clear(dulinklist l) /* 不改变l */
{ /* 初始条件：l已存在。操作结果：将l重置为空表 */
  dulinklist q,p=l->next; /* p指向第一个结点 */
  while(p!=l) /* p没到表头 */
  {
    q=p->next;
    free(p);
    p=q;
  }
  l->next=l->prior=l; /* 头结点的两个指针域均指向自身 */
}
int list_empty(dulinklist l)
{ /* 初始条件：线性表l已存在。操作结果：若l为空表，则返回true，否则返回false */
  if(l->next==l&&l->prior==l)
    return true;
  else
    return false;
}
int list_length(dulinklist l)
{ /* 初始条件：l已存在。操作结果：返回l中数据元素个数 */
  int i=0;
  dulinklist p=l->next; /* p指向第一个结点 */
  while(p!=l) /* p没到表头 */
  {
    i++;
    p=p->next;
  }
  return i;
}
int list_getelem(dulinklist l,int i,elemtype *e)
{ /* 当第i个元素存在时，其值赋给e并返回OK，否则返回ERROR */
  int j=1; /* j为计数器 */
  dulinklist p=l->next; /* p指向第一个结点 */
  do 
  {
	 if (j == i)
	 {
		 *e = p->data;
	 }
	 j++;
	 p = p->next;
  } while (p != l);
  return OK;
}
 
int list_locate_elem(dulinklist l,elemtype e,int(*compare)(elemtype,elemtype))
{ /* 初始条件：l已存在，compare()是数据元素判定函数 */
  /* 操作结果：返回l中第1个与e满足关系compare()的数据元素的位序。 */
  /*           若这样的数据元素不存在，则返回值为0 */
  int i=0;
  dulinklist p=l->next; /* p指向第1个元素 */
  while(p!=l)
  {
    i++;
    if(compare(p->data,e)) /* 找到这样的数据元素 */
      return i;
    p=p->next;
  }
  return 0;
}
 
struct dulnode* list_priorelem(dulinklist l, elemtype cur_e, elemtype *pre_e)
{ /* 操作结果：若cur_e是l的数据元素，且不是第一个，则用pre_e返回它的前驱， */
  /*           否则操作失败，pre_e无定义 */
  dulinklist p=l->prior; /* p指向第prior个元素 */
  do /* p没到表头 */
  {
    if(p->data==cur_e)
    {
      *pre_e=p->prior->data;
	  if (*pre_e == NULL)//l->data is null
	  {
		  *pre_e = p->prior->prior->data;
		  return p->prior->prior;
	  }
	  return p->prior;
    }
    p=p->prior;
  } while (p != l);
  return false;
}
 
struct dulnode* list_nextelem(dulinklist l, elemtype cur_e, elemtype *next_e)
{ /* 操作结果：若cur_e是l的数据元素，且不是最后一个，则用next_e返回它的后继， */
  /*           否则操作失败，next_e无定义 */
  dulinklist p=l->next; /* p指向第1个元素 */
  do /* p没到表头 */                                                                                                                                                          
  {
    if(p->data==cur_e)
    {
      *next_e=p->next->data;
	  if (*next_e == NULL)//l->data is null
	  {
		  *next_e = p->next->next->data;
		  return p->next->next;
	  }
	  return p->next;
	}
    p=p->next;
  } while ((p != l));
  return false;
}
 
dulinklist list_get_elemp(dulinklist l,int i) /* 另加 */
{ /* 在双向链表l中返回第i个元素的地址。i为0，返回头结点的地址。若第i个元素不存在，*/
  /* 返回NULL */
  int j;
  dulinklist p=l; /* p指向头结点 */
  if(i<0||i>list_length(l)) /* i值不合法 */
    return NULL;
  for(j=1;j<=i;j++)
    p=p->next;
  return p;
}
int list_insert(dulinklist l,int i,elemtype e)
{ /* 在带头结点的双链循环线性表l中第i个位置之前插入元素e，i的合法值为1≤i≤表长+1 */
  /* 改进算法2.18，否则无法在第表长+1个结点之前插入元素 */
  dulinklist p,s;
  if(i<1||i>list_length(l)+1) /* i值不合法 */
    return ERROR;
  p=list_get_elemp(l,i-1); /* 在l中确定第i个元素前驱的位置指针p */
  if(!p) /* p=NULL,即第i个元素的前驱不存在(设头结点为第1个元素的前驱) */
    return ERROR;
  s=(dulinklist)malloc(sizeof(dulnode));
  if(!s)
    return OVERFLOW;
  s->data=e;
  s->prior=p; /* 在第i-1个元素之后插入 */
  s->next=p->next;
  p->next->prior=s;
  p->next=s;
  return OK;
}
 
int list_delete(dulinklist l,elemtype e)
{ /* 删除带头结点的双链循环线性表l的第i个元素，i的合法值为1≤i≤表长 */
  dulinklist p = l->next;//1
  do 
  {
	  if (p->data == e)
	  {
		  p->prior->next = p->next;//？？？没有考虑链表头？链表尾？
		  p->next->prior = p->prior;
		  free(p);
		  return OK;
	  }
	  p = p->next;
  } while (p != l);
  return OK;                                                                                                                                                                           
}
void list_traverse(dulinklist l,void(*visit)(elemtype))
{ /* 由双链循环线性表l的头结点出发，正序对每个数据元素调用函数visit() */
  dulinklist p=l->next; /* p指向头结点 */
  while(p!=l)
  {
    visit(p->data);
    p=p->next;
  }
  printf("\n");
}
 
void list_traverse_back(dulinklist l,void(*visit)(elemtype))
{ /* 由双链循环线性表l的头结点出发，逆序对每个数据元素调用函数visit()。另加 */
  dulinklist p=l->prior; /* p指向尾结点 */
  while(p!=l)
  {
    visit(p->data);
    p=p->prior;
  }
  printf("\n");
}