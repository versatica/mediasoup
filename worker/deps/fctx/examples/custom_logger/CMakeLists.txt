# ====================================================================
# Copyright (c) 2010 Ian Blumel.  All rights reserved.
# 
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.  
# ====================================================================

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

ADD_EXECUTABLE(example_custom_logger
    # The 'custom header'.
    custom_logger_fct.h
    # The unit test
    custom_logger.c
)
ADD_TEST(example_custom_logger 
	 ${EXECUTABLE_OUTPUT_PATH}/example_custom_logger
)
