# Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

DEF_SYSROOT_SIM := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
DEF_SYSROOT_OS := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
XCODE_BIN_PATH := /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin

DEF_MIN_IOS_VERSION := 8.0

IOS_EXPORT_PREFIX ?= $(realpath $(GLOBAL_ROOT))

ifdef RELEASE
IOS_ROOT := $(TOOLKIT_OUTPUT)/ios/release
else
IOS_ROOT := $(TOOLKIT_OUTPUT)/ios/debug
endif

ios:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static ios-export
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7s MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=i386 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) static


ios-armv7:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static ios-export

ios-armv7s:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7s MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static ios-export

ios-arm64:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) static ios-export

ios-i386:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=i386 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) static ios-export

ios-x86_64:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) static ios-export


ios-clean:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) clean
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7s MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) clean
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) clean
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=i386 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) clean
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) clean

ios-export:
	$(GLOBAL_MKDIR) $(GLOBAL_ROOT)/build/ios
	@echo '// Autogenerated by makefile' > $(GLOBAL_ROOT)/build/ios/export.xcconfig
	@echo '\nSTAPPLER_INCLUDES = $(patsubst $(GLOBAL_ROOT)/%,$(IOS_EXPORT_PREFIX)/%,$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(STAPPLER_INCLUDES)))' >> $(GLOBAL_ROOT)/build/ios/export.xcconfig
	@echo '\nMATERIAL_INCLUDES = $(patsubst $(GLOBAL_ROOT)/%,$(IOS_EXPORT_PREFIX)/%,$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(MATERIAL_INCLUDES)))' >> $(GLOBAL_ROOT)/build/ios/export.xcconfig
	@echo '\nSTAPPLER_LDFLAGS = -lfreetype -lcurl -ljpeg -lpng16 -lhyphen -lstappler -lmaterial' >> $(GLOBAL_ROOT)/build/ios/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_DEBAG = $(IOS_EXPORT_PREFIX)/libs/ios/$$(CURRENT_ARCH)/lib $(IOS_EXPORT_PREFIX)/build/ios/debug/$$(CURRENT_ARCH)' >> $(GLOBAL_ROOT)/build/ios/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_RELEASE = $(IOS_EXPORT_PREFIX)/libs/ios/$$(CURRENT_ARCH)/lib $(IOS_EXPORT_PREFIX)/build/ios/release/$$(CURRENT_ARCH)' >> $(GLOBAL_ROOT)/build/ios/export.xcconfig

.PHONY: ios ios-clean ios-export ios-armv7 ios-armv7s ios-arm64 ios-i386 ios-x86_64
