; Identifier naming conventions

((identifier) @constructor
 (#match? @constructor "^[A-Z]"))

((identifier) @constant
 (#match? @constant "^[A-Z][A-Z_]*$"))

(field_identifier) @property
(identifier) @variable

; Builtin functions

((call
  function: (identifier) @function.builtin)
 (#match?
   @function.builtin
   "^(abs|all|any|ascii|bin|bool|breakpoint|bytearray|bytes|callable|chr|classmethod|compile|complex|delattr|dict|dir|divmod|enumerate|eval|exec|filter|float|format|frozenset|getattr|globals|hasattr|hash|help|hex|id|input|int|isinstance|issubclass|iter|len|list|locals|map|max|memoryview|min|next|object|oct|open|ord|pow|print|property|range|repr|reversed|round|set|setattr|slice|sorted|staticmethod|str|sum|super|tuple|type|vars|zip|__import__)$"))

; Function calls

(decorator) @function

(call
  function: (attribute attribute: (field_identifier) @method.call))
(call
  function: (identifier) @function.call)

; Function definitions

(function_definition
  name: (identifier) @function)

(type (identifier) @type)

(visibility) @type.qualifier

; Literals

[
  (none)
  (true)
  (false)
] @constant.builtin

[
  (integer)
  (float)
] @number

[
 (comment)
 (frontmatter)
 (frontmatter_sep)
] @comment

(string) @string
(single_string) @string

[
  "-"
  "-="
  "!="
  "*"
  "**"
  "**="
  "*="
  "/"
  "//"
  "//="
  "/="
  "&"
  "%"
  "%="
  "^"
  "+"
  "+="
  "<"
  "<<"
  "<="
  "<>"
  "="
  ":="
  "=="
  ">"
  ">="
  ">>"
  "|"
  "~"
  "and"
  "in"
  "is"
  "not"
  "or"
] @operator

[
  "as"
  "assert"
  "await"
  "break"
  "class"
  "continue"
  "fn"
  "del"
  "elif"
  "else"
  "catch"
  "exec"
  "finally"
  "from"
  "if"
  "import"
  "pass"
  "raise"
  "return"
  "try"
  "while"
  (let)
  "enum"
  "match"
  "when"
] @keyword
