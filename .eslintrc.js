module.exports =
{
	extends : [ 'eslint:recommended' ],
	parserOptions :
	{
		ecmaVersion  : 6,
		sourceType   : 'module',
		ecmaFeatures :
		{
			impliedStrict : true
		}
	},
	env :
	{
		'es6'  : true,
		'node' : true
	},
	'rules' :
	{
		'comma-dangle'                  : 2,
		'comma-spacing'                 : [ 2, { before: false, after: true } ],
		'max-len'                       :
		[
      2,
      {
      	code                   : 100,
      	ignoreStrings          : true,
      	ignoreUrls             : true,
      	ignoreTemplateLiterals : true,
      	tabWidth               : 2,
      	ignoreComments         : true
      }
    ],
		'no-console'                    : 0,
		'no-constant-condition'         : 2,
		'no-control-regex'              : 2,
		'no-dupe-args'                  : 2,
		'no-dupe-keys'                  : 2,
		'no-duplicate-case'             : 2,
		'no-empty'                      : 0,
		'no-empty-character-class'      : 2,
		'no-ex-assign'                  : 2,
		'no-extra-boolean-cast'         : 2,
		'no-extra-parens'               : [ 2, 'all', { nestedBinaryExpressions: false } ],
		'no-extra-semi'                 : 2,
		'no-func-assign'                : 2,
		'no-inner-declarations'         : 2,
		'no-invalid-regexp'             : 2,
		'no-irregular-whitespace'       : 2,
		'no-loop-func'                  : 2,
		'no-negated-in-lhs'             : 2,
		'no-obj-calls'                  : 2,
		'no-prototype-builtins'         : 2,
		'no-regex-spaces'               : 2,
		'no-sparse-arrays'              : 2,
		'no-undef'                      : 2,
		'no-unexpected-multiline'       : 2,
		'no-unreachable'                : 2,
		'no-unsafe-finally'             : 2,
		'no-unused-vars'                : [ 2, { vars: 'all', args: 'after-used' }],
		'no-multi-spaces'               : 0,
		'no-whitespace-before-property' : 2,
		'quotes'                        : [ 2, 'single', { avoidEscape: true } ],
		'semi'                          : [ 2, 'always' ],
		'space-before-blocks'           : 2,
		'space-before-function-paren'   : [ 2, 'never' ],
		'space-in-parens'               : [ 2, 'never' ],
		'spaced-comment'                : [ 2, 'always' ],
		'use-isnan'                     : 2,
		'valid-typeof'                  : 2
	}
};
