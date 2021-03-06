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

#include "Material.h"
#include "MaterialIconSprite.h"
#include "MaterialMenuButton.h"

#include "MaterialResourceManager.h"
#include "MaterialMenu.h"
#include "MaterialLabel.h"
#include "MaterialScene.h"
#include "MaterialFloatingMenu.h"

#include "SPRoundedSprite.h"
#include "SPDataListener.h"

NS_MD_BEGIN

bool MenuButton::init() {
	if (!Button::init()) {
		return false;
	}

	setStyle(Button::Style::FlatBlack);
	setTapCallback(std::bind(&MenuButton::onButton, this));
	setSpawnDelay(0.1f);
	setSwallowTouches(false);

	auto menuNameLabel = Rc<Label>::create(FontType::Subhead);
	menuNameLabel->setVisible(false);
	menuNameLabel->setAnchorPoint(Vec2(0, 0.5f));
	menuNameLabel->setLocaleEnabled(true);\
	_menuNameLabel = addChildNode(menuNameLabel);

	auto menuValueLabel = Rc<Label>::create(FontType::Subhead);
	menuValueLabel->setVisible(false);
	menuValueLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
	menuValueLabel->setLocaleEnabled(true);
	_menuValueLabel = addChildNode(menuValueLabel);

	auto menuNameIcon = Rc<IconSprite>::create();
	menuNameIcon->setVisible(false);
	menuNameIcon->setAnchorPoint(Vec2(0, 0.5));
	_menuNameIcon = addChildNode(menuNameIcon);

	auto menuValueIcon = Rc<IconSprite>::create();
	menuValueIcon->setVisible(false);
	menuValueIcon->setAnchorPoint(Vec2(1, 0.5));
	_menuValueIcon = addChildNode(menuValueIcon);

	onLightLevel();

	return true;
}
void MenuButton::onContentSizeDirty() {
	Button::onContentSizeDirty();
	layoutSubviews();
}

void MenuButton::setMenuSourceItem(MenuSourceItem *iitem) {
	auto item = static_cast<MenuSourceButton *>(iitem);
	setMenuSourceButton(item);
}

void MenuButton::setMenu(Menu *m) {
	if (_menu != m) {
		_menu = m;

		auto menuMetrics = _menu->getMetrics();
		auto font = (menuMetrics == MenuMetrics::Navigation)?FontType::Body_1:FontType::Subhead;

		_menuNameLabel->setFont(font);
		_menuValueLabel->setFont(font);
	}
}

Menu *MenuButton::getMenu() {
	return _menu;
}

void MenuButton::layoutSubviews() {
	auto height = 48.0f;

	auto menu = getMenu();
	if (!menu) {
		return;
	}
	auto menuMetrics = menu->getMetrics();

	_menuNameLabel->setVisible(false);
	_menuValueLabel->setVisible(false);
	_menuNameIcon->setVisible(false);
	_menuValueIcon->setVisible(false);

	if (_source) {
		bool enabled = (_source->getCallback() != nullptr || _source->getNextMenu() != nullptr);
		float namePos = metrics::menuFirstLeftKeyline(menuMetrics);
		auto nameIcon = _source->getNameIcon();
		if (nameIcon != IconName::None) {
			_menuNameIcon->setIconName(nameIcon);
			_menuNameIcon->setPosition(Vec2(namePos, height / 2));
			_menuNameIcon->setVisible(true);
			_menuNameIcon->setOpacity((enabled) ? 222 : 138);

			namePos = metrics::menuSecondLeftKeyline(menuMetrics);
		} else {
			_menuNameIcon->setVisible(false);
		}

		auto name = _source->getName();
		if (!name.empty()) {
			_menuNameLabel->setString(name);
			_menuNameLabel->setPosition(Vec2(namePos, height / 2));
			_menuNameLabel->setVisible(true);
			_menuNameLabel->setOpacity((enabled) ? 222 : 138);
		} else {
			_menuNameLabel->setVisible(false);
		}

		if (_source->getNextMenu()) {
			_menuValueIcon->setIconName(IconName::Navigation_arrow_drop_down);
			_menuValueIcon->setPosition(Vec2(_contentSize.width - 8, height / 2));
			_menuValueIcon->setVisible(true);
			_menuValueIcon->setRotated(true);
		} else {
			_menuValueIcon->setVisible(false);
			_menuValueIcon->setRotated(false);
		}

		if (!_source->getValue().empty() && !_menuValueIcon->isVisible()) {
			_menuValueLabel->setVisible(true);
			_menuValueLabel->setString(_source->getValue());
			_menuValueLabel->setPosition(Vec2(_contentSize.width - 16, height / 2));
			_menuValueLabel->setOpacity((enabled) ? 222 : 138);
		} else {
			_menuValueLabel->setVisible(false);
		}

		if (_source->getValueIcon() != IconName::Empty && _source->getValueIcon() != IconName::None
			&& !_menuValueLabel->isVisible() && !_menuValueIcon->isVisible()) {

			_menuValueIcon->setIconName(_source->getValueIcon());
			_menuValueIcon->setPosition(Vec2(_contentSize.width - 16, height / 2));
			_menuValueIcon->setVisible(true);
			_menuValueIcon->setOpacity((enabled) ? 222 : 138);
		}

		if (_source->isSelected()) {
			setSelected(true, true);
		} else {
			setSelected(false);
		}

		setEnabled(enabled && _menu->isEnabled());
	}

	if (_menuNameLabel->isVisible()) {
		float width = _contentSize.width - _menuNameLabel->getPositionX() - 16.0f;
		if (_menuValueLabel->isVisible()) {
			_menuValueLabel->tryUpdateLabel();
		}

		if (_menuValueLabel->isVisible() && _menuValueLabel->getContentSize().width > 0.0f) {
			width = _menuValueLabel->getPositionX() - _menuNameLabel->getPositionX() - _menuValueLabel->getContentSize().width - 8.0f;
		} else if (_menuValueIcon->isVisible()) {
			width = _menuValueIcon->getPositionX() - _menuNameLabel->getPositionX() - _menuValueIcon->getContentSize().width - 8.0f;
		}

		if (width > 0.0f) {
			if (_wrapName) {
				_menuNameLabel->setWidth(width);
			} else {
				_menuNameLabel->setMaxWidth(width);
			}
		}
	}
}

