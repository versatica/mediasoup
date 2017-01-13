# ====================================================================
# Copyright (c) 2010 Ian Blumel.  All rights reserved.
# 
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.  
# ====================================================================

INCLUDE_DIRECTORIES(../../include)

ADD_EXECUTABLE(example_custom_check
	example_custom_check.cpp
)
ADD_TEST(example_custom_check 
	 ${EXECUTABLE_OUTPUT_PATH}/example_custom_check
)
