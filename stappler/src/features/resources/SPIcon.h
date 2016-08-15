/*
 * SPIcon.h
 *
 *  Created on: 07 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_
#define LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_

#include "SPImage.h"
#include "base/CCMap.h"

NS_SP_BEGIN

class Icon : public cocos2d::Ref {
public:
	Icon(uint16_t id, uint16_t x, uint16_t y, uint16_t width, uint16_t height, float density, Image *tex);
	virtual ~Icon();

	inline uint16_t getId() const { return _id; }
	inline uint16_t getWidth() const { return _width; }
	inline uint16_t getHeight() const { return _height; }
	inline Image *getImage() const { return _image; }
	inline float getDensity() const { return _density; }

	cocos2d::Texture2D *getTexture() const;
	cocos2d::Rect getTextureRect() const;

private:
	uint16_t _id;
	uint16_t _x;
	uint16_t _y;
	uint16_t _width;
	uint16_t _height;
	float _density;
	Image *_image;
};

class IconSet : public cocos2d::Ref {
public:
	using Callback = std::function<void(IconSet *)>;
	struct Config {
		std::string name;
		uint16_t version;
		std::map<std::string, std::string> data;
		uint16_t originalWidth;
		uint16_t originalHeight;
		uint16_t iconWidth;
		uint16_t iconHeight;
	};

	static void generate(Config &&, const Callback &callback);

	Icon *getIcon(const std::string &) const;

	IconSet(Config &&, cocos2d::Map<std::string, Icon *> &&icons, Image *image);
	~IconSet();

	uint16_t getOriginalWidth() const { return _config.originalWidth; }
	uint16_t getOriginalHeight() const { return _config.originalHeight; }
	uint16_t getIconHeight() const { return _config.iconHeight; }
	uint16_t getIconWidth() const { return _config.iconWidth; }

	const std::string &getName() const { return _config.name; }
	uint16_t getVersion() const { return _config.version; }

protected:
	Config _config;
	uint16_t _texWidth = 0;
	uint16_t _texHeight = 0;

	Rc<Image>_image = nullptr;
	cocos2d::Map<std::string, Icon *> _icons;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_ */
