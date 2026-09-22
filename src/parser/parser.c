#include "parser.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARSER_MAX_EXPRESSION_DEPTH 512U
#define PARSER_MAX_BLOCK_DEPTH 512U

typedef struct {
    const SourceFile *source;
    const TokenBuffer *tokens;
    DiagnosticBag *diagnostics;
    size_t current;
    size_t next_node_id;
    size_t expression_depth;
    size_t block_depth;
    int allocation_failed;
    Token synthetic_eof;
} Parser;

static AstNode *parse_expression(Parser *parser);
static AstNode *parse_block(Parser *parser);
static AstNode *parse_statement(Parser *parser);

static const Token *peek_at(const Parser *parser, size_t distance)
{
    size_t index = parser->current + distance;
    if (parser->tokens != NULL && index < parser->tokens->count) {
        return &parser->tokens->items[index];
    }
    return &parser->synthetic_eof;
}

static const Token *current_token(const Parser *parser)
{
    return peek_at(parser, 0U);
}

static int check(const Parser *parser, TokenKind kind)
{
    return current_token(parser)->kind == kind;
}

static const Token *advance(Parser *parser)
{
    const Token *token = current_token(parser);
    if (token->kind != TOKEN_EOF && parser->current < parser->tokens->count) {
        ++parser->current;
    }
    return token;
}

static int match(Parser *parser, TokenKind kind)
{
    if (!check(parser, kind)) {
        return 0;
    }
    (void)advance(parser);
    return 1;
}

static SourceSpan span_from(SourcePosition start, SourcePosition end)
{
    return source_span(start, end);
}

static SourceSpan span_nodes(const AstNode *left, const AstNode *right)
{
    return span_from(left->span.start, right->span.end);
}

static SourcePosition last_end(const Parser *parser, SourcePosition fallback)
{
    if (parser->current > 0U && parser->current - 1U < parser->tokens->count) {
        return parser->tokens->items[parser->current - 1U].span.end;
    }
    return fallback;
}

static char *copy_token_text(Parser *parser, const Token *token)
{
    char *copy;
    if (token == NULL || token->start == NULL || token->length > ((size_t)-1) - 1U) {
        return NULL;
    }
    copy = (char *)malloc(token->length + 1U);
    if (copy == NULL) {
        parser->allocation_failed = 1;
        return NULL;
    }
    memcpy(copy, token->start, token->length);
    copy[token->length] = '\0';
    return copy;
}

static char *decode_string_token(Parser *parser, const Token *token)
{
    const char *input;
    size_t input_length;
    size_t read_index;
    size_t write_index = 0U;
    char *decoded;

    if (token == NULL || token->start == NULL || token->length < 2U) {
        return NULL;
    }
    input = token->start + 1;
    input_length = token->length - 2U;
    decoded = (char *)malloc(input_length + 1U);
    if (decoded == NULL) {
        parser->allocation_failed = 1;
        return NULL;
    }
    for (read_index = 0U; read_index < input_length; ++read_index) {
        unsigned char byte = (unsigned char)input[read_index];
        if (byte == (unsigned char)'\\' && read_index + 1U < input_length) {
            unsigned char escaped = (unsigned char)input[++read_index];
            switch (escaped) {
            case '\\': decoded[write_index++] = '\\'; break;
            case '"': decoded[write_index++] = '"'; break;
            case 'n': decoded[write_index++] = '\n'; break;
            case 'r': decoded[write_index++] = '\r'; break;
            case 't': decoded[write_index++] = '\t'; break;
            default:
                /* O lexer não produz TOKEN_LITERAL_STRING para escape inválido. */
                decoded[write_index++] = '\\';
                decoded[write_index++] = (char)escaped;
                break;
            }
        } else {
            decoded[write_index++] = (char)byte;
        }
    }
    decoded[write_index] = '\0';
    return decoded;
}

static void syntax_error(Parser *parser, const Token *token,
                         const char *code, const char *message)
{
    int added;
    if (token->start != NULL && token->length != 0U) {
        size_t shown = token->length > (size_t)INT_MAX ? (size_t)INT_MAX : token->length;
        added = diagnostic_bag_add(parser->diagnostics, parser->source, token->span,
                                   DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SYNTACTIC,
                                   code, "%s; encontrado %s '%.*s'", message,
                                   token_kind_name(token->kind), (int)shown, token->start);
    } else {
        added = diagnostic_bag_add(parser->diagnostics, parser->source, token->span,
                                   DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SYNTACTIC,
                                   code, "%s; encontrado %s", message,
                                   token_kind_name(token->kind));
    }
    if (!added) {
        parser->allocation_failed = 1;
    }
}

static void out_of_memory(Parser *parser)
{
    if (!parser->allocation_failed) {
        parser->allocation_failed = 1;
        (void)diagnostic_bag_add(parser->diagnostics, parser->source,
                                 current_token(parser)->span, DIAGNOSTIC_ERROR,
                                 DIAGNOSTIC_PHASE_SYNTACTIC, "E-SIN-003",
                                 "memoria insuficiente durante o parsing");
    }
}

static int enter_expression_depth(Parser *parser, const Token *token)
{
    if (parser->expression_depth >= PARSER_MAX_EXPRESSION_DEPTH) {
        syntax_error(parser, token, "E-SIN-009",
                     "profundidade maxima de 512 niveis de expressao excedida");
        return 0;
    }
    ++parser->expression_depth;
    return 1;
}

