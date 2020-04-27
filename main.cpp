//inspired by https://github.com/carld/homelisp
/* reads a symbolic expression into memory
 * Copyright (C) 2015 A. Carl Douglas
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>

 /* structures and functions for representing symbolic expressions .
  * an object structure can store either an atomic type:
  *   a symbol or a number, or a Cons cell.
  *
  * copyright (C) 2015 A. Carl Douglas
  */
#include <stdlib.h>
enum { PAIR = 1, NUMBER = 2, SYMBOL = 3, OPERATOR = 4, POINTER = 5, STRING = 6 };
typedef struct object OBJECT;
typedef OBJECT * (prim_op)(OBJECT *, OBJECT *);

struct object {
	int type;
	union {
		const char * symbol;
		struct { int integer; int fraction; } number;
		struct { OBJECT * car; OBJECT * cdr; } pair;
		prim_op * primitive;
		void * ptr;
		char * string;
	} value;
	size_t size; /* for string length etc */
};

extern OBJECT _NIL;
extern OBJECT _TRUE;
extern OBJECT _FALSE;

#define NIL     ((OBJECT *) &_NIL) 
#define TRUE    ((OBJECT *) &_TRUE)
#define FALSE   ((OBJECT *) &_FALSE)

#define is_nil(x)       (x == NIL)
#define is_true(x)      (x == TRUE)
#define is_pair(x)      (x->type == PAIR)
#define is_atom(x)      (x->type == SYMBOL || x->type == NUMBER)
#define symbol_name(x)  (x->value.symbol)
#define integer(x)      (x->value.number.integer)
#define string(x)       (x->value.string)
#define pointer(x)      (x->value.ptr)
#define object_type(x)  (x->type)
#define _cdr(x)         (x->value.pair.cdr)
#define _car(x)         (x->value.pair.car)
#define _rplaca(x,y)    (x->value.pair.car = y)
#define _rplacd(x,y)    (x->value.pair.cdr = y)

#define _bind(ex,va,en)    _cons(_cons(ex, _cons(va, NIL)), en)

OBJECT * _object_malloc(int type);
OBJECT * _cons(OBJECT *car, OBJECT *cdr);

/* in place reverse */
OBJECT * _reverse_in_place(OBJECT *expr);

/*
 * (def assoc (x y)
 *   (cond ((eq (caar y) x) (cadar y))
 *           (true (assoc x (cdr y)))))
 */
OBJECT * _lookup(OBJECT *expr /* x */, OBJECT *env /* y */);

OBJECT * make_number_i(int integer);

OBJECT * make_number(const char *token);

OBJECT * make_primitive(prim_op *pp);

/* this helps passing around FILE * etc */
OBJECT * make_pointer(void * ptr);

OBJECT * make_symbol(const char *symbol);

/* if the length parameter is less than the length of
 * the string the actual string length will be used
 */
OBJECT * make_string(const char *str, size_t length);

/* warning this mutates the first parameter */
OBJECT *_append(OBJECT *exp1, OBJECT *exp2);

const char * _strcat_alloc(const char *str1, const char *str2);
OBJECT *string_cat(OBJECT *, OBJECT *);

OBJECT * _evlis(OBJECT *expr, OBJECT *environ);

OBJECT * _eval(OBJECT *expr, OBJECT *environ);

OBJECT * _read(OBJECT *port, int eof_exit);

OBJECT * _print(OBJECT *exp, OBJECT *);

OBJECT * debug(OBJECT *exp);

#define debugf(x,m) printf("%s:%d --- %s\n", __FILE__, __LINE__, m); debug(x)

enum { T_NONE = -1, T_LPAREN = 0, T_RPAREN = 1, T_SYMBOL = 2, T_NUMBER = 3, T_STRING = 4, T_QUOTE = 5, T_COMMENT = 6 };

/* returns token type on success, -1 on error or EOF
 * lexer states and transitions are in the diagram lexer-state.dot */
