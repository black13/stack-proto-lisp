#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>

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

typedef struct object OBJECT;

enum { T_NONE = -1, T_LPAREN = 0, T_RPAREN = 1, T_SYMBOL = 2, T_NUMBER = 3, T_STRING = 4, T_QUOTE = 5, T_COMMENT = 6 };

OBJECT _NIL = { PAIR,   { NULL } };
OBJECT _TRUE = { SYMBOL, { "#t" } };
OBJECT _FALSE = { SYMBOL, { "#f" } };

int get_token(std::istream& in,std::string &token, int exit_on_eof)
{
	int state = T_NONE;
	int ch;
top:
	ch = in.get();
	if (ch == EOF) {
		if (exit_on_eof == 1) {
			std::cout << "Exiting" << std::endl;
			exit(0);
		}
		return T_NONE;
	}
	switch (state) {
	case T_NONE:
		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') { /* ignore leading whitespace */
		}
		else if (ch == '(') {
			token.clear();
			token.push_back(ch);
			return T_LPAREN;
		}
		else if (ch == ')') {
			token.clear();
			token.push_back(ch);
			return T_RPAREN;
		}
		else if (ch == '\'') 
		{
			token.clear();
			token.push_back(ch);
			return T_QUOTE;
		}
		else if (ch == ';') {
			state = T_COMMENT;
		}
		else if (std::strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_:-+=*&^%$#@!~'<>/?`|", ch)) {
			token.clear();
			token.push_back(ch);
			state = T_SYMBOL;
		}
		else if (std::strchr("0123456789", ch)) {
			token.clear();
			token.push_back(ch);
			state = T_NUMBER;
		}
		else if (ch == '"') {
			state = T_STRING;
		}
		else {
			std::cout << "Error, unexpected" << ch << state << std::endl;
			//printf("Error, unexpected '%c' in state %d\n", ch, state);
			exit(1);
		}
		break;
	case T_COMMENT:
		if (std::strchr("\r\n", ch) /* hit a new line */) {
			state = T_NONE;
		}
		break;
	case T_SYMBOL:
		if (std::strchr("()' \t\r\n", ch) /* hit whitespace or syntax */) {
			//ungetc(ch, fp);
			in.unget();
			return T_SYMBOL;
		}
		token.push_back(ch);
		break;
	case T_NUMBER:
		if (strchr("0123456789", ch) == NULL /* not a number */) 
		{
			in.unget();
			return T_NUMBER;
		}
		token.push_back(ch);
		break;
	case T_STRING:
		if (ch == '"') 
		{
			token.clear();
			return T_STRING;
		}
		token.push_back(ch);
		break;
	}
	goto top;
}

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

OBJECT * make_string(const std::string & token, size_t length) {
	OBJECT *obj = _object_malloc(STRING);
	size_t len = strlen(token.c_str()) + 1;
	obj->size = length > len ? length : len;
	obj->value.string = static_cast<char*>(malloc(obj->size));
	memcpy(obj->value.string, token.c_str(), len);
	return obj;
}


