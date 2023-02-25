if exists("b:current_syntax")
  finish
endif

syn keyword kabaType void int float bool string char vec2 vec3 mat3 mat4 complex color quaternion rect any Exception File base math hui nix os kaba time net
syn keyword kabaKeyword class struct func if then else for while in return break continue enum lambda use import const extern static virtual override selfref ref let dyn sort call raise try except and or not len str new del out throws pure extends shared owned xfer as is var _filter weak give sizeof typeof
syn keyword kabaConstant true false nil pi
syn keyword kabaFunction print as_binary p2s abs sum sin cos tan exp log min max argmin argmax rand range sqr sqrt pow acos asin atan atan2 clamp loop
"syn keyword kabaOperator + - = * / += -= *= /= < > << >> != == % ^

syn match kabaComment /#.*/
syn region kabaStringInterpolation start=/{{/ end=/}}/ contained
syn region kabaString start=/"/ skip=/\\"/ end=/"/ contains=kabaStringInterpolation
"syn match kabaNumber /[0-9]*/

highlight link kabaType Type
highlight link kabaKeyword Statement
highlight link kabaComment Comment
"highlight link kabaOperator Statement
"Operator
highlight link kabaConstant Constant
highlight link kabaFunction Statement
highlight link kabaNumber Number
highlight link kabaString String
highlight link kabaStringInterpolation SpecialChar