void MenuButton::onButton() {
	if (_source) {
		Rc<Menu> menu = getMenu();

		if (auto cb = _source->getCallback()) {
			cb(this, _source);
		}
		if (auto nextMenu = _source->getNextMenu()) {
			auto scene = Scene::getRunningScene();
			auto size = scene->getContentSize();

			auto posLeft = convertToWorldSpace(Vec2(0, _contentSize.height));
			auto posRight = convertToWorldSpace(Vec2(_contentSize.width, _contentSize.height));

			float pointLeft = posLeft.x;
			float pointRight = size.width - posRight.x;

			if (pointRight >= pointLeft) {
				FloatingMenu::push(nextMenu, posRight, FloatingMenu::Binding::OriginRight,
						dynamic_cast<FloatingMenu *>(_menu));
			} else {
				FloatingMenu::push(nextMenu, posLeft, FloatingMenu::Binding::OriginLeft,
						dynamic_cast<FloatingMenu *>(_menu));
			}
		}
		if (menu) {
			menu->onMenuButtonPressed(this);
		}
	}
}

void MenuButton::updateFromSource() {
	layoutSubviews();
}

void MenuButton::setEnabled(bool value) {
	bool enabled = (_source && (_source->getCallback() != nullptr || _source->getNextMenu() != nullptr));
	if (enabled && value) {
		Button::setEnabled(value);
	} else {
		Button::setEnabled(false);
	}
}

void MenuButton::setWrapName(bool value) {
	if (value != _wrapName) {
		_wrapName = value;
		_contentSizeDirty = true;
	}
}
bool MenuButton::isWrapName() const {
	return _wrapName;
}

void MenuButton::onLightLevel() {
	auto level = material::ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case LightLevel::Dim:
		setStyle(Button::Style::FlatWhite);
		_menuNameLabel->setColor(Color::White);
		_menuValueLabel->setColor(Color::White);
		_menuNameIcon->setColor(Color::White);
		_menuValueIcon->setColor(Color::White);
		break;
	case LightLevel::Normal:
		setStyle(Button::Style::FlatBlack);
		_menuNameLabel->setColor(Color::Black);
		_menuValueLabel->setColor(Color::Black);
		_menuNameIcon->setColor(Color::Black);
		_menuValueIcon->setColor(Color::Black);
		break;
	case LightLevel::Washed:
		setStyle(Button::Style::FlatBlack);
		_menuNameLabel->setColor(Color::Black);
		_menuValueLabel->setColor(Color::Black);
		_menuNameIcon->setColor(Color::Black);
		_menuValueIcon->setColor(Color::Black);
		break;
	};
}

Label *MenuButton::getNameLabel() const {
	return _menuNameLabel;
}

Label *MenuButton::getValueLabel() const {
	return _menuValueLabel;
}

IconSprite *MenuButton::getNameIcon() const {
	return _menuNameIcon;
}
IconSprite *MenuButton::getValueIcon() const {
	return _menuValueIcon;
}

NS_MD_END
