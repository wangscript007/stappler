// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "PGHandle.h"
#include "PGQuery.h"
#include "ResourceTemplates.h"

NS_SA_EXT_BEGIN(pg)

struct ExecParamData {
	std::array<const char *, 64> values;
	std::array<int, 64> sizes;
	std::array<int, 64> formats;

	Vector<const char *> valuesVec;
	Vector<int> sizesVec;
	Vector<int> formatsVec;

	const char *const * paramValues = nullptr;
	const int *paramLengths = nullptr;
	const int *paramFormats = nullptr;

	ExecParamData(const ExecQuery &query) {
		auto size = query.getParams().size();

		if (size > 64) {
			valuesVec.reserve(size);
			sizesVec.reserve(size);
			formatsVec.reserve(size);

			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = query.getParams().at(i);
				bool bin = query.getBinaryVec().at(i);
				valuesVec.emplace_back((const char *)d.data());
				sizesVec.emplace_back(int(d.size()));
				formatsVec.emplace_back(bin);
			}

			paramValues = valuesVec.data();
			paramLengths = sizesVec.data();
			paramFormats = formatsVec.data();
		} else {
			for (size_t i = 0; i < size; ++ i) {
				const Bytes &d = query.getParams().at(i);
				bool bin = query.getBinaryVec().at(i);
				values[i] = (const char *)d.data();
				sizes[i] = int(d.size());
				formats[i] = bin;
			}

			paramValues = values.data();
			paramLengths = sizes.data();
			paramFormats = formats.data();
		}
	}
};

Handle::Handle(apr_pool_t *p, ap_dbd_t *h) : pool(p), handle(h) {
	if (strcmp(apr_dbd_name(handle->driver), "pgsql") == 0) {
		conn = (PGconn *)apr_dbd_native_handle(handle->driver, handle->handle);
	}
}
Handle::Handle(Handle &&h) : pool(h.pool), handle(h.handle), conn(h.conn) {
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
}

Handle & Handle::operator=(Handle &&h) {
	pool = h.pool;
	handle = h.handle;
	conn = h.conn;
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
	return *this;
}

Handle::operator bool() const {
	return pool != nullptr && conn != nullptr;
}

bool Handle::beginTransaction_pg(TransactionLevel l) {
	switch (l) {
	case TransactionLevel::ReadCommited:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL READ COMMITTED"_weak)) {
			level = TransactionLevel::ReadCommited;
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::RepeatableRead:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL REPEATABLE READ"_weak)) {
			level = TransactionLevel::RepeatableRead;
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Serialized:
		if (performSimpleQuery("BEGIN ISOLATION LEVEL SERIALIZABLE"_weak)) {
			level = TransactionLevel::Serialized;
			transactionStatus = TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::cancelTransaction_pg() {
	transactionStatus = TransactionStatus::Rollback;
}

bool Handle::endTransaction_pg() {
	switch (transactionStatus) {
	case TransactionStatus::Commit:
		transactionStatus = TransactionStatus::None;
		if (performSimpleQuery("COMMIT"_weak)) {
			finalizeBroadcast();
			return true;
		}
		break;
	case TransactionStatus::Rollback:
		transactionStatus = TransactionStatus::None;
		if (performSimpleQuery("ROLLBACK"_weak)) {
			finalizeBroadcast();
			return false;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::finalizeBroadcast() {
	if (!_bcasts.empty()) {
		ExecQuery query;
		auto vals = query.insert("__broadcasts").fields("date", "msg").values();
		for (auto &it : _bcasts) {
			vals.values(it.first, move(it.second));
		}
		query.finalize();
		perform(query);
		_bcasts.clear();
	}
}

Result Handle::select(const ExecQuery &query) {
	if (!conn || getTransactionStatus() == TransactionStatus::Rollback) {
		return Result();
	}

	ExecParamData data(query);
	Result res(PQexecParams(conn, query.getQuery().weak().data(), query.getParams().size(), nullptr,
			data.paramValues, data.paramLengths, data.paramFormats, 1));
	if (!res || !res.success()) {
		messages::error("Database", "Fail to perform query");
		messages::debug("Database", "Fail to perform query", res.info());
	}

	lastError = res.getError();
	return res;
}

int64_t Handle::selectId(const ExecQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return 0;
	}
	Result res(select(query));
	if (!res.empty()) {
		return res.readId();
	}
	return 0;
}
size_t Handle::perform(const ExecQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return maxOf<size_t>();
	}
	Result res(select(query));
	if (res.success()) {
		return res.getAffectedRows();
	}
	return maxOf<size_t>();
}

data::Value Handle::select(const Scheme &scheme, const ExecQuery &query) {
	auto result = select(query);
	if (result) {
		return result.decode(scheme);
	}
	return data::Value();
}

data::Value Handle::select(const Field &field, const ExecQuery &query) {
	auto result = select(query);
	if (result) {
		return result.decode(field);
	}
	return data::Value();
}

bool Handle::performSimpleQuery(const String &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return false;
	}

	Result res(PQexec(conn, query.data()));
	lastError = res.getError();
	return res.success();
}

Result Handle::performSimpleSelect(const String &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return Result();
	}

	Result res(PQexec(conn, query.data()));
	lastError = res.getError();
	return res;
}

Resource *Handle::makeResource(ResourceType type, QueryList &&list, const Field *f) {
	switch (type) {
	case ResourceType::ResourceList: return new ResourceReslist(this, std::move(list));  break;
	case ResourceType::ReferenceSet: return new ResourceRefSet(this, std::move(list)); break;
	case ResourceType::Object: return new ResourceObject(this, std::move(list)); break;
	case ResourceType::Set: return new ResourceSet(this, std::move(list)); break;
	case ResourceType::File: return new ResourceFile(this, std::move(list), f); break;
	case ResourceType::Array: return new ResourceArray(this, std::move(list), f); break;
	}
	return nullptr;
}

NS_SA_EXT_END(pg)