int get_token(FILE *fp, char *token, int exit_on_eof) {
	int state = T_NONE;
	int ch;
top:
	ch = fgetc(fp);
	if (ch == EOF) {
		if (exit_on_eof == 1) {
			printf("Exiting.\n");
			exit(0);
		}
		return T_NONE;
	}
	switch (state) {
	case T_NONE:
		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') { /* ignore leading whitespace */
		}
		else if (ch == '(') {
			*token++ = ch; *token = '\0'; return T_LPAREN;
		}
		else if (ch == ')') {
			*token++ = ch; *token = '\0'; return T_RPAREN;
		}
		else if (ch == '\'') {
			*token++ = ch; *token = '\0'; return T_QUOTE;
		}
		else if (ch == ';') {
			state = T_COMMENT;
		}
		else if (strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_:-+=*&^%$#@!~'<>/?`|", ch)) {
			*token++ = ch; *token = '\0';
			state = T_SYMBOL;
		}   
		else if (strchr("0123456789", ch)) {
			*token++ = ch; *token = '\0';
			state = T_NUMBER;
		}
		else if (ch == '"') {
			state = T_STRING;
		}
		else {
			printf("Error, unexpected '%c' in state %d\n", ch, state);
			exit(1);
		}
		break;
	case T_COMMENT:
		if (strchr("\r\n", ch) /* hit a new line */) {
			state = T_NONE;
		}
		break;
	case T_SYMBOL:
		if (strchr("()' \t\r\n", ch) /* hit whitespace or syntax */) {
			ungetc(ch, fp);
			return T_SYMBOL;
		}
		*token++ = ch; *token = '\0';
		break;
	case T_NUMBER:
		if (strchr("0123456789", ch) == NULL /* not a number */) {
			ungetc(ch, fp);
			return T_NUMBER;
		}
		*token++ = ch; *token = '\0';
		break;
	case T_STRING:
		if (ch == '"') {
			*token = '\0';
			return T_STRING;
		}
		*token++ = ch; *token = '\0';
		break;
	}
	goto top;
}


OBJECT * make_symbol(const char *symbol) {
	OBJECT *obj = NULL; // _interned_syms;
	//size_t len = strlen(symbol) + 1;
	//char * storage = 0;
	//for (; obj != NIL; obj = _cdr(obj)) {
	//	if (strcmp(symbol, symbol_name(_car(obj))) == 0) {
	//		return _car(obj);
	//	}
	//}
	//storage = GC_MALLOC(len);
	//memcpy(storage, symbol, len);
	//obj = _object_malloc(SYMBOL);
	//obj->value.symbol = storage;
	//obj->size = len;
	//_interned_syms = _cons(obj, _interned_syms);
	return obj;
}

OBJECT * _append(OBJECT *exp1, OBJECT *exp2) {
	if (exp1 != NIL) {
		OBJECT *tmp = exp1;
		for (; _cdr(tmp) != NIL; tmp = _cdr(tmp))
			; /* walk to end of list */
		_rplacd(tmp, exp2);
		return exp1;
	}
	return exp2;
}

OBJECT * _read(OBJECT *port, int eof_exit) {
	OBJECT *expr_stack = NIL;
	OBJECT *quote_stack = NIL;
	OBJECT *expr = NIL;
	OBJECT *obj = NIL;
	char token[80];
	int tok_type;
	for (; ; ) {
		tok_type = get_token((FILE *)pointer(port), token, eof_exit);
		if (tok_type < 0) break; /* EOF */
		if (tok_type == T_LPAREN) 
		{
			expr_stack = _cons(expr, expr_stack); /* remember the current list */
			expr = NIL; /* start a new list */
			continue; /* go to top and read the first object in the expression */
		}
		else if (tok_type == T_RPAREN) 
		{
			obj = _cons(expr, NIL); /* the new list */
			expr = _car(expr_stack); /* pop the outer expression off the stack */
			expr_stack = _cdr(expr_stack);
		}
		else if (tok_type == T_STRING) 
		{
			obj = _cons(make_string(token, 0), NIL);
		}
		else if (tok_type == T_NUMBER) 
		{
			obj = _cons(make_number(token), NIL);
		}
		else if (tok_type == T_SYMBOL) 
		{
			obj = _cons(make_symbol(token), NIL);
		}
		//else if (tok_type == T_QUOTE) {
		//	obj = _cons(_cons(make_symbol("quote"), NIL), NIL); /* insert quote procedure call */
		//	quote_stack = _cons(obj, quote_stack); /* keep track of quotes */
		//	expr = _append(expr, obj); /* append (quote) to the current expression */
		//	continue; /* go to the top and read the quoted object */
		//}
		//else {
		//	printf("unrecognised token: '%s'\n", token); exit(1);
		//}
		expr = _append(expr, obj); /* append the object to the current expression */
		if (expr_stack == NIL) /* an expression as been read */
			break;
	}
	/*  finish expanding quotes: a 'b c  ->  a (quote b) c  ...  a '(b) c  ->  a (quote (b)) c   */
	//for (; quote_stack != NIL; quote_stack = _cdr(quote_stack)) {
	//	_rplacd(_car(_car(quote_stack)), _cdr(_car(quote_stack))); /* a (quote) b c  ->  a (quote b) c */
	//	_rplacd(_car(quote_stack), _cdr(_cdr(_car(quote_stack))));
	//	_rplacd(_cdr(_car(quote_stack)), NIL);
	//}
	return expr != NIL ? _car(expr) : NIL;
}

OBJECT _NIL = { PAIR,   { NULL } };
OBJECT _TRUE = { SYMBOL, { "#t" } };
OBJECT _FALSE = { SYMBOL, { "#f" } };

