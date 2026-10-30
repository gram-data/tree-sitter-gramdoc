#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>

enum TokenType { NAME };

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
bool tree_sitter_gramdoc_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  (void)payload;
  if (!valid_symbols[NAME]) return false;

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
