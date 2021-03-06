// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPDefine.h"
#include "SPFilesystem.h"
#include "platform/CCGL.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>

NS_SP_BEGIN

void PrintBacktrace(int len, Ref *ref) {
    void *bt[len + 2];
    char **bt_syms;
    int bt_size;

    bt_size = backtrace(bt, len + 2);
    bt_syms = backtrace_symbols(bt, bt_size);

    std::stringstream stream;
    if (ref) {
    	stream << "Ref: " << ref << " (refcount: " << ref->getReferenceCount() << ")";
    }
    stream << "\n";

    for (int i = 2; i < bt_size; i++) {
    	auto str = filepath::replace(bt_syms[i], filesystem::currentDir(), "/");
    	auto first = str.find('(');
    	auto second = str.rfind('+');
    	str = str.substr(first + 1, second - first - 1);

    	int status = 0;
    	auto ptr = abi::__cxa_demangle (str.c_str(), nullptr, nullptr, &status);

    	if (ptr) {
        	stream << "\t[" << i - 2 << "] " << std::string(ptr) << "\n";
        	free(ptr);
    	} else {
    		stream << "\t[" << i - 2 << "] " << bt_syms[i] << "\n";
    	}
    }

	log::text("Backtrace", stream.str());

    free(bt_syms);
}

NS_SP_END

#else

NS_SP_BEGIN
void PrintBacktrace(int len, Ref *ref) {

}
NS_SP_END

#endif

NS_SP_BEGIN

#ifndef SP_RESTRICT

void LogOpenGLError(const char *file, size_t line, const char *func) {
#ifdef DEBUG
	static constexpr int VBO_FREE_MEMORY_ATI = 0x87FB;
	static constexpr int TEXTURE_FREE_MEMORY_ATI = 0x87FC;
	static constexpr int RENDERBUFFER_FREE_MEMORY_ATI = 0x87FD;

	static constexpr int GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX = 0x9047;
	static constexpr int GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX = 0x9048;
	static constexpr int GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX = 0x9049;
	static constexpr int GPU_MEMORY_INFO_EVICTED_MEMORY_NVX = 0x904B;

	GLenum __error = glGetError();
	if (__error) {
		const char *desc = "unknown error";
		switch (__error) {
		case GL_INVALID_ENUM: desc = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: desc = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: desc = "GL_INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY: desc = "GL_OUT_OF_MEMORY"; break;
		}
		stappler::log::format("SP-OpenGL", "%s:%d %s: error 0x%04X (%s)", ::stappler::filepath::lastComponent(file).data(), int(line), func, __error, desc);
		if (__error == GL_OUT_OF_MEMORY) {
			int values[4] = { 0, 0, 0, 0};
			glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, values);
			if (values[0] != 0) {
				glGetIntegerv(GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &values[1]);
				glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &values[2]);
				glGetIntegerv(GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &values[3]);
				stappler::log::format("SP-OpenGL", "  - Memory: Total: %dkb, Dedicated: %dkb, Available: %dkb, Evicted: %dkb", values[0], values[1], values[2], values[3]);
			} else {
				glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, values);
				if (values[0] != 0) {
					auto tex = values[0];

					glGetIntegerv(VBO_FREE_MEMORY_ATI, values);
					auto vbo = values[0];

					glGetIntegerv(RENDERBUFFER_FREE_MEMORY_ATI, values);
					auto rbo = values[0];

					stappler::log::format("SP-OpenGL", "  - Memory free: Texture: %d, VBO: %d, RBO: %d", tex, vbo, rbo);
				}
			}
			glGetError();
		}
	}
#endif
}

#else

void LogOpenGLError(const char *file, size_t line, const char *func) { }

#endif

NS_SP_END
