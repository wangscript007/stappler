/*
 * Request.h
 *
 *  Created on: 20 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_REQUEST_REQUEST_H_
#define SERENITY_SRC_REQUEST_REQUEST_H_

#include "Connection.h"
#include "DatabaseHandle.h"
#include "SPUrl.h"

NS_SA_BEGIN

enum class CookieFlags {
	Secure = 1 << 0,
	HttpOnly = 1 << 1,
	SetOnError = 1 << 2,
	SetOnSuccess = 1 << 3,

	Default = 1 | 2 | 8 // Secure | HttpOnly | SetOnRequest
};

SP_DEFINE_ENUM_AS_MASK(CookieFlags)

class Request : public std::basic_ostream<char, std::char_traits<char>>, public AllocPool {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;
	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using Require = InputConfig::Require;

	enum Method : int {
		Get =                   0,
		Put =                   1,
		Post =                  2,
		Delete =                3,
		Connect =               4,
		Options =               5,
		Trace =                 6,
		Patch =                 7,
		Propfind =              8,
		Proppatch =             9,
		MkCol =                 10,
		Copy =                  11,
		Move =                  12,
		Lock =                  13,
		Unlock =                14,
		VersionControl =        15,
		Checkout =              16,
		Uncheckout =            17,
		Checkin =               18,
		Update =                19,
		Label =                 20,
		Report =                21,
		MkWirkspace =           22,
		MkActivity =            23,
		BaselineControl =       24,
		Merge =                 25,
		Invalid =               26,
	};

	Request();
	Request(request_rec *);
	Request & operator =(request_rec *);

	Request(Request &&);
	Request & operator =(Request &&);

	Request(const Request &);
	Request & operator =(const Request &);

	request_rec *request() const { return _request; }
	operator request_rec *() const { return _request; }
	operator bool () const { return _request != nullptr; }

	void setRequestHandler(RequestHandler *);
	RequestHandler *getRequestHandler() const;

	void writeData(const data::Value &, bool allowJsonP = false);

	void clearFilters();

public: /* request data */
	apr::weak_string getRequestLine() const;
	bool isSimpleRequest() const;
	bool isHeaderRequest() const;

	int getProtocolNumber() const;
	apr::weak_string getProtocol() const;
	apr::weak_string getHostname() const;

	apr_time_t getRequestTime() const;

	apr::weak_string getStatusLine() const;
	int getStatus() const;

	Method getMethod() const;
	apr::weak_string getMethodName() const;

	apr::weak_string getRangeLine() const;
	apr_off_t getContentLength() const;
	bool isChunked() const;

	apr_off_t getRemainingLength() const;
	apr_off_t getReadLength() const;

	apr::table getRequestHeaders() const;
	apr::table getResponseHeaders() const;
	apr::table getErrorHeaders() const;
	apr::table getSubprocessEnvironment() const;
	apr::table getNotes() const;
	apr::uri getParsedURI() const;

	apr::weak_string getDocumentRoot() const;
	apr::weak_string getContentType() const;
	apr::weak_string getHandler() const;
	apr::weak_string getContentEncoding() const;

	apr::weak_string getRequestUser() const;
	apr::weak_string getAuthType() const;

	apr::weak_string getUnparsedUri() const;
	apr::weak_string getUri() const;
	apr::weak_string getFilename() const;
	apr::weak_string getCanonicalFilename() const;
	apr::weak_string getPathInfo() const;
	apr::weak_string getQueryArgs() const;

	bool isEosSent() const;
	bool hasCache() const;
	bool hasLocalCopy() const;
	bool isSecureConnection() const;

	apr::weak_string getUseragentIp() const;

