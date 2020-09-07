const eslintConfig =
{
	env :
	{
		es6  : true,
		node : true
	},
	plugins       : [],
	settings      : {},
	parserOptions :
	{
		ecmaVersion  : 2018,
		sourceType   : 'module',
		ecmaFeatures :
		{
			impliedStrict : true
		},
		lib : [ 'es2018' ]
	},
	globals :
	{
		NodeJS : 'readonly'
	},
	rules :
	{
		'array-bracket-spacing' : [ 2, 'always',
			{
				objectsInArrays : true,
				arraysInArrays  : true
			}
		],
		'arrow-parens'              : [ 2, 'always' ],
		'arrow-spacing'             : 2,
		'block-spacing'             : [ 2, 'always' ],
		'brace-style'               : [ 2, 'allman', { allowSingleLine: true } ],
		'camelcase'                 : 2,
		'comma-dangle'              : 2,
		'comma-spacing'             : [ 2, { before: false, after: true } ],
		'comma-style'               : 2,
		'computed-property-spacing' : 2,
		'constructor-super'         : 2,
		'func-call-spacing'         : 2,
		'generator-star-spacing'    : 2,
		'guard-for-in'              : 2,
		'indent'                    : [ 2, 'tab', { 'SwitchCase': 1 } ],
		'key-spacing'               : [ 2,
			{
				singleLine :
				{
					beforeColon : false,
					afterColon  : true
				},
				multiLine :
				{
					beforeColon : true,
					afterColon  : true,
					align       : 'colon'
				}
			}
		],
		'keyword-spacing'      : 2,
		'linebreak-style'      : [ 2, 'unix' ],
		'lines-around-comment' : [ 2,
			{
				allowBlockStart    : true,
				allowObjectStart   : true,
				beforeBlockComment : false,
				beforeLineComment  : false
			}
		],
		'max-len' : [ 2, 90,
			{
				tabWidth               : 2,
				comments               : 90,
				ignoreUrls             : true,
				ignoreStrings          : true,
				ignoreTemplateLiterals : true,
				ignoreRegExpLiterals   : true
			}
		],
		'newline-after-var'             : 2,
		'newline-before-return'         : 2,
		'newline-per-chained-call'      : 2,
		'no-alert'                      : 2,
		'no-caller'                     : 2,
		'no-case-declarations'          : 2,
		'no-catch-shadow'               : 2,
		'no-class-assign'               : 2,
		'no-confusing-arrow'            : 2,
		'no-console'                    : 2,
		'no-const-assign'               : 2,
		'no-debugger'                   : 2,
		'no-dupe-args'                  : 2,
		'no-dupe-keys'                  : 2,
		'no-duplicate-case'             : 2,
		'no-div-regex'                  : 2,
		'no-empty'                      : [ 2, { allowEmptyCatch: true } ],
		'no-empty-pattern'              : 2,
		'no-else-return'                : 0,
		'no-eval'                       : 2,
		'no-extend-native'              : 2,
		'no-ex-assign'                  : 2,
		'no-extra-bind'                 : 2,
		'no-extra-boolean-cast'         : 2,
		'no-extra-label'                : 2,
		'no-extra-semi'                 : 2,
		'no-fallthrough'                : 2,
		'no-func-assign'                : 2,
		'no-global-assign'              : 2,
		'no-implicit-coercion'          : 2,
		'no-implicit-globals'           : 2,
		'no-inner-declarations'         : 2,
		'no-invalid-regexp'             : 2,
		'no-invalid-this'               : 2,
		'no-irregular-whitespace'       : 2,
		'no-lonely-if'                  : 2,
		'no-mixed-operators'            : 2,
		'no-mixed-spaces-and-tabs'      : 2,
		'no-multi-spaces'               : 2,
		'no-multi-str'                  : 2,
		'no-multiple-empty-lines'       : [ 1, { max: 1, maxEOF: 0, maxBOF: 0 } ],
		'no-native-reassign'            : 2,
		'no-negated-in-lhs'             : 2,
		'no-new'                        : 2,
		'no-new-func'                   : 2,
		'no-new-wrappers'               : 2,
		'no-obj-calls'                  : 2,
		'no-proto'                      : 2,
		'no-prototype-builtins'         : 0,
		'no-redeclare'                  : 2,
		'no-regex-spaces'               : 2,
		'no-restricted-imports'         : 2,
		'no-return-assign'              : 2,
		'no-self-assign'                : 2,
		'no-self-compare'               : 2,
		'no-sequences'                  : 2,
		'no-shadow'                     : 2,
		'no-shadow-restricted-names'    : 2,
		'no-spaced-func'                : 2,
		'no-sparse-arrays'              : 2,
		'no-this-before-super'          : 2,
		'no-throw-literal'              : 2,
		'no-undef'                      : 2,
		'no-unexpected-multiline'       : 2,
		'no-unmodified-loop-condition'  : 2,
		'no-unreachable'                : 2,
		'no-unused-vars'                : [ 1, { vars: 'all', args: 'after-used' } ],
		'no-use-before-define'          : 0,
		'no-useless-call'               : 2,
		'no-useless-computed-key'       : 2,
		'no-useless-concat'             : 2,
		'no-useless-rename'             : 2,
		'no-var'                        : 2,
		'no-whitespace-before-property' : 2,
		'object-curly-newline'          : 0,
		'object-curly-spacing'          : [ 2, 'always' ],
		'object-property-newline'       : [ 2, { allowMultiplePropertiesPerLine: true } ],
		'prefer-const'                  : 2,
		'prefer-rest-params'            : 2,
		'prefer-spread'                 : 2,
		'prefer-template'               : 2,
		'quotes'                        : [ 2, 'single', { avoidEscape: true } ],
		'semi'                          : [ 2, 'always' ],
		'semi-spacing'                  : 2,
		'space-before-blocks'           : 2,
		'space-before-function-paren'   : [ 2,
			{
				anonymous  : 'never',
				named      : 'never',
				asyncArrow : 'always'
			}
		],
		'space-in-parens' : [ 2, 'never' ],
		'spaced-comment'  : [ 2, 'always' ],
		'strict'          : 2,
		'valid-typeof'    : 2,
		'yoda'            : 2
	}
};

