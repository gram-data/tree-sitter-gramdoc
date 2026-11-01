#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>

enum TokenType { NAME, TERM, ALIASES, PROPERTY_LINE, COLON };

void *tree_sitter_gramdoc_external_scanner_create(void) { return NULL; }
void tree_sitter_gramdoc_external_scanner_destroy(void *payload) { (void)payload; }
unsigned tree_sitter_gramdoc_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}
void tree_sitter_gramdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

static bool is_space(int32_t c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static bool is_ident_start(int32_t c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_ident(int32_t c) { return is_ident_start(c) || (c >= '0' && c <= '9'); }

// Scans `name`: an identifier that is alone on its line, directly after the
// opening `/**` (skipping whitespace and leading `*` decoration).
static bool scan_name(TSLexer *lexer) {
  for (;;) {
    if (is_space(lexer->lookahead)) {
      lexer->advance(lexer, true);
    } else if (lexer->lookahead == '*') {
      lexer->advance(lexer, true);
      if (lexer->lookahead == '/') return false;
    } else {
      break;
    }
  }

  if (!is_ident_start(lexer->lookahead)) return false;
  while (is_ident(lexer->lookahead)) lexer->advance(lexer, false);
  lexer->mark_end(lexer);

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
    lexer->advance(lexer, false);
  }

  bool alone = lexer->lookahead == '\n' || lexer->eof(lexer);
  if (!alone && lexer->lookahead == '*') {
    lexer->advance(lexer, false);
    alone = lexer->lookahead == '/';
  }
  if (!alone) return false;

  lexer->result_symbol = NAME;
  return true;
}

static void skip_blanks(TSLexer *lexer) {
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
    lexer->advance(lexer, false);
  }
}

// At `(`: consumes a parenthesized list of quoted strings that ends its line,
// `("a", "b")`. On failure the lexer is left at the first character that did
// not fit, which is not consumed. With `mark`, the token end is set right
// after the `)`.
static bool scan_aliases(TSLexer *lexer, bool mark) {
  lexer->advance(lexer, false);
  for (;;) {
    skip_blanks(lexer);
    if (lexer->lookahead != '"') return false;
    lexer->advance(lexer, false);
    while (lexer->lookahead != '"') {
      if (lexer->lookahead == '\n' || lexer->eof(lexer)) return false;
      lexer->advance(lexer, false);
    }
    lexer->advance(lexer, false);
    skip_blanks(lexer);
    if (lexer->lookahead == ',') {
      lexer->advance(lexer, false);
    } else if (lexer->lookahead == ')') {
      lexer->advance(lexer, false);
      break;
    } else {
      return false;
    }
  }
  if (mark) lexer->mark_end(lexer);
  skip_blanks(lexer);
  if (lexer->lookahead == '\n' || lexer->eof(lexer)) return true;
  if (lexer->lookahead == '*') {
    lexer->advance(lexer, false);
    return lexer->lookahead == '/';
  }
  return false;
}

// Scans the pieces of a `@property` tag after its name:
//   TERM           a quoted localized term, `"nom"`
//   COLON          an optional `:` right after the term
//   PROPERTY_LINE  description text to the end of the line, stopping before a
//                  trailing alias list and before `*/`
//   ALIASES        a trailing list of quoted strings, `("anon", "anonyme")`
// One leading `*` per line is comment decoration; any further `*` is text, so
// Markdown bullets and emphasis survive.
static bool scan_property(TSLexer *lexer, const bool *valid_symbols) {
  bool line_start = false;
  for (;;) {
    if (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
      lexer->advance(lexer, true);
    } else if (lexer->lookahead == '\n') {
      lexer->advance(lexer, true);
      line_start = true;
    } else if (lexer->lookahead == '*' && line_start) {
      lexer->advance(lexer, true);
      if (lexer->lookahead == '/') return false;
      line_start = false;
    } else {
      break;
    }
  }
  if (lexer->eof(lexer) || lexer->lookahead == '@') return false;

  if (valid_symbols[COLON] && lexer->lookahead == ':' && !line_start) {
    lexer->advance(lexer, false);
    lexer->mark_end(lexer);
    lexer->result_symbol = COLON;
    return true;
  }

  bool has_text = false;
  if (valid_symbols[TERM] && lexer->lookahead == '"') {
    lexer->advance(lexer, false);
    while (lexer->lookahead != '"' && lexer->lookahead != '\n' && !lexer->eof(lexer)) {
      lexer->advance(lexer, false);
    }
    if (lexer->lookahead == '"') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      lexer->result_symbol = TERM;
      return true;
    }
    // An unclosed quote is ordinary description text.
    has_text = true;
    lexer->mark_end(lexer);
  }

  if (valid_symbols[ALIASES] && !has_text && lexer->lookahead == '(' && scan_aliases(lexer, true)) {
    lexer->result_symbol = ALIASES;
    return true;
  }
  if (!valid_symbols[PROPERTY_LINE]) return false;

  for (;;) {
    int32_t c = lexer->lookahead;
    if (c == '\n' || lexer->eof(lexer)) break;
    if (c == '*') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '/') break;
      has_text = true;
      lexer->mark_end(lexer);
    } else if (c == '(' && valid_symbols[ALIASES]) {
      if (scan_aliases(lexer, false)) {
        if (!has_text) return false;
        break;
      }
      has_text = true;
      lexer->mark_end(lexer);
    } else {
      lexer->advance(lexer, false);
      if (!is_space(c)) {
        has_text = true;
        lexer->mark_end(lexer);
      }
    }
  }
  if (!has_text) return false;
  lexer->result_symbol = PROPERTY_LINE;
  return true;
}

bool tree_sitter_gramdoc_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  (void)payload;
  // During error recovery every symbol is valid; stay out of it.
  if (valid_symbols[NAME] && valid_symbols[ALIASES]) return false;
  if (valid_symbols[NAME]) return scan_name(lexer);
  if (valid_symbols[TERM] || valid_symbols[COLON] || valid_symbols[ALIASES] || valid_symbols[PROPERTY_LINE]) {
    return scan_property(lexer, valid_symbols);
  }
  return false;
}