public: /* request params setters */
	void setDocumentRoot(apr::string &&str);
	void setContentType(apr::string &&str);
	void setHandler(apr::string &&str);
	void setContentEncoding(apr::string &&str);

	/* set path for file, that should be returned in response via sendfile */
	void setFilename(apr::string &&str);

	/* set HTTP status code and response status line ("404 NOT FOUND", etc.)
	 * if no string provided, default status line for code will be used */
	void setStatus(int status, apr::string &&str = apr::string());

	void setCookie(const String &name, const String &value, TimeInterval maxAge = TimeInterval(), CookieFlags flags = CookieFlags::Default, const String &path = "/"_weak);
	void removeCookie(const String &name, CookieFlags flags = CookieFlags::Default);

	apr::weak_string getCookie(const String &name, bool removeFromHeadersTable = true) const;

	int redirectTo(String && location);
	int sendFile(String && path, size_t cacheTimeInSeconds = maxOf<size_t>());
	int sendFile(String && path, String && contentType, size_t cacheTimeInSeconds = maxOf<size_t>());

	apr::string getFullHostname(int port = -1);

public: /* input config */
	InputConfig & getInputConfig();
	const InputConfig & getInputConfig() const;

	/* Sets required data flags for input filter on POST and PUT queries
	 * if no data is requested, onInsertFilter and onFilterComplete phases will be ignored */
	void setRequiredData(InputConfig::Require);

	/* Sets max size for input query content length, checked from Content-Length header */
	void setMaxRequestSize(size_t);

	/* Sets max size for a single input variable (in urlencoded and multipart types)
	 * Has no effect on JSON or CBOR input */
	void setMaxVarSize(size_t);

	/* Sets max size for input file (for multipart or unrecognized Content-Type)
	 * File size will be checked before writing next chunk of data,
	 * if new size will exceed max size, filter will be aborted */
	void setMaxFileSize(size_t);

public: /* engine and errors */
	const apr::vector<apr::string> & getParsedQueryPath() const;
	const data::Value &getParsedQueryArgs() const;

	Server server() const;
	Connection connection() const;
	apr_pool_t *pool() const;

	storage::Adapter * storage() const;

	// authorize user and create session for 'maxAge' seconds
	// previous session (if existed) will be canceled
	Session *authorizeUser(User *, TimeInterval maxAge);

	// explicitly set user authority to request
	void setUser(User *);

	// try to access to session data
	// if session object already created - returns it
	// if all authorization data is valid - session object will be created
	// otherwise - returns nullptr
	Session *getSession();

	// try to get user data from session (by 'getSession' call)
	// if session is not existed - returns nullptr
	// if session is anonymous - returns nullptr
	User *getUser();
	User *getAuthorizedUser() const;

	// check if request is sent by server/handler administrator
	// uses 'User::isAdmin' or tries to authorize admin by cross-server protocol
	bool isAdministrative();

	const Vector<data::Value> & getDebugMessages() const;
	const Vector<data::Value> & getErrorMessages() const;

	void addErrorMessage(data::Value &&);
	void addDebugMessage(data::Value &&);

protected:
	struct Config;

	/* Buffer class used as basic_streambuf to allow stream writing to request
	 * like 'request << "String you want to send"; */
	class Buffer : public std::basic_streambuf<char, std::char_traits<char>> {
	public:
		using int_type = typename traits_type::int_type;
		using pos_type = typename traits_type::pos_type;
		using off_type = typename traits_type::off_type;

		using streamsize = std::streamsize;
		using streamoff = std::streamoff;

		using ios_base = std::ios_base;

		Buffer(request_rec *r);
		Buffer(Buffer&&);
		Buffer& operator=(Buffer&&);

		Buffer(const Buffer&);
		Buffer& operator=(const Buffer&);

	protected:
		virtual int_type overflow(int_type c = traits_type::eof()) override;

		virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) override;
		virtual pos_type seekpos(pos_type pos, ios_base::openmode mode) override;

		virtual int sync() override;

		virtual streamsize xsputn(const char_type* s, streamsize n) override;

		request_rec *_request;
	};

	Buffer _buffer;
	request_rec *_request = nullptr;
	Config *_config = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_REQUEST_H_ */
