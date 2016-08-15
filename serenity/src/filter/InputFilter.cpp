/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Define.h"
#include "InputFilter.h"
#include "UrlEncodeParser.h"
#include "MultipartParser.h"
#include "DataParser.h"
#include "FileParser.h"
#include "Root.h"
#include "SPString.h"
#include "SPFilesystem.h"

NS_SA_BEGIN

InputFile::InputFile(apr::string &&name, apr::string && type, apr::string && enc, apr::string && orig, size_t s)
: name(std::move(name)), type(std::move(type)), encoding(std::move(enc))
, original(std::move(orig)), writeSize(0), headerSize(s) {
	file.open_tmp(config::getUploadTmpFilePrefix(), APR_FOPEN_CREATE | APR_FOPEN_READ | APR_FOPEN_WRITE | APR_FOPEN_EXCL);
	path.assign_weak(file.path());
}

InputFile::~InputFile() {
	close();
}

bool InputFile::isOpen() const {
	return file.is_open();
}

size_t InputFile::write(const char *s, size_t n) {
	writeSize += n;
	return file.xsputn(s, n);
}

void InputFile::close() {
	file.close_remove();
}

bool InputFile::save(const apr::string &path) const {
	return const_cast<apr::file &>(file).close_rename(filesystem::cachesPath(path).c_str());
}

InputParser::InputParser(const InputConfig &cfg, size_t len)
: config(cfg), length(len), basicParser(root, len, cfg.maxVarSize) { }

void InputParser::cleanup() {
	for (auto &it : files) {
		it.close();
	}
	files.clear();
}

const InputConfig &InputParser::getConfig() const {
	return config;
}

data::Value *InputParser::flushString(CharReaderBase &r, data::Value *current, VarState state) {
	return basicParser.flushString(r, current, state);
}

void InputFilter::filterRegister() {
	ap_register_input_filter("Serenity::InputFilter", &(filterFunc),
			&(filterInit), AP_FTYPE_CONTENT_SET);
}

