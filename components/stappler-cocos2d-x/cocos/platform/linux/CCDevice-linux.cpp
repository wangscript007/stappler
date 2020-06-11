/****************************************************************************
Copyright (c) 2011      Laschweinski
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

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
****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_LINUX

#include "platform/CCDevice.h"
#include <X11/Xlib.h>
#include <stdio.h>

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "platform/CCFileUtils.h"

#include "ft2build.h"
#include FT_FREETYPE_H

// as FcFontMatch is quite an expensive call, cache the results of getFontFile
static std::map<std::string, std::string> fontCache;

struct LineBreakGlyph {
    FT_UInt glyphIndex;
    int paintPosition;
    int glyphWidth;

    int bearingX;
    int kerning;
    int horizAdvance;
};

struct LineBreakLine {
    LineBreakLine() : lineWidth(0) {}

    std::vector<LineBreakGlyph> glyphs;
    int lineWidth;

    void reset() {
        glyphs.clear();
        lineWidth = 0;
    }

    void calculateWidth() {
        lineWidth = 0;
        if ( glyphs.empty() == false ) {
            lineWidth = glyphs.at(glyphs.size() - 1).paintPosition + glyphs.at(glyphs.size() - 1).glyphWidth;
        }
    }
};

NS_CC_BEGIN

int Device::getDPI()
{
    static int dpi = -1;
    if (dpi == -1)
    {
        Display *dpy;
        char *displayname = NULL;
        int scr = 0; /* Screen number */
        dpy = XOpenDisplay (displayname);
        /*
         * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
         *
         *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
         *         = N pixels / (M inch / 25.4)
         *         = N * 25.4 pixels / M inch
         */
        double xres = ((((double) DisplayWidth(dpy,scr)) * 25.4) /
            ((double) DisplayWidthMM(dpy,scr)));
        dpi = (int) (xres + 0.5);
        //printf("dpi = %d\n", dpi);
        XCloseDisplay (dpy);
    }
    return dpi;
}

void Device::setAccelerometerEnabled(bool isEnabled)
{

}

void Device::setAccelerometerInterval(float interval)
{

}

void Device::setKeepScreenOn(bool value)
{
}

NS_CC_END

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_LINUX