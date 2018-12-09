#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// トークンの型を表す値
enum
{
	TK_NUM = 256, // 整数トークン
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
Token tokens[100];

// p が指している文字列をトークンに分割して tokens に保存する
void tokenize(char* p)
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

void dump_tokens(void)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		fprintf(stderr, "ty: %d val: %d input: %s\n", tokens[i].ty, tokens[i].val, tokens[i].input);
	}
}

// エラー報告をするための関数
void error(int i)
{
	fprintf(stderr, "予期せぬトークンです: tokens[%d] %s\n", i, tokens[i].input);
	exit(1);
}

int main(int argc, char** argv)
{
	int i;
	
	if(argc != 2)
	{
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}
	
	
	// トークナイズする
	tokenize(argv[1]);
	
	//dump_tokens();
	
	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");
	
	// 式の最初は数でなければならないので、それをチェックして
	// 最初の mov 命令を出力
	if(tokens[0].ty != TK_NUM)
		error(0);
	printf("  mov rax, %ld\n", tokens[0].val);
	
	// '+ <数>' あるいは '- <数>' というトークンの並びを消費しつつ
	// アセンブリを出力
	
	i = 1;
	while(tokens[i].ty != TK_EOF)
	{
		
		if(tokens[i].ty == '+')
		{
			i++;
			if(tokens[i].ty != TK_NUM)
				error(i);
			printf("  add rax, %d\n", tokens[i].val);
			i++;
			continue;
		}

		if(tokens[i].ty == '-')
		{
			i++;
			if(tokens[i].ty != TK_NUM)
				error(i);
			printf("  sub rax, %d\n", tokens[i].val);
			i++;
			continue;
		}
		error(i);
	}
	
	printf("  ret\n");
	return 0;
}

