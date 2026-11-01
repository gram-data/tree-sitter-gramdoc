; The documented element's name, when the comment starts with one
(document (name) @name) @definition.doc

; Documented properties
(property_tag name: (identifier) @name) @definition.property

; Documented labels
(label_tag name: (identifier) @name) @definition.label

; Aliases
(alias_tag value: (alias) @name) @definition.alias

; External references
(reference_tag target: (reference_target) @name) @reference.reference