static void leave_expression_depth(Parser *parser)
{
    if (parser->expression_depth > 0U) {
        --parser->expression_depth;
    }
}

static const Token *expect(Parser *parser, TokenKind kind, const char *description)
{
    const Token *token = current_token(parser);
    if (token->kind == kind) {
        return advance(parser);
    }
    syntax_error(parser, token, "E-SIN-002", description);
    return NULL;
}

static AstNode *new_node(Parser *parser, AstKind kind, SourceSpan span)
{
    AstNode *node = ast_node_create(kind, span, parser->next_node_id++);
    if (node == NULL) {
        out_of_memory(parser);
        return NULL;
    }
#if MACARONICA_PARSER_DEBUG
    (void)fprintf(stderr, "[debug][parser] no %zu: %s em %zu:%zu\n", // debug - remover
                  node->node_id, ast_kind_name(kind),
                  span.start.line, span.start.column);
#endif
    return node;
}

static int append_node(Parser *parser, AstNodeList *list, AstNode *node)
{
    if (!ast_node_list_append(list, node)) {
        ast_node_destroy(node);
        out_of_memory(parser);
        return 0;
    }
    return 1;
}

static AstOperator token_operator(TokenKind kind)
{
    switch (kind) {
    case TOKEN_PLUS: return AST_OP_ADD;
    case TOKEN_MINUS: return AST_OP_SUBTRACT;
    case TOKEN_STAR: return AST_OP_MULTIPLY;
    case TOKEN_SLASH: return AST_OP_DIVIDE;
    case TOKEN_INTEGER_DIVIDE: return AST_OP_INTEGER_DIVIDE;
    case TOKEN_CARET: return AST_OP_POWER;
    case TOKEN_LESS: return AST_OP_LESS;
    case TOKEN_GREATER: return AST_OP_GREATER;
    case TOKEN_EQUAL_EQUAL: return AST_OP_EQUAL;
    case TOKEN_DIFFERENT: return AST_OP_DIFFERENT;
    case TOKEN_LOGICAL_NOT: return AST_OP_LOGICAL_NOT;
    case TOKEN_LOGICAL_AND: return AST_OP_LOGICAL_AND;
    case TOKEN_LOGICAL_XOR: return AST_OP_LOGICAL_XOR;
    case TOKEN_LOGICAL_OR: return AST_OP_LOGICAL_OR;
    case TOKEN_KW_RAIZ: return AST_OP_ROOT;
    default: return AST_OP_NONE;
    }
}

static AstNode *make_identifier(Parser *parser, const Token *token)
{
    AstNode *node = new_node(parser, AST_IDENTIFIER_EXPR, token->span);
    if (node == NULL) return NULL;
    node->as.identifier_expr.name = copy_token_text(parser, token);
    if (node->as.identifier_expr.name == NULL) {
        ast_node_destroy(node); out_of_memory(parser); return NULL;
    }
    return node;
}

static AstNode *parse_primary(Parser *parser)
{
    const Token *token = current_token(parser);
    AstNode *node;
    if (token->kind == TOKEN_IDENTIFIER) {
        const Token *name = advance(parser);
        if (check(parser, TOKEN_LEFT_PAREN)) {
            const Token *open = advance(parser);
            if (!enter_expression_depth(parser, open)) return NULL;
            node = new_node(parser, AST_CALL_EXPR, name->span);
            if (node == NULL) { leave_expression_depth(parser); return NULL; }
            node->as.call_expr.callee = copy_token_text(parser, name);
            node->as.call_expr.callee_span = name->span;
            if (node->as.call_expr.callee == NULL) {
                ast_node_destroy(node); leave_expression_depth(parser); return NULL;
            }
            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    AstNode *argument = parse_expression(parser);
                    if (argument == NULL || !append_node(parser, &node->as.call_expr.arguments, argument)) {
                        ast_node_destroy(node); leave_expression_depth(parser); return NULL;
                    }
                } while (match(parser, TOKEN_COMMA));
            }
            (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos argumentos");
            node->span.end = last_end(parser, name->span.end);
            leave_expression_depth(parser);
            return node;
        }
        if (check(parser, TOKEN_LEFT_BRACE)) {
            const Token *open = advance(parser);
            AstNode *base = make_identifier(parser, name);
            AstNode *index;
            if (base == NULL) return NULL;
            if (!enter_expression_depth(parser, open)) {
                ast_node_destroy(base); return NULL;
            }
            index = parse_expression(parser);
            leave_expression_depth(parser);
            if (index == NULL) { ast_node_destroy(base); return NULL; }
            (void)expect(parser, TOKEN_RIGHT_BRACE, "esperado '}' apos indice");
            node = new_node(parser, AST_INDEX_EXPR,
                            span_from(name->span.start, last_end(parser, index->span.end)));
            if (node == NULL) { ast_node_destroy(base); ast_node_destroy(index); return NULL; }
            node->as.index_expr.base = base;
            node->as.index_expr.index = index;
            return node;
        }
        return make_identifier(parser, name);
    }
    if (token->kind == TOKEN_LITERAL_INTEGER || token->kind == TOKEN_LITERAL_REAL ||
        token->kind == TOKEN_LITERAL_STRING || token->kind == TOKEN_TRUE || token->kind == TOKEN_FALSE) {
        (void)advance(parser);
        node = new_node(parser, AST_LITERAL_EXPR, token->span);
        if (node == NULL) return NULL;
        if (token->kind == TOKEN_LITERAL_INTEGER) {
            node->as.literal_expr.literal_kind = AST_LITERAL_INTEGER;
            node->as.literal_expr.value.integer = token->literal.inteira;
        } else if (token->kind == TOKEN_LITERAL_REAL) {
            node->as.literal_expr.literal_kind = AST_LITERAL_REAL;
            node->as.literal_expr.value.real = token->literal.real;
        } else if (token->kind == TOKEN_LITERAL_STRING) {
            node->as.literal_expr.literal_kind = AST_LITERAL_STRING;
            node->as.literal_expr.value.string = decode_string_token(parser, token);
            if (node->as.literal_expr.value.string == NULL) { ast_node_destroy(node); return NULL; }
        } else {
            node->as.literal_expr.literal_kind = AST_LITERAL_BOOLEAN;
            node->as.literal_expr.value.boolean = token->kind == TOKEN_TRUE;
        }
        return node;
    }
    if (check(parser, TOKEN_LEFT_PAREN)) {
        const Token *open = advance(parser);
        if (!enter_expression_depth(parser, open)) return NULL;
        node = parse_expression(parser);
        leave_expression_depth(parser);
        if (node == NULL) return NULL;
        (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos expressao");
        return node;
    }
    syntax_error(parser, token, "E-SIN-001", "esperada expressao");
    if (!check(parser, TOKEN_EOF)) (void)advance(parser);
    return NULL;
}

