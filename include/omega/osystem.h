/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer. Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-----------------------------------------------------------------------------
 * What's in this file
 *	The omegalib entry point (main), initialization and shutdown code, plus a
 *	set of system utility functions.
 ******************************************************************************/
#ifndef __OSYSTEM_H__
#define __OSYSTEM_H__

#include "omegaConfig.h"

#include "otypes.h"
#include "Platform.h"

#ifdef OMEGA_OS_WIN
    // Visual leak detector
    #ifdef OMEGA_ENABLE_VLD
    #include <vld.h>
    #endif
#endif

// Forward declaration of DataSource, used for omain
namespace omega 
{ 
    //! Returns the argument parser object.
    OMEGA_API libconfig::ArgumentHelper& oargs();

    //! Returns a vector of optional arguments passed to omegalib.
    //! @since v6.4
    OMEGA_API const Vector<String>& oxargv();

    //! The omegalib entry point
    OMEGA_API int omain(omega::ApplicationBase& app, int argc, char** argv);

    //! Runs the specified command in a separate process.
    //! @return true if the command launched succesfully, false otherwise.
    OMEGA_API bool olaunch(const String& command);

    //! Returns the current working directory.
    OMEGA_API String ogetcwd();
    //! Returns the path to the currently running executable
    OMEGA_API String ogetexecpath();

    //! @internal - python support only. This is a bit of a hack, adds a prefix
    //! that can be used for data lookups. Right now ImageUtils is the only
    //! class using this. This is really ugly and should be killed with fire.
    //! Don't use this.
    OMEGA_API void osetdataprefix(const String& data);
    OMEGA_API String ogetdataprefix();
};

#endif