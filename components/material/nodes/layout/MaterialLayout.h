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

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_

#include "Material.h"
#include "SPStrictNode.h"

NS_MD_BEGIN

class Layout : public stappler::StrictNode {
public:
	using BackButtonCallback = std::function<bool()>;
	using Transition = cocos2d::FiniteTimeAction;

	virtual ~Layout() { }
	virtual bool onBackButton();

	virtual void setBackButtonCallback(const BackButtonCallback &);
	virtual const BackButtonCallback &getBackButtonCallback() const;

	virtual void onPush(ContentLayer *l, bool replace);
	virtual void onPushTransitionEnded(ContentLayer *l, bool replace);

	virtual void onPopTransitionBegan(ContentLayer *l, bool replace);
	virtual void onPop(ContentLayer *l, bool replace);

	virtual void onBackground(ContentLayer *l, Layout *overlay);
	virtual void onBackgroundTransitionEnded(ContentLayer *l, Layout *overlay);

	virtual void onForegroundTransitionBegan(ContentLayer *l, Layout *overlay);
	virtual void onForeground(ContentLayer *l, Layout *overlay);

	virtual Rc<Transition> getDefaultEnterTransition() const;
	virtual Rc<Transition> getDefaultExitTransition() const;

protected:
	bool _inTransition = false;
	BackButtonCallback _backButtonCallback;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_ */
