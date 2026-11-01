# tree-sitter-gramdoc

[Tree-sitter](https://tree-sitter.github.io/tree-sitter/) grammar for structured `/** ... */` documentation comments in [gram](https://github.com/gram-data/tree-sitter-gram) files.

> **Origin.** This repository came out of the tree-sitter-gram proposal
> [gramdoc — structured documentation comments for gram](https://github.com/gram-data/tree-sitter-gram/blob/proposal/tree-sitter-gramdoc/proposals/tree-sitter-gramdoc.md).
> It is modeled on `cypherdoc` from [tree-sitter-cypher](https://github.com/gram-data/tree-sitter-cypher).
> The proposal's open questions (block comments, attachment rules, context markers, linting) concern the host grammar and consumers, not this grammar.

## Comment format

```gram
/**
 * A human individual.
 *
 * @property name The full name of the person.
 */
(Person {
    name: "STRING!"
})

/**
 * Un être humain.
 *
 * @language "fr"
 * @property name "nom": Le nom complet de la personne. (@aliases: "anon")
 */
(Person)
```

A sequence of elements, documented by position:

```gram
/**
 * @elements The members of the team.
 * @element The team lead.
 * @element A regular member.
 */
[team:Team | (lead), (member)]
```

The parser input is the whole `/** ... */` text. A leading `*` on each line is whitespace, so lines may omit it.

| Part | Node | Description |
|------|------|-------------|
| name | `name` | Optional. An identifier **alone** on the first line (`Person`). `Person.` or `A person` is a description instead. |
| description | `description` | Free prose, one `description_line` per line. Blank lines are skipped, not represented. Plain text, no Markdown semantics. |
| `@property <name> ["term"[:]] [description] [(@key value)...]` | `property_tag` | Documents a property of the element, in the shape of a dictionary entry: `@property name "nom": The full name. (@aliases: "anon", "anonyme")`. The optional quoted `term` is the property's localized name; a `:` right after it is allowed and skipped. Trailing `(@key value)` groups are attributes. A quote or parenthetical anywhere else is description text: `Note: ...`, `(see below)` and a closing Markdown link stay in the description. Fields: `name` (`identifier`), `term` (`term`, quotes included), `description` (`tag_description`), `attribute` (repeatable, see below). |
| `@label <name> [description]` | `label_tag` | Documents a label of the subject. Fields: `name` (`identifier`), `description`. |
| `@alias <text>` | `alias_tag` | One alternative name per tag; the value is the rest of the line, spaces allowed. Field: `value` (`alias`). |
| `@reference <target> [description]` | `reference_tag` | An external reference. `target` is one whitespace-free token (URL, IRI, CURIE…). Fields: `target` (`reference_target`), `description`. |
| `@elements [description]` | `elements_tag` | Intent of the pattern's element sequence as a whole. Field: `description`. |
| `@element [description]` | `element_tag` | Repeatable. The nth `@element` documents the nth element, by order of appearance; there is no addressing token. A count mismatch is not a parse error. Field: `description`. |
| `@language <code>` | `language_tag` | The language of the whole comment: `"fr"` or bare `fr`, `pt-BR`. Field: `value` (`language`). |
| `@<other> [description]` | `unknown_tag` | Any other tag. Fields: `name` (`tag_name`, includes the `@`), `description`. |

**Attributes.** An attribute is a parenthesized group `(@key value)` or `(@key: value)` at the end of a `@property` tag, after the description. It has a `key` (`attribute_key`, no `@`) and an optional `value` (`attribute_value`): the raw text up to the closing `)`, with an optional `:` after the key skipped. Nested parentheses must balance, so `(@see [guide](https://example.com/a_(1)))` works. A group may span lines, and several may follow one another. gramdoc does not define any key; `aliases`, `see` and the rest are conventions between authors and consumers, and a consumer may read a value as gram scalars such as `"anon", "anonyme"`. A `(@` group that is not the last thing on its line is description text, and text after an attribute on a later line is a parse error: attributes end their tag.

**Markdown.** The `description` and every tag description are CommonMark. The grammar does not parse it: each line is a `description_line` whose range excludes the comment decoration, so a consumer joins the lines (a skipped blank line is the gap between two ranges) and hands them to a Markdown parser. `queries/injections.scm` injects Markdown line by line for inline highlighting. The `term`, the `aliases`, `@alias` and the name are plain text. Three limits come from the comment form:

 * The decoration is one `*` plus whitespace at the start of a line; any further `*` is text, so ` * * item` is a bullet and ` * **bold**` is emphasis. A bare `* item` on a line with no decoration loses its bullet.
 * `*/` ends the comment and cannot be written, even in a code span.
 * A line that starts with `@` starts a tag, so a Markdown `@mention` cannot begin a line.

Tag descriptions continue over following lines until the next `@` tag or the end of the comment. Tags have no `{type}` and no `-` separator.

`@source`, `@target` and `@self` are not special: they parse as unknown tags. Structure such as cardinality belongs in an annotation on the definition, not in the comment.

`/**/` is not a doc comment (the host grammar treats it as an empty plain block, if at all).

## Using it from a host grammar

The host grammar exposes `/** ... */` as a single node (e.g. `doc_comment` in tree-sitter-gram) and injects this grammar with `queries/injections.scm`:

```scm
((doc_comment) @injection.content
  (#set! injection.language "gramdoc"))
```

The injected content must be the whole comment including `/**` and `*/`. Editors without injection support still see an ordinary comment.

## Install

```sh
# Node (native binding; builds with node-gyp if no prebuild matches)
npm install @gram-data/tree-sitter-gramdoc tree-sitter

# Rust
cargo add tree-sitter-gramdoc
```

```js
const Parser = require("tree-sitter");
const parser = new Parser();
parser.setLanguage(require("@gram-data/tree-sitter-gramdoc"));
```

```rust
let mut parser = tree_sitter::Parser::new();
parser.set_language(&tree_sitter_gramdoc::LANGUAGE.into())?;
```

The npm package is `@gram-data/tree-sitter-gramdoc`; the crate is `tree-sitter-gramdoc`.

## Queries

- `queries/highlights.scm`: name, tag keywords, property names and terms, aliases, reference targets, descriptions.
- `queries/injections.scm`: Markdown in description lines.
- `queries/tags.scm`: `@definition.doc` (name), `@definition.property`, `@definition.alias`, `@reference.reference`, each with a `@name` capture.

## Development

```sh
npm install
npx tree-sitter generate
npx tree-sitter test
node --test bindings/node/*_test.js
cargo test
```

`name` is produced by a tiny external scanner (`src/scanner.c`), because "identifier alone on its line" cannot be expressed as a plain token.

Corpus tests live in `test/corpus/` split by concern: `names.txt`, `descriptions.txt`, `tags.txt`.

## License

MIT, see [LICENSE](LICENSE).