static AstNode *parse_high(Parser *parser)
{
    const Token *operator_token;
    AstNode *operand;
    AstNode *node;
    if (!check(parser, TOKEN_KW_RAIZ)) return parse_primary(parser);
    operator_token = advance(parser);
    if (expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos raiz") == NULL) return NULL;
    if (!enter_expression_depth(parser, operator_token)) return NULL;
    operand = parse_expression(parser);
    leave_expression_depth(parser);
    if (operand == NULL) return NULL;
    (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos argumento de raiz");
    node = new_node(parser, AST_UNARY_EXPR,
                    span_from(operator_token->span.start, last_end(parser, operand->span.end)));
    if (node == NULL) { ast_node_destroy(operand); return NULL; }
    node->as.unary_expr.operator_kind = AST_OP_ROOT;
    node->as.unary_expr.operator_span = operator_token->span;
    node->as.unary_expr.operand = operand;
    return node;
}

static AstNode *parse_unary(Parser *parser)
{
    const Token *operator_token;
    AstNode *operand;
    AstNode *node;
    if (!check(parser, TOKEN_PLUS) && !check(parser, TOKEN_MINUS)) return parse_high(parser);
    if (!enter_expression_depth(parser, current_token(parser))) return NULL;
    operator_token = advance(parser);
    operand = parse_unary(parser);
    leave_expression_depth(parser);
    if (operand == NULL) return NULL;
    node = new_node(parser, AST_UNARY_EXPR, span_from(operator_token->span.start, operand->span.end));
    if (node == NULL) { ast_node_destroy(operand); return NULL; }
    node->as.unary_expr.operator_kind = operator_token->kind == TOKEN_PLUS
                                           ? AST_OP_UNARY_PLUS : AST_OP_UNARY_MINUS;
    node->as.unary_expr.operator_span = operator_token->span;
    node->as.unary_expr.operand = operand;
    return node;
}

static AstNode *make_binary(Parser *parser, AstNode *left,
                            const Token *operator_token, AstNode *right)
{
    AstNode *node;
    if (right == NULL) { ast_node_destroy(left); return NULL; }
    node = new_node(parser, AST_BINARY_EXPR, span_nodes(left, right));
    if (node == NULL) { ast_node_destroy(left); ast_node_destroy(right); return NULL; }
    node->as.binary_expr.operator_kind = token_operator(operator_token->kind);
    node->as.binary_expr.operator_span = operator_token->span;
    node->as.binary_expr.left = left;
    node->as.binary_expr.right = right;
    return node;
}

static AstNode *parse_power(Parser *parser)
{
    AstNode *left = parse_unary(parser);
    if (left != NULL && check(parser, TOKEN_CARET)) {
        const Token *op;
        AstNode *right;
        if (!enter_expression_depth(parser, current_token(parser))) {
            ast_node_destroy(left);
            return NULL;
        }
        op = advance(parser);
        right = parse_power(parser);
        leave_expression_depth(parser);
        left = make_binary(parser, left, op, right);
    }
    return left;
}

static AstNode *parse_multiplicative(Parser *parser)
{
    AstNode *left = parse_power(parser);
    while (left != NULL && (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) ||
                            check(parser, TOKEN_INTEGER_DIVIDE))) {
        const Token *op = advance(parser);
        left = make_binary(parser, left, op, parse_power(parser));
    }
    return left;
}

static AstNode *parse_additive(Parser *parser)
{
    AstNode *left = parse_multiplicative(parser);
    while (left != NULL && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        const Token *op = advance(parser);
        left = make_binary(parser, left, op, parse_multiplicative(parser));
    }
    return left;
}

static int is_comparator(TokenKind kind)
{
    return kind == TOKEN_LESS || kind == TOKEN_GREATER ||
           kind == TOKEN_EQUAL_EQUAL || kind == TOKEN_DIFFERENT;
}

static AstNode *parse_comparison(Parser *parser)
{
    AstNode *left = parse_additive(parser);
    if (left != NULL && is_comparator(current_token(parser)->kind)) {
        const Token *op = advance(parser);
        left = make_binary(parser, left, op, parse_additive(parser));
        if (left != NULL && is_comparator(current_token(parser)->kind)) {
            syntax_error(parser, current_token(parser), "E-SIN-005",
                         "comparadores nao podem ser encadeados");
            while (is_comparator(current_token(parser)->kind)) {
                (void)advance(parser);
                { AstNode *discarded = parse_additive(parser); ast_node_destroy(discarded); }
            }
        }
    }
    return left;
}

static AstNode *parse_not(Parser *parser)
{
    if (check(parser, TOKEN_LOGICAL_NOT)) {
        const Token *op;
        AstNode *operand;
        AstNode *node;
        if (!enter_expression_depth(parser, current_token(parser))) return NULL;
        op = advance(parser);
        operand = parse_not(parser);
        leave_expression_depth(parser);
        if (operand == NULL) return NULL;
        node = new_node(parser, AST_UNARY_EXPR, span_from(op->span.start, operand->span.end));
        if (node == NULL) { ast_node_destroy(operand); return NULL; }
        node->as.unary_expr.operator_kind = AST_OP_LOGICAL_NOT;
        node->as.unary_expr.operator_span = op->span;
        node->as.unary_expr.operand = operand;
        return node;
    }
    return parse_comparison(parser);
}

static AstNode *parse_left_logical(Parser *parser, AstNode *(*lower)(Parser *), TokenKind kind)
{
    AstNode *left = lower(parser);
    while (left != NULL && check(parser, kind)) {
        const Token *op = advance(parser);
        left = make_binary(parser, left, op, lower(parser));
    }
    return left;
}

static AstNode *parse_and(Parser *parser) { return parse_left_logical(parser, parse_not, TOKEN_LOGICAL_AND); }
static AstNode *parse_xor(Parser *parser) { return parse_left_logical(parser, parse_and, TOKEN_LOGICAL_XOR); }
static AstNode *parse_expression(Parser *parser) { return parse_left_logical(parser, parse_xor, TOKEN_LOGICAL_OR); }

static int is_native_value_type(TokenKind kind)
{
    return kind == TOKEN_KW_BOOL || kind == TOKEN_KW_INTEIRA ||
           kind == TOKEN_KW_FLUT || kind == TOKEN_KW_PALAVRA ||
           kind == TOKEN_KW_DUPLOCARPADO;
}

static int is_type_start(TokenKind kind, int allow_void)
{
    return is_native_value_type(kind) || kind == TOKEN_IDENTIFIER ||
           (allow_void && kind == TOKEN_KW_VAZIO);
}

static int parse_type(Parser *parser, int allow_void, AstTypeRef *out)
{
    const Token *token = current_token(parser);
    memset(out, 0, sizeof(*out));
    out->resolved_type_id = AST_ID_NONE;
    if (!is_type_start(token->kind, allow_void)) {
        syntax_error(parser, token, "E-SIN-001",
                     allow_void ? "esperado tipo de retorno" : "esperado tipo de valor");
        return 0;
    }
    (void)advance(parser);
    out->name = copy_token_text(parser, token);
    out->span = token->span;
    if (out->name == NULL) { out_of_memory(parser); return 0; }
    return 1;
}

static int append_declared_token(Parser *parser, AstDeclaredNameList *list, const Token *token)
{
    char *name = copy_token_text(parser, token);
    if (name == NULL) return 0;
    if (!ast_declared_name_list_append(list, name, token->span)) {
        free(name); out_of_memory(parser); return 0;
    }
    return 1;
}

static AstNode *parse_declaration_after_type(Parser *parser, AstTypeRef *type,
                                             SourcePosition start, int field_or_parameter,
                                             int consume_semicolon)
{
    int is_vector = 0;
    long long vector_size = 0;
    SourceSpan size_span = type->span;
    const Token *name;
    AstNode *node;

    if (!field_or_parameter && match(parser, TOKEN_KW_VET)) {
        const Token *size_token;
        is_vector = 1;
        (void)expect(parser, TOKEN_LEFT_BRACE, "esperado '{' antes do tamanho do vetor");
        size_token = expect(parser, TOKEN_LITERAL_INTEGER, "esperado literal inteiro como tamanho do vetor");
        if (size_token != NULL) { vector_size = size_token->literal.inteira; size_span = size_token->span; }
        (void)expect(parser, TOKEN_RIGHT_BRACE, "esperado '}' apos tamanho do vetor");
    }
    name = expect(parser, TOKEN_IDENTIFIER, "esperado nome na declaracao");
    if (name == NULL) { ast_type_ref_destroy(type); return NULL; }
    node = new_node(parser, is_vector ? AST_VECTOR_DECL : AST_VAR_DECL,
                    span_from(start, name->span.end));
    if (node == NULL) { ast_type_ref_destroy(type); return NULL; }
    if (is_vector) {
        node->as.vector_decl.type = *type;
        node->as.vector_decl.size = vector_size;
        node->as.vector_decl.size_span = size_span;
        memset(type, 0, sizeof(*type));
        if (!append_declared_token(parser, &node->as.vector_decl.names, name)) {
            ast_node_destroy(node); return NULL;
        }
    } else {
        node->as.var_decl.type = *type;
        memset(type, 0, sizeof(*type));
        if (!append_declared_token(parser, &node->as.var_decl.names, name)) {
            ast_node_destroy(node); return NULL;
        }
    }
    if (!field_or_parameter) {
        while (match(parser, TOKEN_COMMA)) {
            name = expect(parser, TOKEN_IDENTIFIER, "esperado nome apos ','");
            if (name == NULL) break;
            if (is_vector) {
                if (!append_declared_token(parser, &node->as.vector_decl.names, name)) { ast_node_destroy(node); return NULL; }
            } else if (!append_declared_token(parser, &node->as.var_decl.names, name)) {
                ast_node_destroy(node); return NULL;
            }
        }
    }
    if (consume_semicolon) {
        (void)expect(parser, TOKEN_SEMICOLON, "esperado ';' apos declaracao");
    }
    node->span.end = last_end(parser, name->span.end);
    return node;
}

static AstNode *parse_value_declaration(Parser *parser, int field)
{
    SourcePosition start = current_token(parser)->span.start;
    AstTypeRef type;
    if (!parse_type(parser, 0, &type)) return NULL;
    return parse_declaration_after_type(parser, &type, start, field, 1);
}

static AstNode *parse_destination(Parser *parser)
{
    const Token *name = expect(parser, TOKEN_IDENTIFIER, "esperado destino da atribuicao");
    AstNode *base;
    AstNode *node;
    AstNode *index;
    if (name == NULL) return NULL;
    base = make_identifier(parser, name);
    if (base == NULL || !match(parser, TOKEN_LEFT_BRACE)) return base;
    index = parse_expression(parser);
    if (index == NULL) { ast_node_destroy(base); return NULL; }
    (void)expect(parser, TOKEN_RIGHT_BRACE, "esperado '}' apos indice do destino");
    node = new_node(parser, AST_INDEX_EXPR,
                    span_from(name->span.start, last_end(parser, index->span.end)));
    if (node == NULL) { ast_node_destroy(base); ast_node_destroy(index); return NULL; }
    node->as.index_expr.base = base;
    node->as.index_expr.index = index;
    return node;
}

static AstNode *parse_assignment_no_semicolon(Parser *parser)
{
    AstNode *target = parse_destination(parser);
    const Token *receive;
    AstNode *value;
    AstNode *node;
    if (target == NULL) return NULL;
    receive = expect(parser, TOKEN_KW_RECEBA, "esperado 'receba' na atribuicao");
    if (receive == NULL) { ast_node_destroy(target); return NULL; }
    value = parse_expression(parser);
    if (value == NULL) { ast_node_destroy(target); return NULL; }
    node = new_node(parser, AST_ASSIGNMENT, span_nodes(target, value));
    if (node == NULL) { ast_node_destroy(target); ast_node_destroy(value); return NULL; }
    node->as.assignment.target = target;
    node->as.assignment.value = value;
    (void)receive;
    return node;
}

static AstNode *parse_return(Parser *parser)
{
    const Token *start = advance(parser);
    AstNode *value = NULL;
    AstNode *node;
    if (!check(parser, TOKEN_SEMICOLON)) value = parse_expression(parser);
    (void)expect(parser, TOKEN_SEMICOLON, "esperado ';' apos respost");
    if (value == NULL && !check(parser, TOKEN_SEMICOLON) &&
        parser->allocation_failed) return NULL;
    node = new_node(parser, AST_RETURN_STMT,
                    span_from(start->span.start, last_end(parser, start->span.end)));
    if (node == NULL) { ast_node_destroy(value); return NULL; }
    node->as.return_stmt.value = value;
    return node;
}

static AstNode *parse_while(Parser *parser)
{
    const Token *start = advance(parser);
    AstNode *condition;
    AstNode *body;
    AstNode *node;
    (void)expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos durante");
    condition = parse_expression(parser);
    (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos condicao de durante");
    (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos durante");
    body = parse_block(parser);
    if (condition == NULL || body == NULL) { ast_node_destroy(condition); ast_node_destroy(body); return NULL; }
    node = new_node(parser, AST_WHILE_STMT, span_from(start->span.start, body->span.end));
    if (node == NULL) { ast_node_destroy(condition); ast_node_destroy(body); return NULL; }
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

static AstNode *parse_for(Parser *parser)
{
    const Token *start = advance(parser);
    AstNode *initializer;
    AstNode *condition;
    AstNode *iteration;
    AstNode *body;
    AstNode *node;
    (void)expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos repete");
    initializer = parse_assignment_no_semicolon(parser);
    (void)expect(parser, TOKEN_COMMA, "esperado ',' apos inicializacao de repete");
    condition = parse_expression(parser);
    (void)expect(parser, TOKEN_COMMA, "esperado ',' apos condicao de repete");
    iteration = parse_assignment_no_semicolon(parser);
    (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos cabecalho de repete");
    (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos repete");
    body = parse_block(parser);
    if (initializer == NULL || condition == NULL || iteration == NULL || body == NULL) {
        ast_node_destroy(initializer); ast_node_destroy(condition);
        ast_node_destroy(iteration); ast_node_destroy(body); return NULL;
    }
    node = new_node(parser, AST_FOR_STMT, span_from(start->span.start, body->span.end));
    if (node == NULL) {
        ast_node_destroy(initializer); ast_node_destroy(condition);
        ast_node_destroy(iteration); ast_node_destroy(body); return NULL;
    }
    node->as.for_stmt.initializer = initializer;
    node->as.for_stmt.condition = condition;
    node->as.for_stmt.iteration = iteration;
    node->as.for_stmt.body = body;
    return node;
}

static int append_if_branch(Parser *parser, AstNode *node,
                            AstNode *condition, AstNode *body,
                            SourcePosition start)
{
    if (!ast_if_branch_list_append(&node->as.if_stmt.branches, condition, body,
                                   span_from(start, body->span.end))) {
        ast_node_destroy(condition); ast_node_destroy(body); out_of_memory(parser); return 0;
    }
    return 1;
}

static AstNode *parse_if(Parser *parser)
{
    const Token *start_token = advance(parser);
    AstNode *node = new_node(parser, AST_IF_STMT, start_token->span);
    AstNode *condition;
    AstNode *body;
    if (node == NULL) return NULL;
    (void)expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos cond");
    condition = parse_expression(parser);
    (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos condicao");
    (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos condicao");
    body = parse_block(parser);
    if (condition == NULL || body == NULL ||
        !append_if_branch(parser, node, condition, body, start_token->span.start)) {
        if (condition == NULL || body == NULL) { ast_node_destroy(condition); ast_node_destroy(body); }
        ast_node_destroy(node); return NULL;
    }
    while (match(parser, TOKEN_KW_CASOCONTRARIO)) {
        SourcePosition branch_start = parser->tokens->items[parser->current - 1U].span.start;
        if (!match(parser, TOKEN_KW_COND)) {
            (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos casocontrario");
            node->as.if_stmt.else_body = parse_block(parser);
            if (node->as.if_stmt.else_body == NULL) { ast_node_destroy(node); return NULL; }
            if (check(parser, TOKEN_KW_CASOCONTRARIO)) {
                syntax_error(parser, current_token(parser), "E-SIN-007",
                             "casocontrario final deve encerrar a cadeia");
            }
            break;
        }
        (void)expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos casocontrario cond");
        condition = parse_expression(parser);
        (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos condicao");
        (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos condicao");
        body = parse_block(parser);
        if (condition == NULL || body == NULL ||
            !append_if_branch(parser, node, condition, body, branch_start)) {
            if (condition == NULL || body == NULL) { ast_node_destroy(condition); ast_node_destroy(body); }
            ast_node_destroy(node); return NULL;
        }
    }
    node->span.end = node->as.if_stmt.else_body != NULL
                         ? node->as.if_stmt.else_body->span.end
                         : node->as.if_stmt.branches.items[node->as.if_stmt.branches.count - 1U].body->span.end;
    return node;
}

static AstNode *parse_identifier_statement(Parser *parser)
{
    if (peek_at(parser, 1U)->kind == TOKEN_LEFT_PAREN) {
        AstNode *call = parse_primary(parser);
        AstNode *statement;
        if (call == NULL) return NULL;
        (void)expect(parser, TOKEN_SEMICOLON, "esperado ';' apos chamada");
        statement = new_node(parser, AST_EXPR_STMT,
                             span_from(call->span.start, last_end(parser, call->span.end)));
        if (statement == NULL) { ast_node_destroy(call); return NULL; }
        statement->as.expr_stmt.expression = call;
        return statement;
    }
    {
        AstNode *assignment = parse_assignment_no_semicolon(parser);
        if (assignment == NULL) return NULL;
        (void)expect(parser, TOKEN_SEMICOLON, "esperado ';' apos atribuicao");
        assignment->span.end = last_end(parser, assignment->span.end);
        return assignment;
    }
}

static int looks_like_local_declaration(const Parser *parser)
{
    TokenKind kind = current_token(parser)->kind;
    if (is_native_value_type(kind) || kind == TOKEN_KW_VAZIO) return 1;
    return kind == TOKEN_IDENTIFIER &&
           (peek_at(parser, 1U)->kind == TOKEN_IDENTIFIER ||
            peek_at(parser, 1U)->kind == TOKEN_KW_VET);
}

static AstNode *parse_statement(Parser *parser)
{
#if MACARONICA_PARSER_DEBUG
    (void)fprintf(stderr, "[debug][parser] comando inicia com %s\n", // debug - remover
                  token_kind_name(current_token(parser)->kind));
#endif
    if (looks_like_local_declaration(parser)) return parse_value_declaration(parser, 0);
    if (check(parser, TOKEN_IDENTIFIER)) return parse_identifier_statement(parser);
    if (check(parser, TOKEN_KW_REPETE)) return parse_for(parser);
    if (check(parser, TOKEN_KW_DURANTE)) return parse_while(parser);
    if (check(parser, TOKEN_KW_COND)) return parse_if(parser);
    if (check(parser, TOKEN_KW_RESPOST)) return parse_return(parser);
    if (check(parser, TOKEN_KW_CASOCONTRARIO)) {
        syntax_error(parser, current_token(parser), "E-SIN-007",
                     "casocontrario nao pode aparecer isolado");
    } else if (check(parser, TOKEN_KW_FUNC)) {
        syntax_error(parser, current_token(parser), "E-SIN-004",
                     "'func' nao possui producao; use 'outrafuncao'");
    } else {
        syntax_error(parser, current_token(parser), "E-SIN-001", "esperado comando");
    }
    if (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RIGHT_BRACKET)) (void)advance(parser);
    return NULL;
}

static int is_statement_start(const Parser *parser)
{
    TokenKind kind = current_token(parser)->kind;
    return looks_like_local_declaration(parser) || kind == TOKEN_IDENTIFIER ||
           kind == TOKEN_KW_REPETE || kind == TOKEN_KW_DURANTE ||
           kind == TOKEN_KW_COND || kind == TOKEN_KW_RESPOST;
}

static void synchronize_statement(Parser *parser, size_t failed_at)
{
    if (parser->current == failed_at && !check(parser, TOKEN_EOF) &&
        !check(parser, TOKEN_RIGHT_BRACKET)) {
        (void)advance(parser);
    }
    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RIGHT_BRACKET)) {
        if (match(parser, TOKEN_SEMICOLON)) return;
        if (is_statement_start(parser)) return;
        (void)advance(parser);
    }
}

static AstNode *parse_block(Parser *parser)
{
    const Token *open;
    AstNode *block = NULL;
    if (parser->block_depth >= PARSER_MAX_BLOCK_DEPTH) {
        syntax_error(parser, current_token(parser), "E-SIN-009",
                     "profundidade maxima de 512 blocos excedida");
        return NULL;
    }
    ++parser->block_depth;
    open = expect(parser, TOKEN_LEFT_BRACKET, "esperado '[' para iniciar bloco");
    if (open == NULL) goto finish;
    block = new_node(parser, AST_BLOCK, open->span);
    if (block == NULL) goto finish;
    while (!check(parser, TOKEN_RIGHT_BRACKET) && !check(parser, TOKEN_EOF) &&
           !parser->allocation_failed) {
        size_t before = parser->current;
        AstNode *statement;
        if (match(parser, TOKEN_INVALID)) continue;
        statement = parse_statement(parser);
        if (statement != NULL) {
            if (!append_node(parser, &block->as.block.statements, statement)) {
                ast_node_destroy(block);
                block = NULL;
                goto finish;
            }
        } else {
            synchronize_statement(parser, before);
        }
    }
    if (check(parser, TOKEN_RIGHT_BRACKET)) {
        block->span.end = advance(parser)->span.end;
    } else {
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado ']' para encerrar bloco");
        block->span.end = last_end(parser, open->span.end);
    }
finish:
    --parser->block_depth;
    return block;
}

static AstNode *parse_struct(Parser *parser)
{
    const Token *start = advance(parser);
    const Token *name = expect(parser, TOKEN_IDENTIFIER, "esperado nome do registro");
    AstNode *node;
    if (name == NULL) return NULL;
    node = new_node(parser, AST_STRUCT_DECL, span_from(start->span.start, name->span.end));
    if (node == NULL) return NULL;
    node->as.struct_decl.name = copy_token_text(parser, name);
    node->as.struct_decl.name_span = name->span;
    if (node->as.struct_decl.name == NULL) { ast_node_destroy(node); return NULL; }
    (void)expect(parser, TOKEN_AT, "esperado '@' apos nome do registro");
    if (expect(parser, TOKEN_LEFT_BRACKET, "esperado '[' para iniciar registro") == NULL) {
        ast_node_destroy(node); return NULL;
    }
    while (!check(parser, TOKEN_RIGHT_BRACKET) && !check(parser, TOKEN_EOF)) {
        size_t before = parser->current;
        AstNode *field;
        if (!is_type_start(current_token(parser)->kind, 0)) {
            syntax_error(parser, current_token(parser), "E-SIN-001", "esperada declaracao de campo");
            synchronize_statement(parser, before);
            continue;
        }
        field = parse_value_declaration(parser, 1);
        if (field != NULL) {
            if (!append_node(parser, &node->as.struct_decl.fields, field)) { ast_node_destroy(node); return NULL; }
        } else synchronize_statement(parser, before);
    }
    if (check(parser, TOKEN_RIGHT_BRACKET)) node->span.end = advance(parser)->span.end;
    else {
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado ']' para encerrar registro");
        node->span.end = last_end(parser, name->span.end);
    }
    return node;
}

static AstNode *parse_parameter(Parser *parser)
{
    SourcePosition start = current_token(parser)->span.start;
    AstTypeRef type;
    if (!parse_type(parser, 0, &type)) return NULL;
    return parse_declaration_after_type(parser, &type, start, 1, 0);
}

static int parse_parameters(Parser *parser, AstNodeList *parameters)
{
    if (check(parser, TOKEN_RIGHT_PAREN)) return 1;
    do {
        AstNode *parameter = parse_parameter(parser);
        if (parameter == NULL || !append_node(parser, parameters, parameter)) return 0;
    } while (match(parser, TOKEN_COMMA));
    return 1;
}

static AstNode *parse_function_after_type(Parser *parser, AstTypeRef *return_type,
                                          SourcePosition start, int is_principal)
{
    const Token *name;
    AstNode *node;
    if (is_principal) {
        name = expect(parser, TOKEN_KW_PRINCIPAL, "esperado 'principal'");
    } else {
        name = expect(parser, TOKEN_IDENTIFIER, "esperado nome da funcao");
    }
    if (name == NULL) { ast_type_ref_destroy(return_type); return NULL; }
    node = new_node(parser, AST_FUNCTION_DECL, span_from(start, name->span.end));
    if (node == NULL) { ast_type_ref_destroy(return_type); return NULL; }
    node->as.function_decl.return_type = *return_type;
    memset(return_type, 0, sizeof(*return_type));
    node->as.function_decl.name = is_principal
                                      ? copy_token_text(parser, name)
                                      : copy_token_text(parser, name);
    node->as.function_decl.name_span = name->span;
    node->as.function_decl.is_principal = is_principal;
    if (node->as.function_decl.name == NULL) { ast_node_destroy(node); return NULL; }
    (void)expect(parser, TOKEN_LEFT_PAREN, "esperado '(' apos nome da funcao");
    if (!parse_parameters(parser, &node->as.function_decl.parameters)) {
        ast_node_destroy(node); return NULL;
    }
    (void)expect(parser, TOKEN_RIGHT_PAREN, "esperado ')' apos parametros");
    (void)expect(parser, TOKEN_AT, "esperado '@' antes do corpo da funcao");
    node->as.function_decl.body = parse_block(parser);
    if (node->as.function_decl.body == NULL) { ast_node_destroy(node); return NULL; }
    node->span.end = node->as.function_decl.body->span.end;
    return node;
}

static AstNode *parse_global(Parser *parser)
{
    SourcePosition start = current_token(parser)->span.start;
    AstTypeRef type;
#if MACARONICA_PARSER_DEBUG
    (void)fprintf(stderr, "[debug][parser] declaracao global inicia com %s\n", // debug - remover
                  token_kind_name(current_token(parser)->kind));
#endif
    if (check(parser, TOKEN_KW_REGISTRO)) return parse_struct(parser);
    if (match(parser, TOKEN_KW_OUTRAFUNCAO)) {
        if (!parse_type(parser, 1, &type)) return NULL;
        return parse_function_after_type(parser, &type, start, 0);
    }
    if (check(parser, TOKEN_KW_FUNC)) {
        syntax_error(parser, current_token(parser), "E-SIN-004",
                     "'func' nao possui producao; use 'outrafuncao' ou 'principal'");
        (void)advance(parser); return NULL;
    }
    if (!is_type_start(current_token(parser)->kind, 1)) {
        syntax_error(parser, current_token(parser), "E-SIN-001", "esperada declaracao global");
        if (!check(parser, TOKEN_EOF)) (void)advance(parser);
        return NULL;
    }
    if (!parse_type(parser, 1, &type)) return NULL;
    if (check(parser, TOKEN_KW_PRINCIPAL)) {
        return parse_function_after_type(parser, &type, start, 1);
    }
    if (type.name != NULL && strcmp(type.name, "vazio") == 0) {
        syntax_error(parser, current_token(parser), "E-SIN-008",
                     "tipo vazio somente pode ser retorno de funcao");
        ast_type_ref_destroy(&type); return NULL;
    }
    return parse_declaration_after_type(parser, &type, start, 0, 1);
}

static int is_global_start(const Parser *parser)
{
    TokenKind kind = current_token(parser)->kind;
    return kind == TOKEN_KW_REGISTRO || kind == TOKEN_KW_OUTRAFUNCAO ||
           kind == TOKEN_KW_FUNC || is_type_start(kind, 1);
}

static void synchronize_global(Parser *parser, size_t failed_at)
{
    if (parser->current == failed_at && !check(parser, TOKEN_EOF)) (void)advance(parser);
    while (!check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_SEMICOLON)) return;
        if (is_global_start(parser)) return;
        (void)advance(parser);
    }
}

FrontendStatus parser_parse(const SourceFile *source,
                            const TokenBuffer *tokens,
                            DiagnosticBag *diagnostics,
                            AstNode **out_program)
{
    Parser parser;
    AstNode *program;
    size_t errors_before;
    if (source == NULL || tokens == NULL || diagnostics == NULL || out_program == NULL)
        return FRONTEND_STATUS_INTERNAL_ERROR;
    *out_program = NULL;
    memset(&parser, 0, sizeof(parser));
    parser.source = source;
    parser.tokens = tokens;
    parser.diagnostics = diagnostics;
    parser.synthetic_eof.kind = TOKEN_EOF;
    parser.synthetic_eof.span = source_span(source_position(source->length, 1U, 1U),
                                            source_position(source->length, 1U, 1U));
    errors_before = diagnostics->error_count;
#if MACARONICA_PARSER_DEBUG
    (void)fprintf(stderr, "[debug][parser] iniciando `%s` com %zu tokens\n", // debug - remover
                  source->path != NULL ? source->path : "<fonte>",
                  tokens->count);
#endif
    program = new_node(&parser, AST_PROGRAM, parser.synthetic_eof.span);
    if (program == NULL) return FRONTEND_STATUS_INTERNAL_ERROR;
    if (tokens->count > 0U) program->span.start = tokens->items[0].span.start;
    while (!check(&parser, TOKEN_EOF) && !parser.allocation_failed) {
        size_t before = parser.current;
        AstNode *declaration;
        if (match(&parser, TOKEN_INVALID)) continue;
        declaration = parse_global(&parser);
        if (declaration != NULL) {
            if (!append_node(&parser, &program->as.program.declarations, declaration)) break;
        } else synchronize_global(&parser, before);
    }
    program->span.end = current_token(&parser)->span.end;
    *out_program = program;
#if MACARONICA_PARSER_DEBUG
    (void)fprintf(stderr, "[debug][parser] fim: %zu declaracoes, %zu erro(s) novo(s)\n", // debug - remover
                  program->as.program.declarations.count,
                  diagnostics->error_count - errors_before);
#endif
    if (parser.allocation_failed) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK
               : FRONTEND_STATUS_SOURCE_ERROR;
}
