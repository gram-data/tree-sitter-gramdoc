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
 * @source + Each :KNOWS may start at one or more people.
 * @target ? Each :KNOWS ends at at most one person.
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
| `@self` | `self_tag` | Bare marker: relationships of this kind must be self-loops. No argument, no description. |
| `@<other> [description]` | `unknown_tag` | Any other tag. Fields: `name` (`tag_name`, includes the `@`), `description`. |

**Cardinality** is one of:

| Form | Meaning |
|------|---------|
| `n` | exactly n (any integer: `1`, `3`, `10`) |
| `n..m` | between n and m, inclusive |
| `n..` | at least n |
| `..m` | at most m (minimum 0) |
| `?` | zero or one (same as `..1`) |
| `+` | one or more (same as `1..`) |
| `*` | zero or more (unconstrained) |

Digits are unsigned integers, and there are no spaces around `..` (`2..5`, not `2 .. 5`). The grammar only recognises the form: `5..2` still parses, and checking `n <= m` is left to lint. The description after the cardinality is optional.

**Semantics.** The cardinality at an end is the number of relationships of this kind *per node at that end*, not the number of nodes at the other end. This matches the end `count` / `min_count` / `max_count` of the [Neo4j ontology-graph-spec](https://github.com/neo4j/ontology-graph-spec) relationship (`@source` is `from`, `@target` is `to`), and is the **reverse of UML multiplicity**. For `[:ACTED_IN | person, movie]`:

```gram
/**
 * ACTED_IN
 *
 * @source * A person may act in any number of movies.
 * @target + Every movie has at least one incoming ACTED_IN.
 */
```

`@target +` is `to: {node: movie, min_count: 1}` in the ontology spec.

The first token after `@source` or `@target` is always the cardinality; the description is whatever follows it, so a description may start with punctuation (`@source 1 (the author) ...`). Matching is greedy and needs no space after the cardinality, so `1st` is `1` plus description `st`, and `1..5th` is `1..5` plus `th`. To start a description with a dot or digits, put a space after the cardinality (`@source 1 ...and more`, `@source + 2 people`).

Tag descriptions continue over following lines until the next `@` tag or the end of the comment. Tags have no `{type}` and no `-` separator.

**Self-loops.** `@self` says relationships of this kind MUST be self-loops: source and target are the same node. Its absence says nothing; loops are neither required nor forbidden. A loop counts at both ends for its one node, so cardinalities on `@source` and `@target` still apply to it (a node with one loop has one relationship at its source end and one at its target end). `@self` is a bare marker: text after it on the same line is a parse error rather than being ignored, and `@selfish` is an ordinary unknown tag. It may appear anywhere in the tag list, in any order, and repeating it is allowed (redundant, not an error). Rules that loops are forbidden, and symmetry, are not expressible.

```gram
/**
 * SAME_AS
 *
 * @label SAME_AS
 * @self
 * @source ? A person has at most one SAME_AS loop.
 * @target ..1 Likewise at the other end.
 */
[:SAME_AS | (p:Person), (p)]
```

### More examples

Cardinality always counts relationships of this kind per node at that end, so the same
numbers can read backwards at first. More cases live in `test/corpus/relationships.txt`.

```gram
// Each child has at most two recorded parents: at most two incoming
// PARENT_OF relationships end at any one person, hence `..2` on the target.
/**
 * PARENT_OF
 *
 * @label PARENT_OF
 * @alias HAS_CHILD
 * @source * A person may be recorded as parent of any number of children.
 * @target ..2 A child has at most two recorded parents, so at most two
 *   incoming PARENT_OF relationships end at any one person.
 */
[:PARENT_OF | (parent:Person), (child:Person)]

// `..1` on the source: each person has at most one outgoing REPORTS_TO.
/**
 * REPORTS_TO
 *
 * @label REPORTS_TO
 * @property since When the reporting line started.
 * @source ..1 Each person has at most one manager.
 * @target * A manager may have any number of direct reports.
 */
[:REPORTS_TO {since: date} | (report:Person), (manager:Person)]

/**
 * Un lien de parenté.
 *
 * @label PARENT_OF Un parent d'une personne.
 * @source * Une personne peut être le parent d'un nombre quelconque d'enfants.
 * @target ..2 Un enfant a au plus deux parents enregistrés.
 */
[:French | [:PARENT_OF | (parent:Person), (child:Person)]]
```

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
