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

#ifndef stappler_core_SPConfig_h
#define stappler_core_SPConfig_h

#include "SPForward.h"

/**
 * Enables StoreKit feature (interface for IAP on iOS and Android)
 */
#define SP_CONFIG_STOREKIT 1

NS_SP_EXT_BEGIN(config)

inline constexpr auto getNetworkSpriteAssetTtl() {
	return TimeInterval::seconds(10 * 24 * 60 * 60); // 10 days
}

inline constexpr auto getDocumentAssetTtl() {
	return TimeInterval::seconds(30 * 24 * 60 * 60); // 30 days
}

// max chars count, used by locale::hasLocaleTagsFast
constexpr size_t maxFastLocaleChars = size_t(127);

NS_SP_EXT_END(config)

#endif
