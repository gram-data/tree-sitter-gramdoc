/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

// The input is the whole `/** ... */` comment text, as injected from a host
// grammar. A leading `*` on each line is treated as whitespace.

// Text that runs to the end of the line, stopping before a closing `*/`.
const LINE = /[^\s*]([^\n*]|\*[^\/\n])*/;

export default grammar({
  name: 'gramdoc',

  externals: $ => [
    $.name,
  ],

  extras: $ => [
    /\s+/,
    /[ \t]*\*[ \t\n]/,
  ],

  rules: {
    document: $ => seq(
      '/**',
      optional($.name),
      optional($.description),
      repeat($._tag),
      '*/',
    ),

    // `name` is produced by src/scanner.c: an identifier alone on the first
    // line. Anything longer on that line (including punctuation) is a
    // description instead.

    description: $ => repeat1($.description_line),

    description_line: _ => token(seq(/[^@\s*]/, optional(/([^\n*]|\*[^\/\n])*/))),

    _tag: $ => choice(
      $.property_tag,
      $.alias_tag,
      $.reference_tag,
      $.unknown_tag,
    ),

    // @property <name> <description>
    property_tag: $ => seq(
      '@property',
      field('name', $.identifier),
      optional(field('description', $.tag_description)),
    ),

    // @alias <alternative name or phrase, to end of line>
    alias_tag: $ => seq(
      '@alias',
      field('value', $.alias),
    ),

    // @reference <target> <description>
    reference_tag: $ => seq(
      '@reference',
      field('target', $.reference_target),
      optional(field('description', $.tag_description)),
    ),

    // @anything <description>
    unknown_tag: $ => seq(
      field('name', $.tag_name),
      optional(field('description', $.tag_description)),
    ),

    tag_description: $ => repeat1($.description_line),

    identifier: _ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    alias: _ => LINE,

    reference_target: _ => /[^\s*]\S*/,

    tag_name: _ => /@[a-zA-Z_][a-zA-Z0-9_-]*/,
  },
});
