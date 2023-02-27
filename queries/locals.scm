;;; Program structure
(module) @scope

(class_definition
  body: (block
          (expression_statement
            (assignment
              left: (identifier) @definition.field)))) @scope
(class_definition
  body: (block
          (expression_statement
            (assignment
              left: (_ 
                     (identifier) @definition.field))))) @scope

; Imports
(import_statement
  name: (dotted_name ((identifier) @definition.import)))

; Function with parameters, defines parameters
(parameters
  (identifier) @definition.parameter)

; Function defines function and scope
((function_definition
  name: (identifier) @definition.function) @scope
 (#set! definition.function.scope "parent"))


((class_definition
  name: (identifier) @definition.type) @scope
 (#set! definition.type.scope "parent"))

(class_definition
  body: (block
          (function_definition
            name: (identifier) @definition.method)))

;;; Assignments

(assignment
 left: (identifier) @definition.var)

(assignment
 left: (attribute
   (identifier)
   (field_identifier) @definition.field))

;;; REFERENCES
(identifier) @reference
