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
    /* Modo panico (recuperacao de erros): ligado pelo primeiro erro sintatico
     * de um comando e desligado quando o parser sincroniza (ver synchronize).
     * Enquanto ligado, syntax_error nao relata nada: os erros seguintes do
     * mesmo comando sao consequencia do primeiro (cascata). */
    int panic_mode;
    /* Saldo de colchetes do arquivo inteiro: quantidade de '[' menos a de ']'.
     * Negativo: falta algum '[' (ou sobra um ']'). Positivo: falta algum ']'
     * (ou sobra um '['). Decide recuperacoes que, sem essa informacao, seriam
     * um chute: supor um '[' esquecido, descartar um '[' solto, fechar um ramo
     * de cond antes do 'casocontrario'. Cada suposicao corrige o saldo. */
    long bracket_balance;
    /* Quantos blocos de ramos de cond estao abertos agora. */
    size_t open_cond_blocks;
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

/* Ultimo token consumido, ou NULL no inicio do arquivo. */
static const Token *previous_token(const Parser *parser)
{
    if (parser->current > 0U && parser->current - 1U < parser->tokens->count) {
        return &parser->tokens->items[parser->current - 1U];
    }
    return NULL;
}

static SourcePosition last_end(const Parser *parser, SourcePosition fallback)
{
    const Token *previous = previous_token(parser);
    return previous != NULL ? previous->span.end : fallback;
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
    /* Um diagnostico por erro real: em modo panico o erro e consequencia de
     * outro ja relatado no mesmo comando. */
    if (parser->panic_mode) {
        return;
    }
    parser->panic_mode = 1;
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
    /* O token errado nao e consumido: se for ')', ';' ou ']', ele ainda fecha
     * a construcao de fora, e a sincronizacao do comando cuida do resto. */
    syntax_error(parser, token, "E-SIN-001", "esperada expressao");
    return NULL;
}

/* Cada ponto acrescenta um no: a.b.c agrupa como (a.b).c. */
static AstNode *parse_field_suffixes(Parser *parser, AstNode *base)
{
    size_t field_depth = 0U;
    if (base == NULL) return NULL;
    while (check(parser, TOKEN_DOT)) {
        const Token *dot = advance(parser);
        const Token *field;
        AstNode *node;
        if (parser->expression_depth + field_depth >= PARSER_MAX_EXPRESSION_DEPTH) {
            syntax_error(parser, dot, "E-SIN-009",
                         "profundidade maxima de 512 niveis de expressao excedida");
            ast_node_destroy(base);
            return NULL;
        }
        ++field_depth;
        field = expect(parser, TOKEN_IDENTIFIER, "esperado nome do campo apos '.'");
        if (field == NULL) { ast_node_destroy(base); return NULL; }
        node = new_node(parser, AST_FIELD_EXPR, span_from(base->span.start, field->span.end));
        if (node == NULL) { ast_node_destroy(base); return NULL; }
        node->as.field_expr.base = base;
        node->as.field_expr.field_name = copy_token_text(parser, field);
        node->as.field_expr.field_span = field->span;
        if (node->as.field_expr.field_name == NULL) {
            ast_node_destroy(node); out_of_memory(parser); return NULL;
        }
        base = node;
    }
    return base;
}

static AstNode *parse_high_base(Parser *parser)
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

static AstNode *parse_high(Parser *parser)
{
    return parse_field_suffixes(parser, parse_high_base(parser));
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
    size_t links = 0U;
    while (left != NULL && (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) ||
                            check(parser, TOKEN_INTEGER_DIVIDE))) {
        const Token *op;
        if (!enter_expression_depth(parser, current_token(parser))) {
            ast_node_destroy(left); left = NULL; break;
        }
        ++links;
        op = advance(parser);
        left = make_binary(parser, left, op, parse_power(parser));
    }
    while (links != 0U) { leave_expression_depth(parser); --links; }
    return left;
}

