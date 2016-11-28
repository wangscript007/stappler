/*
 * SAForward.h
 *
 *  Created on: 15 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_CORE_FORWARD_H_
#define SERENITY_SRC_CORE_FORWARD_H_

#include "SPCommon.h"
#include "SPString.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPLog.h"

#include <atomic>
#include <idna.h>
#include <postgresql/libpq-fe.h>

#include "apr_hash.h"
#include "apr_strings.h"
#include "apr_pools.h"
#include "apr_reslist.h"
#include "apr_atomic.h"
#include "apr_random.h"
#include "apr_dbd.h"
#include "apr_uuid.h"
#include "apr_md5.h"
#include "apr_base64.h"
#include "apr_errno.h"
#include "apr_time.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_cond.h"
#include "apr_sha1.h"

#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_connection.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"
#include "mod_dbd.h"
#include "mod_ssl.h"
#include "mod_log_config.h"
#include "util_cookies.h"

#define MAGICKCORE_QUANTUM_DEPTH 16
#define MAGICKCORE_HDRI_ENABLE 0

#include "magick/MagickCore.h"

#define NS_SA_BEGIN				NS_SP_EXT_BEGIN(serenity)
#define NS_SA_END				NS_SP_EXT_END(serenity)
#define USING_NS_SA				using namespace stappler::serenity

#define NS_SA_EXT_BEGIN(v)		NS_SP_EXT_BEGIN(serenity) namespace v {
#define NS_SA_EXT_END(v)		NS_SP_EXT_END(serenity) }
#define USING_NS_SA_EXT(v)		using namespace stappler::serenity::v

NS_SA_BEGIN

class Root;
class Server;
class Connection;

class ServerComponent;
class Request;
class InputFilter;

class RequestHandler;

class AccessControl;
class Resource;
class ResourceHandler;

using AllocStack = apr::AllocStack;
using AllocPool = apr::AllocPool;

class Session;
class User;
class InputFile;

NS_SA_END


#ifndef NOCB
NS_SA_EXT_BEGIN(couchbase)

class Handle;
class Connection;

NS_SA_EXT_END(couchbase)
#endif

NS_SA_EXT_BEGIN(storage)

class Field;

struct FieldText;
struct FieldPassword;
struct FieldExtra;
struct FieldFile;
struct FieldImage;
struct FieldObject;
struct FieldArray;

class Adapter;
class Resolver;
class Object;
class Scheme;

struct Query;
class File;

NS_SA_EXT_END(storage)


NS_SA_EXT_BEGIN(idn)

String toAscii(const String &, bool validate = true);
String toUnicode(const String &, bool validate = false);

NS_SA_EXT_END(idn)


NS_SA_EXT_BEGIN(valid)

/** Identifier starts with [a-zA-Z_] and can contain [a-zA-Z0-9_\-.@] */
bool validateIdentifier(const String &str);

/** Text can contain all characters above 0x1F and \t, \r, \n, \b, \f */
bool validateText(const String &str);

bool validateEmail(String &str);
bool validateUrl(String &str);

bool validateNumber(const String &str);
bool validateHexadecimial(const String &str);
bool validateBase64(const String &str);

Bytes makePassword(const String &str, const String &key = "");
bool validatePassord(const String &str, const Bytes &passwd, const String &key = "");

NS_SA_EXT_END(valid)


NS_SA_EXT_BEGIN(websocket)

class Handler;
class Manager;

NS_SA_EXT_END(websocket)


NS_SA_EXT_BEGIN(tpl)

class Cache;
class Exec;

NS_SA_EXT_END(tpl)


NS_SA_EXT_BEGIN(messages)

/* Serenity cross-server messaging interface
 *
 * error(Tag, Name, Data = nullptr)
 *  - send information about error to current output interface,
 *  if debug interface is active, it will receive this message
 *
 * debug(Tag, Name, Data = nullptr)
 *  - send debug information to current output interface and
 *  connected debug interface
 *
 * broadcast(Message)
 *  - send cross-server broadcast with message
 *
 *  Broadcast format:
 *  (message) ->  { "message": true, "level": "debug"/"error", "data": Data }
 *   - used for system errors, warnings and debug, received by admin shell
 *
 *  (websocket) -> { "server": "SERVER_NAME", "url": "WEBSOCKET_URL", "data": Data }
 *   - used for communication between websockets, received by all websockets with the same server id and url
 *
 *  (option) -> { "system": true, "option": "OPTION_NAME", "OPTION_NAME": NewValue }
 *   - used for system-wide option switch, received only by system
 *
 *   NOTE: All broadcast messages coded as CBOR, so, BYTESTRING type is safe
 */

bool isDebugEnabled();
void setDebugEnabled(bool);

void _addErrorMessage(data::Value &&);
void _addDebugMessage(data::Value &&);

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text) {
	_addErrorMessage(data::Value{
		std::make_pair("source", data::Value(std::forward<Source>(source))),
		std::make_pair("text", data::Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text, data::Value &&d) {
	_addErrorMessage(data::Value{
		std::make_pair("source", data::Value(std::forward<Source>(source))),
		std::make_pair("text", data::Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text) {
	_addDebugMessage(data::Value{
		std::make_pair("source", data::Value(std::forward<Source>(source))),
		std::make_pair("text", data::Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text, data::Value &&d) {
	_addDebugMessage(data::Value{
		std::make_pair("source", data::Value(std::forward<Source>(source))),
		std::make_pair("text", data::Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename ... Args>
void error(Args && ...args) {
	_addError(std::forward<Args>(args)...);
}

template <typename ... Args>
void debug(Args && ...args) {
	if (isDebugEnabled()) {
		_addDebug(std::forward<Args>(args)...);
	}
}

void broadcast(const data::Value &);
void broadcast(const Bytes &);

void setNotifications(apr_pool_t *, const Function<void(data::Value &&)> &error, const Function<void(data::Value &&)> &debug);

NS_SA_EXT_END(messages)


#endif /* SERENITY_SRC_CORE_FORWARD_H_ */
