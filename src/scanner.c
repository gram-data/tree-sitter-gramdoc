#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>

enum TokenType { NAME, TERM, ATTRIBUTE_OPEN, ATTRIBUTE_VALUE, PROPERTY_LINE, COLON };

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

// After a group (and blanks): true at the end of the line or before `*/`, so
// the group is the last thing on its line. Anything after it on a later line
// is not description text any more: attributes end their tag.
static bool at_line_end(TSLexer *lexer) {
  skip_blanks(lexer);
  if (lexer->eof(lexer) || lexer->lookahead == '\n') return true;
  if (lexer->lookahead == '*') {
    lexer->advance(lexer, false);
    return lexer->lookahead == '/';
  }
  return false;
}

// At `(`: true when a run of `(@key ...)` groups starts here and is the last
// thing on its line. A group may span lines; nested parentheses must balance,
// so Markdown links fit inside. With `mark`, the token end is set right after
// `(@`. On failure the lexer is left at the first character that did not fit,
// which is not consumed.
static bool scan_attributes(TSLexer *lexer, bool mark) {
  for (;;) {
    lexer->advance(lexer, false);
    if (lexer->lookahead != '@') return false;
    lexer->advance(lexer, false);
    if (mark) {
      lexer->mark_end(lexer);
      mark = false;
    }
    if (!is_ident_start(lexer->lookahead)) return false;
    int depth = 1;
    while (depth > 0) {
      if (lexer->eof(lexer)) return false;
      if (lexer->lookahead == '(') depth++;
      if (lexer->lookahead == ')') depth--;
      if (lexer->lookahead == '*') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '/') return false;
        continue;
      }
      lexer->advance(lexer, false);
    }
    skip_blanks(lexer);
    if (lexer->lookahead != '(') return at_line_end(lexer);
  }
}

// Scans the value of an attribute: everything after the key (and an optional
// `:`) up to the `)` that closes the group, nested parentheses included.
static bool scan_attribute_value(TSLexer *lexer) {
  for (;;) {
    if (is_space(lexer->lookahead)) lexer->advance(lexer, true);
    else break;
  }
  if (lexer->lookahead == ':') {
    lexer->advance(lexer, true);
    while (is_space(lexer->lookahead)) lexer->advance(lexer, true);
  }
  bool has_text = false;
  int depth = 0;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    if (c == '(') depth++;
    if (c == ')') {
      if (depth == 0) break;
      depth--;
    }
    lexer->advance(lexer, false);
    if (!is_space(c)) {
      has_text = true;
      lexer->mark_end(lexer);
    }
  }
  if (!has_text) return false;
  lexer->result_symbol = ATTRIBUTE_VALUE;
  return true;
}

// Scans the pieces of a `@property` tag after its name:
//   TERM           a quoted localized term, `"nom"`
//   COLON          an optional `:` right after the term
//   PROPERTY_LINE  description text to the end of the line, stopping before a
//                  trailing attribute group and before `*/`
//   ATTRIBUTE_OPEN the `(@` that starts a trailing `(@key value)` group
//   ATTRIBUTE_VALUE the raw value inside such a group
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

  if (valid_symbols[ATTRIBUTE_OPEN] && !has_text && lexer->lookahead == '(' && scan_attributes(lexer, true)) {
    lexer->result_symbol = ATTRIBUTE_OPEN;
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
    } else if (c == '(' && valid_symbols[ATTRIBUTE_OPEN]) {
      if (scan_attributes(lexer, false)) {
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
  if (valid_symbols[NAME] && valid_symbols[ATTRIBUTE_OPEN]) return false;
  if (valid_symbols[NAME]) return scan_name(lexer);
  if (valid_symbols[ATTRIBUTE_VALUE]) return scan_attribute_value(lexer);
  if (valid_symbols[TERM] || valid_symbols[COLON] || valid_symbols[ATTRIBUTE_OPEN] || valid_symbols[PROPERTY_LINE]) {
    return scan_property(lexer, valid_symbols);
  }
  return false;
}
