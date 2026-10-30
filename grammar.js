/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

// The input is the whole `/** ... */` comment text, as injected from a host
// grammar. A leading `*` on each line is treated as whitespace.

// Text that runs to the end of the line, stopping before a closing `*/`.
const LINE = /[^\s*]([^\n*]|\*[^\/\n])*/;

module.exports = grammar({
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
      $.label_tag,
      $.alias_tag,
      $.elements_tag,
      $.element_tag,
      $.source_tag,
      $.target_tag,
      $.self_tag,
      $.reference_tag,
      $.unknown_tag,
    ),

    // @property <name> <description>
    property_tag: $ => seq(
      '@property',
      field('name', $.identifier),
      optional(field('description', $.tag_description)),
    ),

    // @label <name> <description>
    label_tag: $ => seq(
      '@label',
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

    // @elements <description>: the sequence of pattern elements as a whole
    elements_tag: $ => seq(
      '@elements',
      optional(field('description', $.tag_description)),
    ),

    // @element <description>: repeatable, the nth documents the nth element
    element_tag: $ => seq(
      '@element',
      optional(field('description', $.tag_description)),
    ),

    // @source <cardinality> <description>
    source_tag: $ => seq(
      '@source',
      field('cardinality', $.cardinality),
      optional(field('description', $.tag_description)),
    ),

    // @target <cardinality> <description>
    target_tag: $ => seq(
      '@target',
      field('cardinality', $.cardinality),
      optional(field('description', $.tag_description)),
    ),

    // @self: bare marker, no argument or description
    self_tag: _ => '@self',

    // @anything <description>
    unknown_tag: $ => seq(
      field('name', $.tag_name),
      optional(field('description', $.tag_description)),
    ),

    tag_description: $ => repeat1($.description_line),

    identifier: _ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    // n, n..m, n.., ..m, ?, +, *. Higher precedence so a bare `*` is not
    // consumed as the leading-asterisk extra.
    cardinality: _ => token(prec(1, /[0-9]+(\.\.[0-9]*)?|\.\.[0-9]+|\?|\+|\*/)),

    alias: _ => LINE,

    reference_target: _ => /[^\s*]\S*/,

    tag_name: _ => /@[a-zA-Z_][a-zA-Z0-9_-]*/,
  },
});