static AstNode *parse_additive(Parser *parser)
{
    AstNode *left = parse_multiplicative(parser);
    size_t links = 0U;
    while (left != NULL && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        const Token *op;
        if (!enter_expression_depth(parser, current_token(parser))) {
            ast_node_destroy(left); left = NULL; break;
        }
        ++links;
        op = advance(parser);
        left = make_binary(parser, left, op, parse_multiplicative(parser));
    }
    while (links != 0U) { leave_expression_depth(parser); --links; }
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
    size_t links = 0U;
    while (left != NULL && check(parser, kind)) {
        const Token *op;
        if (!enter_expression_depth(parser, current_token(parser))) {
            ast_node_destroy(left); left = NULL; break;
        }
        ++links;
        op = advance(parser);
        left = make_binary(parser, left, op, lower(parser));
    }
    while (links != 0U) { leave_expression_depth(parser); --links; }
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

/* Ha um '}' adiante antes do fim do comando? Decide se vale pular uma
 * expressao usada como tamanho de vetor ate o '}' que a fecha. */
static int closing_brace_ahead(const Parser *parser)
{
    size_t distance;
    for (distance = 0U;; ++distance) {
        TokenKind kind = peek_at(parser, distance)->kind;
        if (kind == TOKEN_RIGHT_BRACE) return 1;
        if (kind == TOKEN_SEMICOLON || kind == TOKEN_LEFT_BRACKET ||
            kind == TOKEN_RIGHT_BRACKET || kind == TOKEN_EOF) return 0;
    }
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
    /* Ultimo nome aceito. `name` vira NULL quando falta o nome depois de uma
     * virgula (`inteira a, ;`); o fim do no usa `last_name`, nunca `name`. */
    const Token *last_name;
    AstNode *node;

    if (!field_or_parameter && match(parser, TOKEN_KW_VET)) {
        is_vector = 1;
        (void)expect(parser, TOKEN_LEFT_BRACE, "esperado '{' antes do tamanho do vetor");
        if (check(parser, TOKEN_LITERAL_INTEGER)) {
            const Token *size_token = advance(parser);
            vector_size = size_token->literal.inteira;
            size_span = size_token->span;
        } else {
            syntax_error(parser, current_token(parser), "E-SIN-002",
                         "esperado literal inteiro como tamanho do vetor");
        }
        if (!check(parser, TOKEN_RIGHT_BRACE) && closing_brace_ahead(parser)) {
            /* Expressao no lugar do literal (`vet{n}`, `vet{2 + 1}`): o tamanho
             * e fixo, conhecido na compilacao. Um so erro; o resto ate '}' e
             * pulado e a declaracao continua no nome do vetor. */
            syntax_error(parser, current_token(parser), "E-SIN-002",
                         "o tamanho do vetor deve ser um unico literal inteiro");
            while (!check(parser, TOKEN_RIGHT_BRACE)) (void)advance(parser);
        }
        (void)expect(parser, TOKEN_RIGHT_BRACE, "esperado '}' apos tamanho do vetor");
    }
    name = expect(parser, TOKEN_IDENTIFIER, "esperado nome na declaracao");
    if (name == NULL) { ast_type_ref_destroy(type); return NULL; }
    last_name = name;
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
            if (check(parser, TOKEN_KW_VET)) {
                /* Cada declaracao e so de vetores ou so de variaveis simples. */
                syntax_error(parser, current_token(parser), "E-SIN-002",
                             "'vet' nao pode vir depois de ','; declare o vetor em outra declaracao");
                break;
            }
            name = expect(parser, TOKEN_IDENTIFIER, "esperado nome apos ','");
            if (name == NULL) break;
            last_name = name;
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
    node->span.end = last_end(parser, last_name->span.end);
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
    if (base == NULL) return NULL;
    if (!match(parser, TOKEN_LEFT_BRACE)) return parse_field_suffixes(parser, base);
    index = parse_expression(parser);
    if (index == NULL) { ast_node_destroy(base); return NULL; }
    (void)expect(parser, TOKEN_RIGHT_BRACE, "esperado '}' apos indice do destino");
    node = new_node(parser, AST_INDEX_EXPR,
                    span_from(name->span.start, last_end(parser, index->span.end)));
    if (node == NULL) { ast_node_destroy(base); ast_node_destroy(index); return NULL; }
    node->as.index_expr.base = base;
    node->as.index_expr.index = index;
    return parse_field_suffixes(parser, node);
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

/*
 * Recuperacao local dentro de um cabecalho entre parenteses (condicao de cond
 * e durante, cabecalho de repete, parametros de funcao): pula ate o ')' que
 * fecha o cabecalho, inclusive, e para antes de 'edai', '@', '[' ou ']'. Assim
 * o corpo ainda e analisado. Nao para em ';': `repete (i receba 0; ...)` e um
 * erro comum de quem vem do C, e esse ';' nao encerra o comando.
 */
static void skip_to_header_end(Parser *parser)
{
    size_t depth = 0U;
    for (;;) {
        TokenKind kind = current_token(parser)->kind;
        if (kind == TOKEN_EOF || kind == TOKEN_KW_EDAI || kind == TOKEN_AT ||
            kind == TOKEN_LEFT_BRACKET || kind == TOKEN_RIGHT_BRACKET) {
            return;
        }
        (void)advance(parser);
        if (kind == TOKEN_LEFT_PAREN) {
            ++depth;
        } else if (kind == TOKEN_RIGHT_PAREN) {
            if (depth == 0U) return;
            --depth;
        }
    }
}

/* Fecha um cabecalho: sem erro nele e com ')', termina ali. Com erro antes,
 * ou sem o ')' agora (erro relatado por expect), pula o resto ate o ')'. Isso
 * cobre `soma(inteira a a, inteira b)`: o 'a' repetido e o erro, e os
 * parametros seguintes nao viram declaracoes globais. */
static void close_header(Parser *parser, const char *message)
{
    if (!parser->panic_mode && expect(parser, TOKEN_RIGHT_PAREN, message) != NULL) return;
    skip_to_header_end(parser);
}

static AstNode *parse_return(Parser *parser)
{
    const Token *start = advance(parser);
    int has_value = !check(parser, TOKEN_SEMICOLON);
    AstNode *value = has_value ? parse_expression(parser) : NULL;
    AstNode *node;
    (void)expect(parser, TOKEN_SEMICOLON, "esperado ';' apos respost");
    /* Valor com erro: sem no, para nao virar um `respost;` sem valor. */
    if (has_value && value == NULL) return NULL;
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
    close_header(parser, "esperado ')' apos condicao de durante");
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
    close_header(parser, "esperado ')' apos cabecalho de repete");
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

/*
 * cond ( c ) edai [ ... ] { casocontrario cond ( c ) edai [ ... ] }
 *                          [ casocontrario edai [ ... ] ]
 * A cadeia e lida ate o fim mesmo que um ramo falhe: se o parser parasse no
 * primeiro erro, cada 'casocontrario' seguinte viraria um erro solto (cascata).
 */
static AstNode *parse_if(Parser *parser)
{
    const Token *start_token = advance(parser);
    AstNode *node = new_node(parser, AST_IF_STMT, start_token->span);
    SourcePosition branch_start = start_token->span.start;
    int first_branch = 1;
    int failed = node == NULL;
    for (;;) {
        AstNode *condition;
        AstNode *body;
        (void)expect(parser, TOKEN_LEFT_PAREN, first_branch ? "esperado '(' apos cond"
                                                            : "esperado '(' apos casocontrario cond");
        condition = parse_expression(parser);
        close_header(parser, "esperado ')' apos condicao");
        (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos condicao");
        ++parser->open_cond_blocks; /* ver missing_bracket_before_else */
        body = parse_block(parser);
        --parser->open_cond_blocks;
        if (failed || condition == NULL || body == NULL) {
            ast_node_destroy(condition);
            ast_node_destroy(body);
            failed = 1;
        } else if (!append_if_branch(parser, node, condition, body, branch_start)) {
            failed = 1; /* append_if_branch ja liberou condicao e corpo */
        }
        if (!match(parser, TOKEN_KW_CASOCONTRARIO)) break;
        branch_start = previous_token(parser)->span.start;
        first_branch = 0;
        if (match(parser, TOKEN_KW_COND)) continue;
        /* casocontrario final, sem condicao */
        (void)expect(parser, TOKEN_KW_EDAI, "esperado 'edai' apos casocontrario");
        ++parser->open_cond_blocks;
        body = parse_block(parser);
        --parser->open_cond_blocks;
        if (failed || body == NULL) {
            ast_node_destroy(body);
            failed = 1;
        } else {
            node->as.if_stmt.else_body = body;
        }
        if (check(parser, TOKEN_KW_CASOCONTRARIO)) {
            syntax_error(parser, current_token(parser), "E-SIN-007",
                         "casocontrario final deve encerrar a cadeia");
        }
        break;
    }
    if (failed) {
        ast_node_destroy(node);
        return NULL;
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
    /* Nao consome: a sincronizacao do bloco pula o token, ou o bloco
     * '[ ... ]' inteiro quando o comando errado comeca em '['. */
    return NULL;
}

/*
 * Recuperacao de erros: modo panico com sincronizacao.
 *
 * O primeiro erro de um comando liga parser->panic_mode e os seguintes deixam
 * de ser relatados (ver syntax_error). Os lacos que leem sequencias (comandos
 * de um bloco, campos de um registro, declaracoes globais) chamam synchronize
 * depois de um item com erro: ela descarta tokens ate um ponto seguro e
 * desliga o modo panico. Pontos seguros:
 *   - logo depois de ';' ou de um bloco '[ ... ]' completo (fim de comando);
 *   - antes de ']' (fecha o bloco que contem o comando) e do fim do arquivo;
 *   - antes de uma palavra que so pode comecar comando ou declaracao;
 *   - antes de um identificador que abre uma linha nova com cara de comando,
 *     o que cobre o ';' esquecido no fim da linha anterior.
 * Um bloco '[ ... ]' encontrado no caminho e pulado inteiro (e os
 * 'casocontrario' que o seguem), para que o ']' dele nao feche o bloco de fora.
 * Assim `b receba 2 2;`, `inteira vet{n} v;` e `inteira a, 5;` geram um erro
 * cada, e os comandos seguintes continuam sendo analisados.
 */

/* Palavra que so pode comecar um comando (global_level = 0) ou uma
 * declaracao global (global_level = 1). Identificadores ficam de fora: no
 * meio de uma linha eles costumam ser o resto do comando com erro. */
static int is_sync_keyword(TokenKind kind, int global_level)
{
    if (is_native_value_type(kind) || kind == TOKEN_KW_VAZIO ||
        kind == TOKEN_KW_REGISTRO || kind == TOKEN_KW_OUTRAFUNCAO) {
        return 1;
    }
    if (global_level) return kind == TOKEN_KW_FUNC;
    return kind == TOKEN_KW_REPETE || kind == TOKEN_KW_DURANTE ||
           kind == TOKEN_KW_COND || kind == TOKEN_KW_RESPOST;
}

/* Identificador no inicio de uma linha seguido do que so o inicio de um
 * comando teria (`x receba`, `f(`, `v{`, `r.campo`, `Tipo nome`) ou, no nivel
 * global, de uma declaracao de tipo registro (`Tipo nome`, `Tipo principal`). */
static int identifier_starts_line(const Parser *parser, int global_level)
{
    const Token *token = current_token(parser);
    const Token *previous = previous_token(parser);
    TokenKind next = peek_at(parser, 1U)->kind;
    if (token->kind != TOKEN_IDENTIFIER || previous == NULL ||
        token->span.start.line <= previous->span.end.line) {
        return 0;
    }
    if (next == TOKEN_IDENTIFIER || next == TOKEN_KW_PRINCIPAL || next == TOKEN_KW_VET) {
        return 1;
    }
    return !global_level && (next == TOKEN_KW_RECEBA || next == TOKEN_LEFT_PAREN ||
                             next == TOKEN_LEFT_BRACE || next == TOKEN_DOT);
}

/* Pula um bloco '[ ... ]' inteiro, com os aninhados. E iterativo: um
 * aninhamento enorme nao estoura a pilha. Comeca com o token atual em '['. */
static void skip_bracketed_block(Parser *parser)
{
    size_t depth = 0U;
    while (!check(parser, TOKEN_EOF)) {
        TokenKind kind = advance(parser)->kind;
        if (kind == TOKEN_LEFT_BRACKET) {
            ++depth;
        } else if (kind == TOKEN_RIGHT_BRACKET && depth > 0U && --depth == 0U) {
            return;
        }
    }
}

/* 'casocontrario' no inicio de um comando, dentro do bloco de um ramo de cond,
 * com ']' faltando no arquivo: o ']' do ramo foi esquecido. O bloco termina
 * ali (com um erro) e o cond continua no 'casocontrario'. */
static int missing_bracket_before_else(const Parser *parser)
{
    return check(parser, TOKEN_KW_CASOCONTRARIO) && parser->open_cond_blocks > 0U &&
           parser->bracket_balance > 0;
}

static void synchronize(Parser *parser, size_t failed_at, int global_level)
{
    /* Um item que falhou sem consumir nada perde ao menos um token (ou o
     * bloco que comeca nele): garante que o laco de fora sempre avanca. */
    int progressed = parser->current != failed_at;
    for (;;) {
        const Token *previous = previous_token(parser);
        TokenKind kind = current_token(parser)->kind;
        if (kind == TOKEN_EOF || (kind == TOKEN_RIGHT_BRACKET && !global_level)) break;
        if (progressed && !global_level) {
            /* Fim de comando. No nivel global estas regras nao valem: la, ';'
             * e ']' costumam encerrar comandos soltos (depois de um ']' a
             * mais), e parar neles faria cada comando virar um erro. */
            if (previous != NULL && previous->kind == TOKEN_SEMICOLON) break;
            /* Depois de um bloco: para, exceto se vier 'casocontrario' (o
             * resto do cond) ou ';' (o bloco estava no meio de um comando,
             * como em `a receba [ 1 ];`, e o ';' fecha esse comando). */
            if (previous != NULL && previous->kind == TOKEN_RIGHT_BRACKET &&
                kind != TOKEN_KW_CASOCONTRARIO && kind != TOKEN_SEMICOLON) break;
            if (missing_bracket_before_else(parser)) break;
        }
        if (progressed && (is_sync_keyword(kind, global_level) ||
                           identifier_starts_line(parser, global_level))) break;
        if (kind == TOKEN_LEFT_BRACKET && !progressed && parser->bracket_balance > 0) {
            /* O item com erro comeca num '[' e sobra '[' no arquivo: e um '['
             * solto. Descarta so ele; o conteudo segue no bloco atual e o ']'
             * dele fecha esse bloco, como devia. */
            --parser->bracket_balance;
            (void)advance(parser);
        } else if (kind == TOKEN_LEFT_BRACKET) {
            skip_bracketed_block(parser);
        } else {
            (void)advance(parser);
        }
        progressed = 1;
    }
    parser->panic_mode = 0;
}

/* Palavras que so comecam uma declaracao global. Dentro de um bloco ou de um
 * registro, indicam que o ']' que o fecharia foi esquecido. */
static int starts_global_only(const Parser *parser)
{
    TokenKind kind = current_token(parser)->kind;
    if (kind == TOKEN_KW_REGISTRO || kind == TOKEN_KW_OUTRAFUNCAO) return 1;
    return is_type_start(kind, 1) && peek_at(parser, 1U)->kind == TOKEN_KW_PRINCIPAL &&
           peek_at(parser, 2U)->kind == TOKEN_LEFT_PAREN;
}

/* Token que pode abrir o conteudo de um bloco: inicio de comando, ou o ']'
 * de um bloco vazio. */
static int starts_block_content(const Parser *parser)
{
    TokenKind kind = current_token(parser)->kind;
    return looks_like_local_declaration(parser) || kind == TOKEN_IDENTIFIER ||
           kind == TOKEN_KW_REPETE || kind == TOKEN_KW_DURANTE ||
           kind == TOKEN_KW_COND || kind == TOKEN_KW_RESPOST ||
           kind == TOKEN_RIGHT_BRACKET;
}

/* Le os comandos de um bloco ate o ']' (que fica para quem chamou), o fim do
 * arquivo, uma declaracao global ou o 'casocontrario' de um ramo sem ']'.
 * Devolve 0 so em falta de memoria. */
static int parse_block_items(Parser *parser, AstNode *block)
{
    while (!check(parser, TOKEN_RIGHT_BRACKET) && !check(parser, TOKEN_EOF) &&
           !starts_global_only(parser) && !missing_bracket_before_else(parser) &&
           !parser->allocation_failed) {
        size_t before = parser->current;
        AstNode *statement;
        if (match(parser, TOKEN_INVALID)) continue;
        statement = parse_statement(parser);
        if (statement != NULL &&
            !append_node(parser, &block->as.block.statements, statement)) {
            return 0;
        }
        /* Comando com erro: pula ate o proximo ponto seguro. */
        if (statement == NULL || parser->panic_mode) synchronize(parser, before, 0);
    }
    return !parser->allocation_failed;
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
    if (check(parser, TOKEN_LEFT_BRACKET)) {
        open = advance(parser);
        /* Um '[' de verdade abre comandos novos: um erro dentro do bloco nao e
         * cascata de um erro anterior no cabecalho (condicao, parametros). */
        parser->panic_mode = 0;
    } else {
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado '[' para iniciar bloco");
        /* Falta '[' no arquivo (saldo negativo) e o que vem parece conteudo de
         * bloco: le o bloco como se o '[' estivesse la, para que o ']' dele
         * feche este bloco, e nao o de fora. Sem essa evidencia (como em
         * `cond (a) edai respost;`, sem colchete algum), o bloco e abandonado
         * e quem chamou sincroniza. */
        if (parser->bracket_balance >= 0 || !starts_block_content(parser)) goto finish;
        ++parser->bracket_balance;
        open = current_token(parser);
    }
    block = new_node(parser, AST_BLOCK, open->span);
    if (block == NULL) goto finish;
    if (!parse_block_items(parser, block)) {
        ast_node_destroy(block);
        block = NULL;
        goto finish;
    }
    if (check(parser, TOKEN_RIGHT_BRACKET)) {
        block->span.end = advance(parser)->span.end;
    } else {
        /* Fim do arquivo, declaracao global ou 'casocontrario' de um ramo: o
         * ']' foi esquecido. O bloco termina aqui e o saldo e corrigido. */
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado ']' para encerrar bloco");
        --parser->bracket_balance;
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
    if (match(parser, TOKEN_LEFT_BRACKET)) {
        /* Como em parse_block: o '[' abre campos novos. */
        parser->panic_mode = 0;
    } else {
        /* Sem '[': como em parse_block, le os campos mesmo assim so quando
         * falta '[' no arquivo e o que vem parece campo. */
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado '[' para iniciar registro");
        if (parser->bracket_balance >= 0 ||
            !(is_type_start(current_token(parser)->kind, 0) || check(parser, TOKEN_RIGHT_BRACKET))) {
            ast_node_destroy(node); return NULL;
        }
        ++parser->bracket_balance;
    }
    /* O laco para numa declaracao global: o ']' do registro foi esquecido. */
    while (!check(parser, TOKEN_RIGHT_BRACKET) && !check(parser, TOKEN_EOF) &&
           !starts_global_only(parser) && !parser->allocation_failed) {
        size_t before = parser->current;
        AstNode *field = NULL;
        if (!is_type_start(current_token(parser)->kind, 0)) {
            syntax_error(parser, current_token(parser), "E-SIN-001", "esperada declaracao de campo");
        } else {
            field = parse_value_declaration(parser, 1);
            if (field != NULL && !append_node(parser, &node->as.struct_decl.fields, field)) {
                ast_node_destroy(node); return NULL;
            }
        }
        /* Campo com erro: pula ate o proximo ponto seguro (ver synchronize). */
        if (field == NULL || parser->panic_mode) synchronize(parser, before, 0);
    }
    if (check(parser, TOKEN_RIGHT_BRACKET)) node->span.end = advance(parser)->span.end;
    else {
        syntax_error(parser, current_token(parser), "E-SIN-002", "esperado ']' para encerrar registro");
        --parser->bracket_balance;
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

/* Pula o resto de um cabecalho de funcao com erro: ate o '@' (inclusive) ou
 * ate o '[' do corpo, sem passar de ']', do fim do arquivo ou de outra
 * declaracao global. */
static void skip_function_header(Parser *parser)
{
    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_LEFT_BRACKET) &&
           !check(parser, TOKEN_RIGHT_BRACKET) && !starts_global_only(parser)) {
        if (advance(parser)->kind == TOKEN_AT) return;
    }
}

static AstNode *parse_function_after_type(Parser *parser, AstTypeRef *return_type,
                                          SourcePosition start, int is_principal)
{
    const Token *name;
    AstNode *node;
    int parameters_ok;
    if (is_principal) {
        name = expect(parser, TOKEN_KW_PRINCIPAL, "esperado 'principal'");
    } else {
        name = expect(parser, TOKEN_IDENTIFIER, "esperado nome da funcao");
    }
    if (name == NULL) {
        /* Sem nome (`outrafuncao inteira (`, `outrafuncao inteira inteira
         * soma(`): pula o resto do cabecalho e le o corpo mesmo assim, para
         * que os comandos dele nao sejam tomados por declaracoes globais. */
        ast_type_ref_destroy(return_type);
        skip_function_header(parser);
        ast_node_destroy(parse_block(parser));
        return NULL;
    }
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
    /* Parametro com erro nao abandona a funcao: o resto do cabecalho e pulado
     * ate o ')' (close_header) e o corpo ainda e analisado, para que os erros
     * reais dele aparecam sem cascata vinda do cabecalho. */
    parameters_ok = parse_parameters(parser, &node->as.function_decl.parameters);
    if (parser->allocation_failed) { ast_node_destroy(node); return NULL; }
    close_header(parser, "esperado ')' apos parametros");
    (void)expect(parser, TOKEN_AT, "esperado '@' antes do corpo da funcao");
    node->as.function_decl.body = parse_block(parser);
    if (!parameters_ok || node->as.function_decl.body == NULL) {
        ast_node_destroy(node); return NULL;
    }
    node->span.end = node->as.function_decl.body->span.end;
    return node;
}

/* No nivel global: o token atual comeca um comando (nao uma declaracao), vem
 * logo depois de um ']' e sobra ']' no arquivo. Esse ']' fechou a funcao
 * antes da hora. */
static int stray_bracket_before(const Parser *parser)
{
    const Token *previous = previous_token(parser);
    TokenKind kind = current_token(parser)->kind;
    TokenKind next = peek_at(parser, 1U)->kind;
    if (parser->bracket_balance >= 0 || previous == NULL ||
        previous->kind != TOKEN_RIGHT_BRACKET) {
        return 0;
    }
    if (kind == TOKEN_KW_REPETE || kind == TOKEN_KW_DURANTE || kind == TOKEN_KW_COND ||
        kind == TOKEN_KW_RESPOST || kind == TOKEN_KW_CASOCONTRARIO) {
        return 1;
    }
    return kind == TOKEN_IDENTIFIER &&
           (next == TOKEN_KW_RECEBA || next == TOKEN_LEFT_PAREN ||
            next == TOKEN_LEFT_BRACE || next == TOKEN_DOT);
}

static AstNode *parse_global(Parser *parser)
{
    SourcePosition start = current_token(parser)->span.start;
    AstTypeRef type;
    if (check(parser, TOKEN_KW_REGISTRO)) return parse_struct(parser);
    if (match(parser, TOKEN_KW_OUTRAFUNCAO)) {
        if (!parse_type(parser, 1, &type)) return NULL;
        return parse_function_after_type(parser, &type, start, 0);
    }
    if (check(parser, TOKEN_KW_FUNC)) {
        /* 'func' e erro, mas o resto e lido como 'outrafuncao' (ou como
         * 'principal'): o corpo ainda e analisado e nao ha cascata. O modo
         * panico segue ligado ate o '[' do corpo, que o desliga. */
        syntax_error(parser, current_token(parser), "E-SIN-004",
                     "'func' nao possui producao; use 'outrafuncao' ou 'principal'");
        (void)advance(parser);
        if (!parse_type(parser, 1, &type)) return NULL;
        return parse_function_after_type(parser, &type, start,
                                         check(parser, TOKEN_KW_PRINCIPAL));
    }
    if (stray_bracket_before(parser)) {
        /* Um erro so, no primeiro comando fora da funcao. Os comandos que
         * seguem, ate o ']' que fecharia a funcao, sao lidos como bloco e
         * descartados, em vez de virarem declaracoes globais erradas. */
        AstNode *orphan;
        syntax_error(parser, current_token(parser), "E-SIN-001",
                     "comando fora de funcao: um ']' a mais antes daqui fechou a funcao");
        ++parser->bracket_balance;
        orphan = new_node(parser, AST_BLOCK, current_token(parser)->span);
        if (orphan != NULL) {
            (void)parse_block_items(parser, orphan);
            ast_node_destroy(orphan);
        }
        if (check(parser, TOKEN_RIGHT_BRACKET)) (void)advance(parser);
        return NULL;
    }
    if (!is_type_start(current_token(parser)->kind, 1)) {
        /* Nao consome: synchronize pula o token (ou o bloco '[ ... ]'). */
        syntax_error(parser, current_token(parser), "E-SIN-001", "esperada declaracao global");
        return NULL;
    }
    if (!parse_type(parser, 1, &type)) return NULL;
    if (check(parser, TOKEN_KW_PRINCIPAL)) {
        return parse_function_after_type(parser, &type, start, 1);
    }
    if (check(parser, TOKEN_IDENTIFIER) && peek_at(parser, 1U)->kind == TOKEN_LEFT_PAREN) {
        /* `inteira soma(...)` sem 'outrafuncao': um erro, e a funcao e lida
         * inteira, para que parametros e corpo nao virem declaracoes globais. */
        syntax_error(parser, current_token(parser), "E-SIN-002",
                     "declaracao de funcao exige 'outrafuncao' antes do tipo de retorno");
        return parse_function_after_type(parser, &type, start, 0);
    }
    if (type.name != NULL && strcmp(type.name, "vazio") == 0) {
        syntax_error(parser, current_token(parser), "E-SIN-008",
                     "tipo vazio somente pode ser retorno de funcao");
        ast_type_ref_destroy(&type); return NULL;
    }
    return parse_declaration_after_type(parser, &type, start, 0, 1);
}

FrontendStatus parser_parse(const SourceFile *source,
                            const TokenBuffer *tokens,
                            DiagnosticBag *diagnostics,
                            AstNode **out_program)
{
    Parser parser;
    AstNode *program;
    size_t errors_before;
    size_t index;
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
    /* Saldo de colchetes, usado pela recuperacao de erros (ver Parser). */
    for (index = 0U; index < tokens->count; ++index) {
        if (tokens->items[index].kind == TOKEN_LEFT_BRACKET) ++parser.bracket_balance;
        else if (tokens->items[index].kind == TOKEN_RIGHT_BRACKET) --parser.bracket_balance;
    }
    program = new_node(&parser, AST_PROGRAM, parser.synthetic_eof.span);
    if (program == NULL) return FRONTEND_STATUS_INTERNAL_ERROR;
    if (tokens->count > 0U) program->span.start = tokens->items[0].span.start;
    while (!check(&parser, TOKEN_EOF) && !parser.allocation_failed) {
        size_t before = parser.current;
        AstNode *declaration;
        if (match(&parser, TOKEN_INVALID)) continue;
        declaration = parse_global(&parser);
        if (declaration != NULL &&
            !append_node(&parser, &program->as.program.declarations, declaration)) break;
        /* Declaracao com erro (ou que deixou o modo panico ligado, como um
         * bloco sem ']'): pula ate a proxima declaracao global. */
        if (declaration == NULL || parser.panic_mode) synchronize(&parser, before, 1);
    }
    program->span.end = current_token(&parser)->span.end;
    *out_program = program;
    if (parser.allocation_failed) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK
               : FRONTEND_STATUS_SOURCE_ERROR;
}
