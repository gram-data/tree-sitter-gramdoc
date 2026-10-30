; Name
(document (name) @title)

; Tag keywords
"@property" @keyword
"@alias" @keyword
"@reference" @keyword
(tag_name) @keyword

; Tag arguments
(property_tag name: (identifier) @variable.parameter)
(alias) @string
(reference_target) @string.special

; Descriptions
(description_line) @comment
