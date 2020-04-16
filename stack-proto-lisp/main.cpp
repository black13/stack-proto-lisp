#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

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
	case 0:  return tok;       break;
	case 1:  return stk.top(); break;
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
int main()
{
	std::string filename = "small.lsp";
	std::fstream s(filename);
	if (s.is_open()) {
		std::stringstream ss;
		SExp exp = readExpression(s);
		print(std::cout, exp);
		
		std::cout.flush();
	}
	
}