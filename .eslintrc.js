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
		'no-console'                    : 0,
		'no-undef'                      : 2,
		'no-unused-vars'                : [ 2, { vars: 'all', args: 'after-used' }],
		'no-empty'                      : 0,
		'quotes'                        : [ 2, 'single', { avoidEscape: true } ],
		'semi'                          : [ 2, 'always' ],
		'no-multi-spaces'               : 0,
		'no-whitespace-before-property' : 2,
		'space-before-blocks'           : 2,
		'space-before-function-paren'   : [ 2, 'never' ],
		'space-in-parens'               : [ 2, 'never' ],
		'spaced-comment'                : [ 2, 'always' ],
		'comma-spacing'                 : [ 2, { before: false, after: true } ],
		'no-loop-func'                  : 2
	}
};
