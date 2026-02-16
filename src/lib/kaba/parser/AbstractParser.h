//
// Created by michi on 2/16/26.
//

#ifndef KABA_ABSTRACTPARSER_H
#define KABA_ABSTRACTPARSER_H

namespace kaba {

class ExpressionBuffer;
class Context;
class SyntaxTree;

class AbstractParser {
public:
	explicit AbstractParser(SyntaxTree* tree);

	void do_error(const string &msg, shared<Node> node);
	void do_error(const string &msg, int token_id);
	void do_error_exp(const string &msg, int override_token_id = -1);
	void expect_no_new_line(const string &error_msg = "");
	void expect_new_line(const string &error_msg = "");
	void expect_new_line_with_indent();
	void expect_identifier(const string &identifier, const string &error_msg, bool consume = true);
	bool try_consume(const string &identifier);

	Context* context;
	SyntaxTree* tree;
	ExpressionBuffer& Exp;
};

}

#endif //KABA_ABSTRACTPARSER_H