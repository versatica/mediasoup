# Specifications for all the TEST executables.
#
# ====================================================================
# Copyright (c) 2009 Ian Blumel.  All rights reserved.
# 
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.  
# ====================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

MACRO(TO_CPP _NAME_) 
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_NAME_}.cpp
		COMMAND ${CMAKE_COMMAND} -E copy 
                             ${CMAKE_CURRENT_SOURCE_DIR}/${_NAME_}.c 
                             ${CMAKE_CURRENT_BINARY_DIR}/${_NAME_}.cpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_NAME_}.c
		)
	SET_SOURCE_FILES_PROPERTIES(
		${CMAKE_CURRENT_BINARY_DIR}/${_NAME_}.cpp 
		PROPERTIES GENERATED 1
		)
ENDMACRO(TO_CPP)

MACRO(TEST_CPP_VERSION _NAME_)
    TO_CPP(${_NAME_})
    ADD_EXECUTABLE(${_NAME_}_cpp ${CMAKE_CURRENT_BINARY_DIR}/${_NAME_}.cpp)
    ADD_TEST(
        run_${_NAME_}_cpp 
        ${EXECUTABLE_OUTPUT_PATH}/${_NAME_}_cpp
    )
    ADD_TEST(
        run_${_NAME_}_cpp 
        ${EXECUTABLE_OUTPUT_PATH}/${_NAME_}_cpp 
    )
ENDMACRO(TEST_CPP_VERSION)


SET(SIMPLE_TESTS test_1suite
                 test_basic
		 test_call_teardown
                 test_chk_types
		 test_count
                 test_fctkern
                 test_fct_bgn_func
                 test_fct_xchk2
                 test_fct_chk_conditional
                 test_help_logger
                 test_money
		 test_multi_old_style
		 test_quick_test
		 test_two_names
                 test_clp
                 test_conditionals
                 test_chk
                 test_empty
                 test_req_in_setup_teardown
                 test_start_other_than_main
	)	
FOREACH( PROGRAM ${SIMPLE_TESTS}) 
	ADD_EXECUTABLE(${PROGRAM} ${PROGRAM}.c)
ENDFOREACH(PROGRAM)

# This requires more than one file and doesn't fall under the "test simple"
# category.
ADD_EXECUTABLE(
	test_multi 
	test_multi.c test_multi_suite1.c test_multi_with_fixtures_suite2.c
	)

# This one will always fail.
ADD_EXECUTABLE(test_fail test_fail.c)

# TEST_BIG. This requires generating test_big.c before we can
# build test_big.exe. To generate test_big.c you need to compile
# and execute mkbigtest.c. Here is what is happening below,
#
#  1. First we say to build mkbigtest we need to compile mkbigtest.c
#  2. Then we say to build test_big you need to compile test_big.c
#  3. Then we say, well, to generate a test_big.c file you need to
#     run mkbigtest.
#  4. Finally we just make it obvious that to make your test_big you
#     need to make your mkbigtest. I tried to set a dependency on 
#     test_big.c, which seems more natural to me, but CMAKE could not
#     locate a target called "test_big.c".
ADD_EXECUTABLE(mkbigtest mkbigtest.c)
ADD_CUSTOM_COMMAND(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_big.c"
    COMMAND mkbigtest
    DEPENDS mkbigtest 
    )	
ADD_EXECUTABLE(test_big "${CMAKE_CURRENT_BINARY_DIR}/test_big.c")

# -------
# TESTING 
# -------

FOREACH( PROGRAM ${SIMPLE_TESTS}) 
  ADD_TEST(run_${PROGRAM} 
      ${EXECUTABLE_OUTPUT_PATH}/${PROGRAM}
  )
  ADD_TEST(run_${PROGRAM}_with_junit
      ${EXECUTABLE_OUTPUT_PATH}/${PROGRAM} 
      --logger=junit
  )
  ADD_TEST(run_${PROGRAM}_with_minimal
      ${EXECUTABLE_OUTPUT_PATH}/${PROGRAM} 
      --logger=minimal
  )
  ADD_TEST(run_${PROGRAM}_help
      ${EXECUTABLE_OUTPUT_PATH}/${PROGRAM}
      --help
  )
  TEST_CPP_VERSION(${PROGRAM})
ENDFOREACH(PROGRAM)

ADD_TEST(run_test_big ${EXECUTABLE_OUTPUT_PATH}/test_big)
ADD_TEST(run_test_call_teardown 
    ${EXECUTABLE_OUTPUT_PATH}/test_call_teardown
)

TO_CPP(test_multi)
TO_CPP(test_multi_suite1)
TO_CPP(test_multi_with_fixtures_suite2)
ADD_EXECUTABLE(test_multi_cpp 
	${CMAKE_CURRENT_BINARY_DIR}/test_multi.cpp 
	${CMAKE_CURRENT_BINARY_DIR}/test_multi_suite1.cpp
	${CMAKE_CURRENT_BINARY_DIR}/test_multi_with_fixtures_suite2.cpp
	)
ADD_TEST(run_test_multi_cpp
    ${EXECUTABLE_OUTPUT_PATH}/test_multi_cpp
)

# The following tests confirm that failure happens.
ADD_TEST(run_test_fail 
    ${EXECUTABLE_OUTPUT_PATH}/test_fail
)
SET_TESTS_PROPERTIES(run_test_fail PROPERTIES WILL_FAIL 1) 


# Test money uses 'strcpy', and its just an example, so lets add a definition
# for that one program.
SET_TARGET_PROPERTIES(test_money 
    PROPERTIES
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
    )
SET_TARGET_PROPERTIES(test_money_cpp
    PROPERTIES
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
    )


# Tests for the custom command line parse involve generating different
# configurations based on a common template file.
MACRO(TEST_COMMAND_LINE NAME USE_FLAG USE_VALUE) 
    SET(CL_PASSED_IN_VALUE "scooby-doo")
    SET(CL_USE_FLAG  ${USE_FLAG})
    SET(CL_USE_VALUE ${USE_VALUE})
    SET(CL_USE_NAME  ${NAME})
    CONFIGURE_FILE(
        test_cl_customize.c.in 
        ${CMAKE_CURRENT_BINARY_DIR}/test_cl_customize_${NAME}.c
    )
    ADD_EXECUTABLE(test_cl_customize_${NAME}
       	${CMAKE_CURRENT_BINARY_DIR}/test_cl_customize_${NAME}.c
    )
    IF(${USE_FLAG})
        LIST(APPEND ARGS "--flag")
    ENDIF()
    IF(${USE_VALUE})
        LIST(APPEND ARGS "--value" ${CL_PASSED_IN_VALUE})
    ENDIF()
    ADD_TEST(test_cl_customize_${NAME}
        ${EXECUTABLE_OUTPUT_PATH}/test_cl_customize_${NAME}
        ${ARGS}
    )
    SET(CL_PASSED_IN_VALUE )
    SET(CL_USE_FLAG )
    SET(CL_UES_VALUE )
    SET(ARGS )
ENDMACRO()

# Truth table the combinations.
TEST_COMMAND_LINE(noflag_novalue 0 0)
TEST_COMMAND_LINE(noflag_value   0 1)
TEST_COMMAND_LINE(flag_novalue   1 0)
TEST_COMMAND_LINE(flag_value     1 1)

# C++ Only 
add_executable(test_chk_ex test_chk_ex.cxx)
add_test(test_chk_ex ${EXECUTABLE_OUTPUT_PATH}/test_chk_ex)
