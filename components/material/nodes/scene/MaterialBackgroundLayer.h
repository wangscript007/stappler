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

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_

#include "Material.h"
#include "2d/CCNode.h"

NS_MD_BEGIN

class BackgroundLayer : public cocos2d::Node {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setTexture(cocos2d::Texture2D *);

	virtual void setColor(const Color &);

protected:
	Layer *_layer = nullptr;
	DynamicSprite *_sprite = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALBACKGROUNDLAYER_H_ */
