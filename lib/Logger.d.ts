import debug from 'debug';
export declare class Logger {
    private readonly _debug;
    private readonly _warn;
    private readonly _error;
    constructor(prefix?: string);
    get debug(): debug.Debugger;
    get warn(): debug.Debugger;
    get error(): debug.Debugger;
}
//# sourceMappingURL=Logger.d.ts.map