; Descriptions are Markdown. Each line is injected on its own, which gives
; inline highlighting (emphasis, code spans, links); block structure such as
; paragraphs and nested lists needs a consumer that joins the lines.
((description_line) @injection.content
  (#set! injection.language "markdown"))
