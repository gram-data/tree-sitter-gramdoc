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
 * @property name Le nom complet de la personne.
 */
[:French {
      name: "nom"
} | Person ]
```

A relationship, documented by its ends:

```gram
/**
 * KNOWS
 *
 * A person knows another person.
 *
 * @label KNOWS
 * @source 1..* Each :KNOWS may start at one or more people.
 * @target 0..1 Each :KNOWS ends at at most one person.
 */
[:KNOWS | (a:Person), (b:Person)]
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
| `@property <name> [description]` | `property_tag` | Documents a property of the element. Fields: `name` (`identifier`), `description` (`tag_description`). |
| `@label <name> [description]` | `label_tag` | Documents a label of the subject. Fields: `name` (`identifier`), `description`. |
| `@alias <text>` | `alias_tag` | One alternative name per tag; the value is the rest of the line, spaces allowed. Field: `value` (`alias`). |
| `@reference <target> [description]` | `reference_tag` | An external reference. `target` is one whitespace-free token (URL, IRI, CURIE…). Fields: `target` (`reference_target`), `description`. |
| `@elements [description]` | `elements_tag` | Intent of the pattern's element sequence as a whole. Field: `description`. |
| `@element [description]` | `element_tag` | Repeatable. The nth `@element` documents the nth element, by order of appearance; there is no addressing token. A count mismatch is not a parse error. Field: `description`. |
| `@source <cardinality> [description]` | `source_tag` | Source end of a relationship `[r \| source, target]`. Fields: `cardinality` (`cardinality`), `description`. |
| `@target <cardinality> [description]` | `target_tag` | Target end of a relationship. Same shape as `@source`. |
| `@<other> [description]` | `unknown_tag` | Any other tag. Fields: `name` (`tag_name`, includes the `@`), `description`. |

**Cardinality** is `1`, `0..1`, `1..*`, `*` or `n..m` (digits, e.g. `2..5`), the number of instances at that end that can relate to one instance at the other end. The grammar only recognises the form; it does not check `n <= m`. The description after it is optional.

Tag descriptions continue over following lines until the next `@` tag or the end of the comment. Tags have no `{type}` and no `-` separator.

`/**/` is not a doc comment (the host grammar treats it as an empty plain block, if at all).

## Using it from a host grammar

The host grammar exposes `/** ... */` as a single node (e.g. `doc_comment` in tree-sitter-gram) and injects this grammar with `queries/injections.scm`:

```scm
((doc_comment) @injection.content
  (#set! injection.language "gramdoc"))
```

The injected content must be the whole comment including `/**` and `*/`. Editors without injection support still see an ordinary comment.

## Queries

- `queries/highlights.scm`: name, tag keywords, property names, aliases, reference targets, descriptions.
- `queries/tags.scm`: `@definition.doc` (name), `@definition.property`, `@definition.alias`, `@reference.reference`, each with a `@name` capture.

## Development

```sh
npm install
npx tree-sitter generate
npx tree-sitter test
cargo test
```

`name` is produced by a tiny external scanner (`src/scanner.c`), because "identifier alone on its line" cannot be expressed as a plain token.

Corpus tests live in `test/corpus/` split by concern: `names.txt`, `descriptions.txt`, `tags.txt`.

## License

MIT, see [LICENSE](LICENSE).
