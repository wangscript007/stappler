/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_DOCUMENT_SLDOCUMENT_H_
#define LAYOUT_DOCUMENT_SLDOCUMENT_H_

#include "SPBitmap.h"
#include "SLNode.h"
#include "SLCssDocument.h"

NS_LAYOUT_BEGIN

class Document : public Ref {
public:
	using check_data_fn = bool (*) (const BytesViewNetwork &str, const StringView &ct);
	using load_data_fn = Rc<Document> (*) (const BytesViewNetwork &, const StringView &ct);

	using check_file_fn = bool (*) (const StringView &path, const StringView &ct);
	using load_file_fn = Rc<Document> (*) (const StringView &path, const StringView &ct);

	struct DocumentFormat {
		check_data_fn check_data;
		check_file_fn check_file;

		load_data_fn load_data;
		load_file_fn load_file;

		size_t priority = 0;

		DocumentFormat(check_file_fn, load_file_fn, check_data_fn, load_data_fn, size_t = 0);
		~DocumentFormat();

		DocumentFormat(const DocumentFormat &) = delete;
		DocumentFormat(DocumentFormat &&) = delete;
		DocumentFormat & operator=(const DocumentFormat &) = delete;
		DocumentFormat & operator=(DocumentFormat &&) = delete;
	};

	struct Image {
		enum Type {
			Embed,
			Local,
			Web,
		};

		Type type;

		uint16_t width;
		uint16_t height;

		size_t offset;
		size_t length;
		String encoding;

		String name;
		String ref;

		Image(const MultipartParser::Image &);
		Image(uint16_t width, uint16_t height, size_t size, const String &path, const String &ref = String());
	};

	struct ContentRecord {
		String label;
		String href;
		Vector<ContentRecord> childs;
	};

	struct AssetImageMeta {
		uint16_t width;
		uint16_t height;
	};

	struct AssetMeta {
		String type;
		Rc<CssDocument> css;
		AssetImageMeta image;
	};

	using ImageMap = Map<String, Image>;
	using GalleryMap = Map<String, Vector<String>>;
	using StringDocument = ValueWrapper<String, class StringDocumentTag>;

	static bool canOpenDocumnt(const StringView &path, const StringView &ct = StringView());
	static bool canOpenDocumnt(const BytesViewNetwork &data, const StringView &ct = StringView());

	static Rc<Document> openDocument(const StringView &path, const StringView &ct = StringView());
	static Rc<Document> openDocument(const BytesViewNetwork &data, const StringView &ct = StringView());

	static StringView resolveName(const StringView &);
	static StringView getImageName(const StringView &);
	static Vector<StringView> getImageOptions(const StringView &);

	Document();

	virtual ~Document() { }

	virtual bool init(const StringDocument &);
	virtual bool init(const FilePath &, const StringView &ct = StringView());
	virtual bool init(const BytesViewNetwork &, const StringView &ct = StringView());

	virtual void setMeta(const String &key, const String &value);
	virtual String getMeta(const String &) const;

	virtual bool isFileExists(const StringView &) const;
	virtual Bytes getFileData(const StringView &);
	virtual Bytes getImageData(const StringView &);
	virtual Pair<uint16_t, uint16_t> getImageSize(const StringView &);

	void storeData(const BytesViewNetwork &);
	bool prepare();

	const Vector<String> &getSpine() const;

	const ContentPage *getRoot() const;
	const ContentPage *getContentPage(const StringView &) const;
	const Node *getNodeById(const StringView &pagePath, const StringView &id) const;
	Pair<const ContentPage *, const Node *> getNodeByIdGlobal(const StringView &id) const;

	const ImageMap & getImages() const;
	const GalleryMap & getGalleryMap() const;
	const ContentRecord & getTableOfContents() const;
	const Map<String, ContentPage> & getContentPages() const;

	NodeId getMaxNodeId() const;

	// Default style, that can be redefined with css
	virtual Style beginStyle(const Node &, const Vector<const Node *> &, const MediaParameters &) const;

	// Default style, that can NOT be redefined with css
	virtual Style endStyle(const Node &, const Vector<const Node *> &, const MediaParameters &) const;

protected:
	Bytes readData(size_t offset, size_t len);

	virtual void processCss(const String &, const StringView &);
	virtual void processHtml(const String &, const StringView &, bool linear = true);
	virtual void processMeta(ContentPage &c, const Vector<Pair<String, String>> &);

	virtual void onStyleAttribute(Style &style, const StringView &tag, const StringView &name, const StringView &value, const MediaParameters &) const;

	void updateNodes();

	Map<String, ContentPage> _pages;
	Vector<String> _spine;

	ImageMap _images;
	GalleryMap _gallery;

	Bytes _data;
	String _filePath;
	String _contentType;

	ContentRecord _contents;

	Map<String, String> _meta;
	NodeId _maxNodeId = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_DOCUMENT_SLDOCUMENT_H_ */