InputFilter::Accept getAcceptedData(const Request &req, InputFilter::Exception &e) {
	Request r = req;
	auto &cfg = r.getInputConfig();
	InputFilter::Accept ret = InputFilter::Accept::None;
	if (r.getMethod() == M_POST || r.getMethod() == M_PUT || r.getMethod() == M_PATCH) {
		const auto &ct = r.getRequestHeaders().at("Content-Type");
		const auto &b = r.getRequestHeaders().at("Content-Length");

		size_t cl = 0;
		if (!b.empty()) {
			cl = (size_t)apr_atoi64(b.c_str());
		}

		if (cl > config::getMaxInputPostSize() || cl > cfg.maxRequestSize) {
			messages::error("InputFilter", "Request size is out of limits", data::Value{
				std::make_pair("length", data::Value((int64_t)cl)),
				std::make_pair("local", data::Value((int64_t)cfg.maxRequestSize)),
				std::make_pair("global", data::Value((int64_t)config::getMaxInputPostSize())),
			});
			e = InputFilter::Exception::TooLarge;
			return ret;
		}

 		if (!ct.empty() && cl != 0 && cl < config::getMaxInputPostSize()) {
			if (ct.compare(0, 16, "application/json") == 0 || ct.compare(0, 16, "application/cbor") == 0) {
				if ((cfg.required & InputConfig::Require::Data) == InputConfig::Require::None
						&& (cfg.required & InputConfig::Require::Body) == InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", data::Value{
						std::make_pair("content", data::Value(ct)),
						std::make_pair("available", data::Value("data, body")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Json;
			} else if (ct.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
				if ((cfg.required & InputConfig::Require::Data) == InputConfig::Require::None
						&& (cfg.required & InputConfig::Require::Body) == InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", data::Value{
						std::make_pair("content", data::Value(ct)),
						std::make_pair("available", data::Value("data, body")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Urlencoded;
			} else if (ct.compare(0, 30, "multipart/form-data; boundary=") == 0) {
				if ((cfg.required & InputConfig::Require::Data) == InputConfig::Require::None
						&& (cfg.required & InputConfig::Require::Body) == InputConfig::Require::None
						&& (cfg.required & InputConfig::Require::Files) == InputConfig::Require::None
						&& (cfg.required & InputConfig::Require::FilesAsData) == InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", data::Value{
						std::make_pair("content", data::Value(ct)),
						std::make_pair("available", data::Value("data, body, files")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Multipart;
			} else {
				if ((cfg.required & InputConfig::Require::Files) == InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", data::Value{
						std::make_pair("content", data::Value(ct)),
						std::make_pair("available", data::Value("files")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Files;
			}
		} else {
			e = InputFilter::Exception::Unrecognized;
		}
	}
	return ret;
}

InputFilter::Exception InputFilter::insert(const Request &r) {
	return AllocStack::perform([&] () -> InputFilter::Exception {
		Exception e = Exception::None;
		auto accept = getAcceptedData(r, e);
		if (accept == Accept::None) {
			return e;
		}

		auto f = new (r.request()->pool) InputFilter(r, accept);
		ap_add_input_filter("Serenity::InputFilter", (void *)f, r, r.connection());
		return e;
	}, r.request());
}

apr_status_t InputFilter::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {
	return AllocStack::perform([&] () -> apr_status_t {
		if (f->ctx) {
			InputFilter *filter = (InputFilter *) f->ctx;
			return filter->func(f, bb, mode, block, readbytes);
		} else {
			return ap_get_brigade(f->next, bb, mode, block, readbytes);
		}
	}, f->r);
}

int InputFilter::filterInit(ap_filter_t *f) {
	return AllocStack::perform([&] () -> int {
		if (f->ctx) {
			InputFilter *filter = (InputFilter *) f->ctx;
			return filter->init(f);
		} else {
			return OK;
		}
	}, f->r);
}

InputFilter::InputFilter(const Request &r, Accept a) : _body() {
	_accept = a;
	_request = r;

	_isStarted = false;
	_isCompleted = false;

	const auto &b = _request.getRequestHeaders().at("Content-Length");

	if (!b.empty()) {
		_contentLength = apr_atoi64(b.c_str());
	}
}

int InputFilter::init(ap_filter_t *f) {
	_time = Time::now();
	_startTime = Time::now();
	_isStarted = true;

	if (_accept == Accept::Multipart) {
		const auto &ct = _request.getRequestHeaders().at("Content-Type");
		auto pos = ct.find("boundary="_weak);
		if (pos != apr::string::npos) {
			_parser = new MultipartParser(_request.getInputConfig(), _contentLength,
					ct.substr(pos + "boundary="_len));
		}
	} else if (_accept == Accept::Urlencoded) {
		_parser = new UrlEncodeParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Json) {
		_parser = new DataParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Files) {
		const auto &ct = _request.getRequestHeaders().at("Content-Type");
		const auto &name = _request.getRequestHeaders().at("X-File-Name");

		_parser = new FileParser(_request.getInputConfig(), ct, name, _contentLength);
	}

	if (isBodySavingAllowed()) {
		_body.reserve(_contentLength);
	}

	if (_parser) {
		Root::getInstance()->onFilterInit(this);
	}

	return OK;
}

apr_status_t InputFilter::func(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {

	apr_status_t rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
	if (rv == APR_SUCCESS && _parser && getConfig().required != InputConfig::Require::None && _accept != Accept::None) {
		if (_read + readbytes > _contentLength) {
			_unupdated += _contentLength - _read;
			_read = _contentLength;
		} else {
			_read += readbytes;
			_unupdated += readbytes;
		}

		if (_read > getConfig().maxRequestSize) {
			f->ctx = NULL;
			return rv;
		}

		auto t = Time::now();
		_timer = t - _time;
		_time = t;

		if (_timer > getConfig().updateTime || _unupdated > (_contentLength * getConfig().updateFrequency)) {
			Root::getInstance()->onFilterUpdate(this);
			_timer = TimeInterval();
			_unupdated = 0;
		}

		apr_bucket *b = NULL;
		for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); b = APR_BUCKET_NEXT(b)) {
			if (!_eos) {
				step(b, block);
			}
		}
	}
	return rv;
}

void InputFilter::step(apr_bucket* b, apr_read_type_e block) {
	if (getConfig().required == InputConfig::Require::None || _accept == Accept::None) {
		return;
	}
	if (!APR_BUCKET_IS_METADATA(b)) {
		const char *str;
		size_t len;
		apr_bucket_read(b, &str, &len, block);
		Reader reader(str, len);

		if (isBodySavingAllowed()) {
			_body.write(str, len);
		}
		if (_parser && (
				(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
				(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
				(_accept == Accept::Json && isDataParsingAllowed()) ||
				(_accept == Accept::Files && isFileUploadAllowed()))) {
			_parser->run(str, len);
		}
	} else if (APR_BUCKET_IS_EOS(b)) {
		if (_parser && (
				(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
				(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
				(_accept == Accept::Json && isDataParsingAllowed()) ||
				(_accept == Accept::Files && isFileUploadAllowed()))) {
			_parser->finalize();
			auto &files = _parser->getFiles();
			if (!files.empty()) {

			}
		}
		_eos = true;
		_isCompleted = true;
		Root::getInstance()->onFilterComplete(this);
		if (_parser) {
			_parser->cleanup();
		}
	}
}

size_t InputFilter::getContentLength() const {
	return _contentLength;
}
size_t InputFilter::getBytesRead() const {
	return _read;
}
size_t InputFilter::getBytesReadSinceUpdate() const {
	return _unupdated;
}

Time InputFilter::getStartTime() const {
	return _startTime;
}
TimeInterval InputFilter::getElapsedTime() const {
	return Time::now() - _startTime;
}
TimeInterval InputFilter::getElapsedTimeSinceUpdate() const {
	return Time::now() - _time;
}

bool InputFilter::isFileUploadAllowed() const {
	return (getConfig().required & InputConfig::Require::Files) != InputConfig::Require::None;
}
bool InputFilter::isDataParsingAllowed() const {
	return (getConfig().required & InputConfig::Require::Data) != InputConfig::Require::None;
}
bool InputFilter::isBodySavingAllowed() const {
	return (getConfig().required & InputConfig::Require::Body) != InputConfig::Require::None;
}

bool InputFilter::isCompleted() const {
	return _isCompleted;
}

const apr::ostringstream & InputFilter::getBody() const {
	return _body;
}
data::Value & InputFilter::getData() {
	return _parser->getData();
}
apr::array<InputFile> &InputFilter::getFiles() {
	return _parser->getFiles();
}

const InputConfig & InputFilter::getConfig() const {
	return _request.getInputConfig();
}

Request InputFilter::getRequest() const {
	return _request;
}

NS_SA_END
