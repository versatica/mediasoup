const path = require('path');

module.exports = {
	mode   : 'development',
	entry  : './src/index.ts',
	module : {
		rules : [
			{
				test    : /\.ts$/,
				use     : 'ts-loader',
				exclude : /node_modules/
			}
		]
	},
	resolve : {
		extensions : [ '.ts', '.js' ]
	},
	output : {
		filename : 'dist/bundle.js',
		path     : path.resolve(__dirname, 'dist')
	},
	devServer : {
		liveReload : false,
		port       : 3001,
		static     : {
			directory: __dirname
		}
	}
};