OBJECT *_interned_syms = NIL;

OBJECT * _object_malloc(int type) {
	OBJECT *obj = static_cast<OBJECT *>(malloc(sizeof(OBJECT)));
	obj->type = type; return obj;
}

OBJECT * _cons(OBJECT *car, OBJECT *cdr) {
	OBJECT *obj = _object_malloc(PAIR);
	obj->value.pair.car = car;
	obj->value.pair.cdr = cdr;
	return obj;
}

/* in place reverse */
OBJECT * _reverse_in_place(OBJECT *expr) {
	OBJECT *tmp, *revexpr = NIL;
	while (expr != NIL) {
		tmp = _cdr(expr);
		_rplacd(expr, revexpr);
		revexpr = expr;
		expr = tmp;
	}
	return revexpr;
}

OBJECT * make_number(const char *token) {
	OBJECT *obj = _object_malloc(NUMBER);
	char *dec = std::strchr((char *)token, '.');
	if (dec) {
		*dec = '\0';
		obj->value.number.fraction = atoi(dec + 1);
	}
	obj->value.number.integer = atoi(token);
	return obj;
}

OBJECT * make_pointer(void * ptr) {
	OBJECT *obj = _object_malloc(POINTER);
	obj->value.ptr = ptr;
	return obj;
}

OBJECT * make_string(const char *str, size_t length) {
	OBJECT *obj = _object_malloc(STRING);
	size_t len = strlen(str) + 1;
	obj->size = length > len ? length : len;
	//obj->value.string = GC_MALLOC(obj->size);
	obj->value.string = static_cast<char *>(malloc(obj->size));
	memcpy(obj->value.string, str, len);
	return obj;
}

#define ERR(x) printf("%s:%d \n", __FILE__, __LINE__); debug(x);

char * obj_inspector(OBJECT *obj) {
	char *str = static_cast<char*>(malloc(256));
	if (obj == NIL) {
		snprintf(str, 255, "[%p NIL]", (void *)obj);
		return str;
	}
	switch (object_type(obj)) {
	case PAIR:   snprintf(str, 255, "[%p, CONS %p %p %s] NIL=%p",
		(void *)obj, (void *)_car(obj), (void *)_cdr(obj),
		_cdr(obj) == NIL ? "(NIL)" : "",
		(void *)NIL); break;
	case SYMBOL: snprintf(str, 255, "[%p, SYMBOL %s]",
		(void *)obj, obj->value.symbol); break;
	case STRING: snprintf(str, 255, "[%p, STRING %s]",
		(void *)obj, obj->value.string); break;
	case NUMBER: snprintf(str, 255, "[%p, NUMBER %d.%d]",
		(void *)obj, obj->value.number.integer, obj->value.number.fraction); break;
	case OPERATOR: snprintf(str, 255, "[%p, OPERATOR ]",
		(void*)obj); break;
	default: abort();
	}
	return str;
}

void indent_print_obj(OBJECT *obj, int indent) {
	int i = 0;
	for (i = 0; i < indent; i++) printf("  ");
	printf("%s\n", obj_inspector(obj));
}

OBJECT * debug(OBJECT *exp) {
	OBJECT *expr_stack = NIL;
	int indent = 0;
next:
	indent_print_obj(exp, indent);
	if (exp == NIL)
		goto pop_frame;

	if (object_type(exp) == PAIR) {
		expr_stack = _cons(exp, expr_stack);
		exp = _car(exp);
		indent++;
		goto next;
	}
	else if (object_type(exp) == SYMBOL
		|| object_type(exp) == NUMBER
		|| object_type(exp) == OPERATOR
		|| object_type(exp) == STRING) {

	pop_frame:
		if (expr_stack != NIL) {
			exp = _cdr(_car(expr_stack));
			expr_stack = _cdr(expr_stack);
			indent--;
			if (exp == NIL)
				goto pop_frame;
		}
		else {
			exp = NIL;
		}
	}
	if (exp != NIL) goto next;

	printf("\n");
	return NIL;
}



int main()
{
	//std::string filename = "init.lsp";
	//std::string filename = "small.lsp";
	//std::fstream s(filename);

	OBJECT *port;
	FILE *fp;
	const char filename[] = "init.lsp";
	int no_exit = 0; /* do not exit on EOF */
	fp = fopen(filename, "r");
	if (fp == NULL) 
		return -1;
	port = make_pointer(fp);

	
	OBJECT *exp = _read(port, no_exit);
	debug(exp);
	fclose(fp);

	/*
	std::string filename = "small.lsp";

	std::fstream s(filename);
	if (s.is_open()) {
		std::stringstream ss;
		SExp exp = readExpression(s);
		print(std::cout, exp);
		
		std::cout.flush();
	}
	*/
} 