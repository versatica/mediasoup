import { Boolean } from '../../f-b-s/rtp-parameters/boolean';
import { Double } from '../../f-b-s/rtp-parameters/double';
import { Integer } from '../../f-b-s/rtp-parameters/integer';
import { IntegerArray } from '../../f-b-s/rtp-parameters/integer-array';
import { String } from '../../f-b-s/rtp-parameters/string';
export declare enum Value {
    NONE = 0,
    Boolean = 1,
    Integer = 2,
    Double = 3,
    String = 4,
    IntegerArray = 5
}
export declare function unionToValue(type: Value, accessor: (obj: Boolean | Double | Integer | IntegerArray | String) => Boolean | Double | Integer | IntegerArray | String | null): Boolean | Double | Integer | IntegerArray | String | null;
export declare function unionListToValue(type: Value, accessor: (index: number, obj: Boolean | Double | Integer | IntegerArray | String) => Boolean | Double | Integer | IntegerArray | String | null, index: number): Boolean | Double | Integer | IntegerArray | String | null;
//# sourceMappingURL=value.d.ts.map