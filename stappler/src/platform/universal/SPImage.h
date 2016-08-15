//
//  SPImage.h
//  stappler
//
//  Created by SBKarr on 14.02.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPImage__
#define __stappler__SPImage__

#include "SPBitmap.h"
#include "SPEventHandler.h"

#ifndef SP_RESTRICT
#include "renderer/CCTexture2D.h"
#endif

NS_SP_BEGIN

#ifndef SP_RESTRICT

/** Image - ресурс для хранения изображений и текстур на их основе
 *
 * Пока существует экземпляр Image, он может хранить данные и/или текстуру изображения в памяти
 * Данные и текстура управляются внутренними счётчиками ссылок,
 * Вызов retainData/retainTexture увеличивает соответствующий счётчик на 1,
 * и загружает соответсвующие данные в память, если они ещё не загружены.
 * вызов releaseData/releaseTexture уменьшает соостветствующий счётчик на 1,
 * и выгружает данные из памяти, есть счётчик достиг значения 0
 *
 * В некоторых случаях эффективно постоянное хранение текстуры в памяти.
 * Для этого устанавливается флаг persistent.
 * Если флаг установлен - объкт управляет временем жизни текстуры и данных автоматически.
 * Текстура хранится всё время жизни объекта, использование памяти объектом минимизируется
 *
 * Помните, что cocos2d::Texture2D имеет свой собственный счётчик ссылок.
 * Удаление экземпляра Image не повредит уже использующимся текстурам.
 */

class Image : public cocos2d::Ref, public EventHandler {
public:
	using Format = Bitmap::Format;

	enum class Type {
		File,
		Data,
		Bitmap
	};

    /** savePng - сохраняет данные в PNG файл */
	static void savePng(const std::string &filename, const uint8_t *data, uint32_t width, uint32_t height, Format fmt);
	static void savePng(const std::string &filename, cocos2d::Texture2D *tex);

	/** Записывает в память данные изображения в виде PNG. */
	static std::vector<uint8_t> writePng(const uint8_t *data, uint32_t width, uint32_t height, Format fmt);
	static std::vector<uint8_t> writePng(cocos2d::Texture2D *tex);

	static cocos2d::Texture2D::PixelFormat getPixelFormat(Image::Format fmt);

public:
	/* Создаёт Image для чтения данных из файла */
	static Image *createWithFile(const std::string &filename, Format fmt = Format::RGBA8888, bool persistent = false);

	static Image *createWithBitmap(std::vector<uint8_t> &&, uint32_t width, uint32_t height, Format fmt, bool persistent = false);
	static Image *createWithBitmap(Bitmap &&, bool persistent = false);

	static Image *createWithData(std::vector<uint8_t> &&, bool persistent = false);
	static Image *createWithData(std::vector<uint8_t> &&, uint32_t width, uint32_t height, Format fmt, bool persistent = false);

	bool initWithFile(const std::string &filename, Format fmt = Format::RGBA8888, bool persistant = false);

	bool initWithBitmap(std::vector<uint8_t> &&, uint32_t width, uint32_t height, Format fmt, bool persistent = false);
	bool initWithBitmap(Bitmap &&, bool persistent = false);

	bool initWithData(std::vector<uint8_t> &&, bool persistent = false);
	bool initWithData(std::vector<uint8_t> &&, uint32_t width, uint32_t height, Format fmt, bool persistent = false);

	virtual ~Image();

	bool retainData();
	void releaseData();

	inline bool isPersistant() const { return _persistent; }
	inline bool isDataLoaded() const { return !_bmp.empty(); }
	inline unsigned int getWidth() const { return _bmp.width(); }
	inline unsigned int getHeight() const { return _bmp.height(); }
	inline const std::vector<uint8_t> &getData() const { return _bmp.data(); }

	const std::string &getFilename() { return _filename; }

	cocos2d::Texture2D *retainTexture();
	void releaseTexture();

private:
	cocos2d::Texture2D *getTexture();
	bool loadData();
	void freeDataAndTexture();

	Image();

	Bitmap _bmp;
	uint32_t _dataRefCount = 0;

	Type _type = Type::File;

	std::string _filename;
	std::vector<uint8_t> _fileData;

	cocos2d::Texture2D *_texture = nullptr;
	uint32_t _textureRefCount = 0;
	bool _persistent = false;

	void reloadTexture();
};

#endif

NS_SP_END

#endif /* defined(__stappler__SPImage__) */
