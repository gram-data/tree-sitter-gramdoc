; Name
(document (name) @title)

; Tag keywords
"@property" @keyword
"@alias" @keyword
"@label" @keyword
"@elements" @keyword
"@element" @keyword
"@language" @keyword
"@reference" @keyword
(tag_name) @keyword

; Tag arguments
(property_tag name: (identifier) @variable.parameter)
(label_tag name: (identifier) @type)
(term) @string
(attribute_key) @attribute
(attribute_value) @string
(language) @string.special
(alias) @string
(reference_target) @string.special

; Descriptions
(description_line) @comment
