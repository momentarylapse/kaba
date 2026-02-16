#include "../kaba.h"
#include "AbstractParser.h"

namespace kaba {

AbstractParser::AbstractParser(SyntaxTree* t):
		Exp(t->expressions) {

	context = t->module->context;
	tree = t;
	Exp.cur_line = nullptr;
}

void AbstractParser::do_error(const string &str, shared<Node> node) {
	do_error_exp(str, node->token_id);
}

void AbstractParser::do_error(const string &str, int token_id) {
	do_error_exp(str, token_id);
}

void AbstractParser::do_error_exp(const string &str, int override_token_id) {
	if (Exp.lines.num == 0)
		throw Exception(str, "", 0, 0, tree->module);

	// what data do we have?
	int token_id = Exp.cur_token();

	// override?
	if (override_token_id >= 0)
		token_id = override_token_id;

	int physical_line = Exp.token_physical_line_no(token_id);
	int pos = Exp.token_line_offset(token_id);
	string expr = Exp.get_token(token_id);

#ifdef CPU_ARM
	msg_error(str);
#endif
	throw Exception(str, expr, physical_line, pos, tree->module);
}

void AbstractParser::expect_no_new_line(const string &error_msg) {
	if (Exp.end_of_line())
		do_error_exp(error_msg.num > 0 ? error_msg : "unexpected newline");
}

void AbstractParser::expect_new_line(const string &error_msg) {
	if (!Exp.end_of_line())
		do_error_exp(error_msg.num > 0 ? error_msg : "newline expected");
}

void AbstractParser::expect_new_line_with_indent() {
	if (!Exp.end_of_line())
		do_error_exp("newline expected");
	if (!Exp.next_line_is_indented())
		do_error_exp("additional indent expected");
}

void AbstractParser::expect_identifier(const string &identifier, const string &error_msg, bool consume) {
	if (Exp.cur != identifier)
		do_error_exp(error_msg);
	if (consume)
		Exp.next();
}

bool AbstractParser::try_consume(const string &identifier) {
	if (Exp.cur == identifier) {
		Exp.next();
		return true;
	}
	return false;
}

} // kaba