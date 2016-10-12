/*
 * SPLayeredBatchNode.cpp
 *
 *  Created on: 26 окт. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPLayeredBatchNode.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

LayeredBatchNode::~LayeredBatchNode() { }

void LayeredBatchNode::setTextures(const Vector<Rc<cocos2d::Texture2D>> &tex) {
	_textures.clear();
	for (auto &it : tex) {
		updateBlendFunc(it);
		_textures.emplace_back(TextureLayer{it, nullptr, Rc<DynamicQuadArray>::alloc()});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<DynamicBatchCommand>::create());
		}
	}
}
void LayeredBatchNode::setTextures(const Vector<cocos2d::Texture2D *> &tex) {
	_textures.clear();
	for (auto &it : tex) {
		updateBlendFunc(it);
		_textures.emplace_back(TextureLayer{it, nullptr, Rc<DynamicQuadArray>::alloc()});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<DynamicBatchCommand>::create());
		}
	}
}

void LayeredBatchNode::setTextures(const Vector<Rc<cocos2d::Texture2D>> &tex, Vector<Rc<DynamicQuadArray>> &&newQuads) {
	_textures.clear();
	for (size_t i = 0; i < tex.size(); ++i) {
		updateBlendFunc(tex[i]);
		_textures.emplace_back(TextureLayer{tex[i], nullptr, std::move(newQuads[i])});
	}

	if (_commands.size() < _textures.size()) {
		auto size = _commands.size();
		_commands.reserve(_textures.size());
		for (size_t i = size; i < _textures.size(); ++ i) {
			_commands.push_back(Rc<DynamicBatchCommand>::create());
		}
	}
}

void LayeredBatchNode::draw(cocos2d::Renderer *renderer, const Mat4 &transform, uint32_t flags, const ZPath &zPath) {
	size_t i = 0;
	for (auto &it : _textures) {
		if (!it.atlas && it.texture) {
			if (auto atlas = construct<DynamicAtlas>(it.texture)) {
				it.atlas = atlas;
				it.atlas->addQuadArray(it.quads);
			}
		}

		if (!it.atlas || !it.texture) {
			continue;
		}

		auto cmd = _commands[i];
		if (_normalized) {
			Mat4 newMV;
			newMV.m[12] = floorf(transform.m[12]);
			newMV.m[13] = floorf(transform.m[13]);
			newMV.m[14] = floorf(transform.m[14]);

			cmd->init(_globalZOrder, getGLProgram(), _blendFunc, it.atlas, newMV, zPath, _normalized);
		} else {
			cmd->init(_globalZOrder, getGLProgram(), _blendFunc, it.atlas, transform, zPath, _normalized);
		}

		renderer->addCommand(cmd);
		++ i;
	}
}

void LayeredBatchNode::updateColor() {
	if (!_textures.empty()) {
	    Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

	    for (auto &it : _textures) {
	    	if (!it.quads->empty()) {
	    	    // special opacity for premultiplied textures
	    		if (_opacityModifyRGB) {
	    			color4.r *= _displayedOpacity/255.0f;
	    			color4.g *= _displayedOpacity/255.0f;
	    			color4.b *= _displayedOpacity/255.0f;
	    	    }

	    		for (size_t i = 0; i < it.quads->size(); i++) {
	    			it.quads->setColor(i, color4);
	    		}
	    	}
	    }
	}
}

DynamicQuadArray *LayeredBatchNode::getQuads(cocos2d::Texture2D *tex) {
    for (auto &it : _textures) {
    	if (it.texture == tex) {
    		return it.quads;
    	}
    }
    return nullptr;
}

NS_SP_END
