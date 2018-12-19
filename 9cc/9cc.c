#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

// トークンの型を表す値
enum
{
	TK_NUM = 255, // 整数トークン
	TK_EOF,       // 入力の終わりを表すトークン
};

// トークンの型
typedef struct
{
	int ty;       // トークンの型
	int val;      // ty が TK_NUM の場合、その数値
	char* input;  // トークン文字列(エラーメッセージ用)
} Token;

// トークナイズした結果のトークン列はこの配列に保存する
// 100 個以上のトークンは来ないものとする
#define TOKENS_MAX 20
static Token tokens[TOKENS_MAX];

static int pos = 0;

typedef struct Node
{
  int ty;            // 演算子か TK_NUM
  struct Node* lhs;  // 左辺
  struct Node* rhs;  // 右辺
  int val;           // ty が TK_NUM の場合のみ使う
} Node;

static Node* new_node(int ty, Node* lhs, Node* rhs)
{
  Node* node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static void error(char* fmt, ...);
static void dump_tokens(void);
static void tokenize(char* p);
static Node* expr(void);
static Node* mul(void);
static Node* term(void);
static Node* new_node_num(int val);
static void gen(Node* node);


static void gen(Node* node)
{
  if(node->ty == TK_NUM)
  {
    printf("  push %d\n", node->val);
    return;
  }
  
  gen(node->lhs);
  gen(node->rhs);
  
  printf("  pop rdi\n");
  printf("  pop rax\n");
  
  switch(node->ty)
  {
    case '+':
      printf("  add rax, rdi\n");
      break;
    case '-':
      printf("  sub rax, rdi\n");
      break;
    case '*':
      printf("  mul rax, rdi\n");
      break;
    case '/':
      printf("  mov rdx, 0\n");
      printf("  div rdi\n");
      break;
  }
  printf("  push rax\n");
}

static Node* new_node_num(int val)
{
  Node* node = malloc(sizeof(Node));
  node->ty = TK_NUM;
  node->val = val;
  return node;
}

static Node* term(void)
{
  if(tokens[pos].ty == TK_NUM)
    return new_node_num(tokens[pos++].val);
  if(tokens[pos].ty == '(')
  {
    pos++;
    Node* node = expr();
    if(tokens[pos].ty != ')')
      error("開きカッコに対応する閉じカッコがありません: %s", tokens[pos].input);
    pos++;
    return node;
  }
  error("数値でも開きカッコでもないトークンです: %s", tokens[pos].input);
}

static Node* mul(void)
{
  Node* lhs = term();
  if(tokens[pos].ty == TK_EOF)
    return lhs;
  if(tokens[pos].ty == '*')
  {
    pos++;
    return new_node('*', lhs, mul());
  }
  if(tokens[pos].ty == '/')
  {
    pos++;
    return new_node('/', lhs, mul());
  }
  return lhs;
}

static Node* expr(void)
{
  Node* lhs = mul();
  if(tokens[pos].ty == TK_EOF)
    return lhs;
  if(tokens[pos].ty == '+')
  {
    pos++;
    return new_node('+', lhs, expr());
  }
  if(tokens[pos].ty == '-')
  {
    pos++;
    return new_node('-', lhs, expr());
  }
  return lhs;
}

// p が指している文字列をトークンに分割して tokens に保存する
static void tokenize(char* p)
{
	int i = 0;
	
	while(*p)
	{
		// 空白文字をスキップ
		if(isspace(*p))
		{
			p++;
			continue;
		}
		
		if(*p == '+' || *p == '-')
		{
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}
		
		if(isdigit(*p))
		{
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}
		
		fprintf(stderr, "トークナイズできません: %s\n", p);
		exit(1);
	}

	tokens[i].ty = TK_EOF;
	tokens[i].input = p;
}

static void dump_tokens(void)
{
	int i;
	for(i = 0; i < TOKENS_MAX; i++)
	{
		fprintf(stderr, "ty: %c val: %d input: %s\n", tokens[i].ty, tokens[i].val, tokens[i].input);
	}
}

// エラー報告をするための関数
static void error(char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  exit(1);
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}
	
	// トークナイズしてパースする
	tokenize(argv[1]);
	//dump_tokens();
	
	Node* node = expr();
	
	
	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");
	
	// 抽象構文木を下りながらコード生成
	gen(node);
	
	// スタックトップに式全体の値が残っているはずなので
	// それを RAX にロードして関数からの返り値とする
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}