OBJECT * make_number(const std::string & token) {
	OBJECT *obj = _object_malloc(NUMBER);

	obj->value.number.integer = atoi(token.c_str());
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

OBJECT * _read(std::istream& in, int eof_exit) {
	OBJECT *expr_stack = NIL;
	OBJECT *quote_stack = NIL;
	OBJECT *expr = NIL;
	OBJECT *obj = NIL;
	//char token[80];
	std::string token;
	int tok_type;
	for (; ; ) {
		tok_type = get_token(in, token, eof_exit);
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
			obj = _cons(make_string(token,0), NIL);
		}
		else if (tok_type == T_QUOTE) 
		{
			exit(-1);
		}
		else 
		{
			std::cout << "unrecognised token:" << token << std::endl; 
			exit(1);
		}
		expr = _append(expr, obj); /* append the object to the current expression */
		if (expr_stack == NIL) /* an expression as been read */
			break;
	}
	/*  finish expanding quotes: a 'b c  ->  a (quote b) c  ...  a '(b) c  ->  a (quote (b)) c   */
	//for (; quote_stack != NIL; quote_stack = _cdr(quote_stack)) 
	//{
	//	_rplacd(_car(_car(quote_stack)), _cdr(_car(quote_stack))); /* a (quote) b c  ->  a (quote b) c */
	//	_rplacd(_car(quote_stack), _cdr(_cdr(_car(quote_stack))));
	//	_rplacd(_cdr(_car(quote_stack)), NIL);
	//}
	return expr != NIL ? _car(expr) : NIL;
}
struct SExp {
	SExp() : type(LIST) {}
	SExp(const std::list<SExp>& l) : type(LIST), list(l) {}
	SExp(const std::string&     s) : type(ATOM), atom(s) {}
	enum { ATOM, LIST } type;
	std::string         atom;
	std::list<SExp>     list;
};

  
static SExp
readExpression(std::istream& in)
{
	std::stack<SExp> stk;
	std::string      tok;

#define APPEND_TOK() \
	if (stk.empty()) return tok; else stk.top().list.push_back(SExp(tok))

	while (char ch = in.get()) {
		switch (ch) {
		case EOF:
			return SExp();
		case ' ': case '\t': case '\n':
			if (tok == "")
				continue;
			else
				APPEND_TOK();
			tok = "";
			break;
		case '"':
			do { tok.push_back(ch); } while ((ch = in.get()) != '"');
			tok.push_back('"');
			APPEND_TOK();
			tok = "";
			break;
		case '(':
			stk.push(SExp());
			break;
		case ')':
			switch (stk.size()) {
			case 0:
				throw std::exception("Missing '('");
				break;
			case 1:
				if (tok != "") stk.top().list.push_back(SExp(tok));
				return stk.top();
			default:
				if (tok != "") stk.top().list.push_back(SExp(tok));
				SExp l = stk.top();
				stk.pop();
				stk.top().list.push_back(l);
			}
			tok = "";
			break;
		default:
			tok.push_back(ch);
		}
	}

	switch (stk.size()) {
	case 0:  
		return tok;       break;
	case 1:  
		return stk.top(); 
		break;
	default: throw std::exception("Missing ')'");
	}
}

void print(std::ostream &out, SExp &_sexp)
{
	if (_sexp.type == SExp::LIST)
	{
		std::list<SExp>::iterator it;
		out << "(";
		for (it = _sexp.list.begin(); it != _sexp.list.end(); it++)
		{
			auto foo = *it;
			if (foo.type == SExp::ATOM)
			{
				out << foo.atom << " ";
			}
			else
			{ 
				print(out, foo);
			}
			
		}
		out << ") ";
	}
	else if (_sexp.type == SExp::ATOM)
	{
		out << _sexp.atom << " ";
	}
}

std::ostream &operator<<(std::ostream &os, OBJECT *obj)
{
    if (obj == NIL) {
		//snprintf(str, 255, "[%p NIL]", (void *)obj);
		os  << "[" << (void*) obj << "]";
		return os;
	}

	switch (object_type(obj)) {
	case PAIR:   
		{
			os << "[" << (void *)obj 
			<< (void *)_car(obj)
			<< (void *)_cdr(obj)
			<< _cdr(obj) == NIL ? "(NIL)" : "",(void *)NIL); 
		break;
		}
	case SYMBOL: 
		{
			os << "[" << (void *)obj
			<< (void *)obj << obj->value.symbol << "]"; 
		}
		break;
	case STRING:
		{ 
			os << "[" << (void *)obj << "STRING" 
			<<  obj->value.string
			<< "]";
		//snprintf(str, 255, "[%p, STRING %s]",
		//(void *)obj, obj->value.string); 
		break;
		}
	case NUMBER:
		{
			os << "["
			<< obj->value.number.integer
			<< obj->value.number.fraction
			<< "]";
			//snprintf(str, 255, "[%p, NUMBER %d.%d]",
			//(void *)obj, obj->value.number.integer, obj->value.number.fraction); 
			break;
		}
	case OPERATOR: 
	{
		os << "["
		<< ((void*)obj)
		<< "OPERATOR ]";

		//snprintf(str, 255, "[%p, OPERATOR ]",
		//(void*)obj); 
		break;
	}
		
	default: 
		abort();
	}

    return os;
}


void indent_print_obj(OBJECT *obj, int indent) {
	int i = 0;
	for (i = 0; i < indent; i++) 
		std::cout << " " << std::endl;
	std::cout << obj << std::endl;
	//printf("%s\n", obj_inspector(obj));
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
	std::string filename = "small.lsp";
	std::fstream s(filename);

	OBJECT *sexp = _read(s, 0);
	debug(sexp);
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