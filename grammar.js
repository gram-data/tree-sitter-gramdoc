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
    $.term,
    $._attribute_open,
    $.attribute_value,
    $._property_line,
    $._colon,
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
      $.language_tag,
      $.reference_tag,
      $.unknown_tag,
    ),

    // @property <name> ["term"[:]] <description> [(@key value)...]
    // The term, the description lines and the attribute groups are produced
    // by src/scanner.c, which keeps trailing groups out of the description.
    property_tag: $ => seq(
      '@property',
      field('name', $.identifier),
      optional(seq(field('term', $.term), optional($._colon))),
      optional(field('description', alias($._property_description, $.tag_description))),
      repeat(field('attribute', $.attribute)),
    ),

    // `(@aliases: "anon", "anonyme")`, `(@see [guide](https://example.com))`.
    // The value is raw text; its meaning is up to the consumer.
    attribute: $ => seq(
      $._attribute_open,
      field('key', $.attribute_key),
      optional(field('value', $.attribute_value)),
      ')',
    ),

    attribute_key: _ => /[a-zA-Z_][a-zA-Z0-9_-]*/,

    _property_description: $ => repeat1(alias($._property_line, $.description_line)),

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

    // @language <code>: applies to the whole comment; `"fr"` or `fr`
    language_tag: $ => seq(
      '@language',
      field('value', $.language),
    ),

    // @anything <description>
    unknown_tag: $ => seq(
      field('name', $.tag_name),
      optional(field('description', $.tag_description)),
    ),

    tag_description: $ => repeat1($.description_line),

    identifier: _ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    // A language code: `"fr"`, or bare `fr`, `pt-BR`.
    language: _ => token(prec(1, /"[^"\n]*"|[a-zA-Z][a-zA-Z0-9-]*/)),

    alias: _ => LINE,

    reference_target: _ => /[^\s*]\S*/,

    tag_name: _ => /@[a-zA-Z_][a-zA-Z0-9_-]*/,
  },
});