switch (process.env.MEDIASOUP_NODE_LANGUAGE)
{
	case 'typescript':
	{
		eslintConfig.parser = '@typescript-eslint/parser';
		eslintConfig.plugins =
		[
			...eslintConfig.plugins,
			'@typescript-eslint'
		];
		eslintConfig.extends =
		[
			'eslint:recommended',
			'plugin:@typescript-eslint/eslint-recommended',
			'plugin:@typescript-eslint/recommended'
		];
		eslintConfig.rules =
		{
			...eslintConfig.rules,
			'no-unused-vars'                                    : 0,
			'@typescript-eslint/ban-types'                      : 0,
			'@typescript-eslint/ban-ts-comment'                 : 0,
			'@typescript-eslint/ban-ts-ignore'                  : 0,
			'@typescript-eslint/explicit-module-boundary-types' : 0,
			'@typescript-eslint/member-delimiter-style'         : [ 2,
				{
					multiline  : { delimiter: 'semi', requireLast: true },
					singleline : { delimiter: 'semi', requireLast: false }
				}
			],
			'@typescript-eslint/no-explicit-any' : 0,
			'@typescript-eslint/no-unused-vars'  : [ 2,
				{
					vars               : 'all',
					args               : 'after-used',
					ignoreRestSiblings : false
				}
			],
			'@typescript-eslint/no-use-before-define'  : [ 2, { functions: false } ],
			'@typescript-eslint/no-empty-function'     : 0,
			'@typescript-eslint/no-non-null-assertion' : 0
		};

		break;
	}

	case 'javascript':
	{
		eslintConfig.env['jest/globals'] = true;
		eslintConfig.plugins =
		[
			...eslintConfig.plugins,
			'jest'
		];

		break;
	}

	default:
	{
		throw new TypeError('wrong/missing MEDIASOUP_NODE_LANGUAGE env');
	}
}

module.exports = eslintConfig;
