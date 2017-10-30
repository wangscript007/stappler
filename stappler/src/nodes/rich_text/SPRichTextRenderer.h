/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_

#include "SPRichText.h"
#include "2d/CCComponent.h"
#include "SPRichTextDrawer.h"

NS_SP_EXT_BEGIN(rich_text)

class Renderer : public cocos2d::Component {
public:
	using RenderingCallback = Function<void(Result *, bool started)>;

	static bool shouldSplitPages(const Size &);

	virtual ~Renderer();

	virtual bool init(const Vector<String> &ids = {});
	virtual void onContentSizeDirty() override;
	virtual void onVisit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, const ZPath &zPath) override;

	virtual void setSource(Source *source);
	virtual Source *getSource() const;

	virtual Document *getDocument() const;
	virtual layout::Result *getResult() const;
	virtual Drawer *getDrawer() const;

public: /* media type resolver */
	void setSurfaceSize(const Size &size);

	// size of rendering surface (size for media-queries)
	const Size &getSurfaceSize() const;
	const Margin & getPageMargin() const;
	bool isPageSplitted() const;

	void setDpi(int dpi);
	void setDensity(float density);

	void setMediaType(layout::style::MediaType value);
	void setOrientationValue(layout::style::Orientation value);
	void setPointerValue(layout::style::Pointer value);
	void setHoverValue(layout::style::Hover value);
	void setLightLevelValue(layout::style::LightLevel value);
	void setScriptingValue(layout::style::Scripting value);
	void setHyphens(layout::HyphenMap *);

	void setPageMargin(const Margin &);

	void addOption(const String &);
	void removeOption(const String &);
	bool hasOption(const String &) const;

	void addFlag(layout::RenderFlag::Flag flag);
	void removeFlag(layout::RenderFlag::Flag flag);
	bool hasFlag(layout::RenderFlag::Flag flag) const;

	String getLegacyBackground(const layout::Node &, const StringView &opt) const;

public: /* font routines */
	virtual void setRenderingCallback(const RenderingCallback &);
	virtual void onResult(Result *result);

protected:
	virtual bool requestRendering();
	virtual void onSource();
	virtual void pushVersionOptions();

	bool _renderingDirty = false;
	bool _renderingInProgress = false;
	bool _isPageSplitted = false;

	Margin _pageMargin;
	data::Listener<Source> _source;
	Vector<String> _ids;
	Size _surfaceSize;
	MediaParameters _media;
	Rc<layout::Result> _result;
	Rc<Drawer> _drawer;
	Rc<layout::HyphenMap> _hyphens;
	RenderingCallback _renderingCallback = nullptr;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERER_H_ */
