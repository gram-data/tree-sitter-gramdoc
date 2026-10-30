/// <reference types="node" />

const assert = require("node:assert");
const { test } = require("node:test");

const Parser = require("tree-sitter");
const Gramdoc = require(".");

test("can load grammar", () => {
  const parser = new Parser();
  assert.doesNotThrow(() => parser.setLanguage(Gramdoc));
});

test("parses a doc comment", () => {
  const parser = new Parser();
  parser.setLanguage(Gramdoc);

  const tree = parser.parse(
    "/**\n * A human individual.\n *\n * @property name The full name.\n * @target + At least one.\n */",
  );
  assert(!tree.rootNode.hasError, "should parse without errors");
  const types = tree.rootNode.namedChildren.map((n) => n.type);
  assert.deepStrictEqual(types, ["description", "property_tag", "target_tag"]);
});
