# ====================================================================
# Copyright (c) 2009 Ian Blumel.  All rights reserved.
# 
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.  
# ====================================================================

# Just hold the include file in a project listing. Mainly as a convience for
# IDE generators.
ADD_CUSTOM_TARGET(include
    fct.h
    )
SET_SOURCE_FILES_PROPERTIES(fct.h
    PROPERTIES
    HEADER_FILE_ONLY 1
    )
