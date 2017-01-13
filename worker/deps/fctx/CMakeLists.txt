# Specifications for all the TEST executables.
#
# ====================================================================
# Copyright (c) 2009 Ian Blumel.  All rights reserved.
# 
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.  
# ====================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(FCT)
ENABLE_TESTING()

# ------------------------
# Configure Warning Levels
# ------------------------

IF (MSVC)
    # Warning Level 4, Treat all warnings as errors!
    IF (MSVC80)
        # 64 bit warning flags are only really used in MSVC80.
	SET(WARN_LEV 4)
	SET(C_WARNINGS "/WX /Wp64")
	SET(CXX_WARNINGS "/WX /Wp64")
    ELSEIF(MSVC60)
        # Treating warnings as errors for this compiler makes things unworkable
        # as you get warnings about functions being inlined (who cares),
        # warnings about inlined functions in the math library not being used
        # (who cares), and so on. For my sanity this old compiler will get the
        # warnings turned on, but we can manually choose to ignore them.        
        #
        # Not only am I not turning on warnings, I am dialing them down to a
        # "sane" level 3.
	SET(WARN_LEV 3)
	SET(C_WARNINGS )
	SET(CXX_WARNINGS  )
    ELSE()
	SET(WARN_LEV 4)    
	SET(C_WARNINGS "/WX")
	SET(CXX_WARNINGS "/WX")
    ENDIF()
    STRING(REPLACE "/W3" "/W${WARN_LEV}" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    STRING(REPLACE "/W3" "/W${WARN_LEV}" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
ELSEIF (CMAKE_COMPILER_IS_GNUCC)
    set(GCC_COMMON_WARNING_FLAGS 
        "-Wall -Wextra -O -Werror -Wformat=2 -Wshadow -Wfloat-equal"
        )
    SET(C_WARNINGS "${GCC_COMMON_WARNING_FLAGS}")
    set(CXX_WARNINGS "${GCC_COMMON_WARNING_FLAGS}")
ENDIF()

SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${C_WARNINGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_WARNINGS}")

SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/bin)

# --------
# Delegate
# --------

ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(examples)

# ------ 
# ASTYLE
# ------
# Target for standardizing the style in FCTX, if astyle.exe is
# available.

find_program(ASTYLE astyle)
if(ASTYLE)
    add_custom_target(run_astyle
        "${CMAKE_COMMAND}" 
                -E chdir "${CMAKE_CURRENT_SOURCE_DIR}"
                ${ASTYLE}
                --options=${CMAKE_CURRENT_SOURCE_DIR}/astyle.conf 
                include/*.h 
                tests/*.c
                examples/custom_logger/*.h
                examples/custom_logger/*.c
    )
endif()
