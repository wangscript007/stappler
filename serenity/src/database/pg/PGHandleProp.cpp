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
#include "StorageScheme.h"
#include "StorageFile.h"
#include "StorageFile.h"

NS_SA_EXT_BEGIN(pg)

static void Handle_writeSelectSetQuery(ExecQuery &query, const Scheme &s, uint64_t oid, const Field &f) {
	auto fs = f.getForeignScheme();
	if (f.isReference()) {
		query.with("s", [&] (ExecQuery::GenericQuery &q) {
			q.select(ExecQuery::Field(toString(fs->getName(), "_id")).as("id"))
					.from(toString(s.getName(), "_f_", f.getName()))
					.where(toString(s.getName(), "_id"), Comparation::Equal, oid);
		}).select( ExecQuery::Field::all(fs->getName()) )
				.from(fs->getName()).innerJoinOn("s", [&] (ExecQuery::WhereBegin &q) {
			q.where(ExecQuery::Field(fs->getName(), "__oid"), Comparation::Equal, ExecQuery::Field("s", "id"));
		}).finalize();
	} else {
		auto l = s.getForeignLink(f);
		query.select().from(fs->getName()).where(l->getName(), Comparation::Equal, oid).finalize();
	}
}

data::Value Handle::getProperty(const Scheme &s, uint64_t oid, const Field &f) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		query.with("s", [&] (ExecQuery::GenericQuery &q) {
			q.select(f.getName()).from(s.getName()).where("__oid", Comparation::Equal, oid);
		}).select(ExecQuery::Field::all("__files")).from("__files").innerJoinOn("s", [&] (ExecQuery::WhereBegin &q) {
			q.where(ExecQuery::Field("__files","__oid"), Comparation::Equal, ExecQuery::Field("s", f.getName()));
		}).finalize();
		ret = select(*File::getScheme(), query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		break;
	case storage::Type::Array:
		query.select("data").from(toString(s.getName(), "_f_", f.getName()))
			.where(toString(s.getName(), "_id"), Comparation::Equal, oid).finalize();
		ret = select(f, query);
		break;
	case storage::Type::Object:
		if (auto fs = f.getForeignScheme()) {
			query.with("s", [&] (ExecQuery::GenericQuery &q) {
				q.select(f.getName()).from(s.getName()).where("__oid", Comparation::Equal, oid);
			}).select(ExecQuery::Field::all(fs->getName())).from(fs->getName()).innerJoinOn("s", [&] (ExecQuery::WhereBegin &q) {
				q.where(ExecQuery::Field(fs->getName(), "__oid"), Comparation::Equal, ExecQuery::Field("s", f.getName()));
			}).finalize();
			ret = select(*fs, query);
			if (ret.isArray()) {
				ret = std::move(ret.getValue(0));
			}
		}
		break;
	case storage::Type::Set:
		if (auto fs = f.getForeignScheme()) {
			Handle_writeSelectSetQuery(query, s, oid, f);
			ret = select(*fs, query);
		}
		break;
	default:
		query.select(f.getName()).from(s.getName()).where("__oid", Comparation::Equal, oid).finalize();
		ret = select(s, query);
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		if (ret.isDictionary()) {
			ret = ret.getValue(f.getName());
		}
		break;
	}
	return ret;
}

data::Value Handle::getProperty(const Scheme &s, const data::Value &d, const Field &f) {
	ExecQuery query;
	data::Value ret;
	auto oid = d.getInteger("__oid");
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return storage::File::getData(this, d.getInteger(f.getName()));
		break;
	case storage::Type::Array:
		query << "SELECT data FROM " << s.getName() << "_f_" << f.getName() << " WHERE " << s.getName() << "_id=" << oid << ";";
		ret = select(f, query);
		break;
	case storage::Type::Object:
		if (auto fs = f.getForeignScheme()) {
			query << "SELECT t.* FROM " << fs->getName() << " t WHERE t.__oid=" << d.getInteger(f.getName()) << ";";
			ret = select(*fs, query);
			if (ret.isArray()) {
				ret = std::move(ret.getValue(0));
			}
		}
		break;
	case storage::Type::Set:
		if (auto fs = f.getForeignScheme()) {
			Handle_writeSelectSetQuery(query, s, oid, f);
			ret = select(*fs, query);
		}
		break;
	default:
		return d.getValue(f.getName());
		break;
	}
	return ret;
}

data::Value Handle::setProperty(const Scheme &s, uint64_t oid, const Field &f, data::Value && val) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return data::Value(); // file update should be done by scheme itself
		break;
	case storage::Type::Array:
		if (val.isArray()) {
			clearProperty(s, oid, f, data::Value());
			insertIntoArray(query, s, oid, f, val);
			ret = move(val);
		}
		break;
	case storage::Type::Set:
		if (f.isReference()) {
			auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
			if (objField->onRemove == storage::RemovePolicy::Reference) {
				clearProperty(s, oid, f, data::Value());
			} else {
				auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

				auto & source = s.getName();
				auto & target = obj->scheme->getName();

				query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
						<< s.getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ")";

				for (auto &it : val.asArray()) {
					if (it.isInteger()) {
						query << " AND __oid != " << it.asInteger();
					}
				}
				query << ";";
				perform(query);
			}

			ret = appendProperty(s, oid, f, move(val));
		}
		// not implemented
		break;
	default: {
		data::Value patch;
		patch.setValue(val, f.getName());
		patchObject(s, oid, patch);
		return val;
	}
		break;
	}
	return ret;
}

data::Value Handle::setProperty(const Scheme &s, const data::Value &d, const Field &f, data::Value && val) {
	return setProperty(s, (uint64_t)d.getInteger("__oid"), f, std::move(val));
}

bool Handle::clearProperty(const Scheme &s, uint64_t oid, const Field &f, data::Value && hint) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		query << "DELETE FROM " << s.getName() << "_f_" << f.getName() << " WHERE " << s.getName() << "_id=" << oid << ";";
		return perform(query) != maxOf<size_t>();
		break;
	case storage::Type::Set:
		if (f.isReference()) {
			auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
			if (!hint.isArray()) {
				if (objField->onRemove == storage::RemovePolicy::Reference) {
					query.remove(toString(s.getName(), "_f_", f.getName()))
							.where(toString(s.getName(), "_id"), Comparation::Equal, oid)
							.finalize();
					return perform(query) != maxOf<size_t>();
				} else {
					auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

					auto & source = s.getName();
					auto & target = obj->scheme->getName();

					query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
							<< s.getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ");";
					return perform(query) != maxOf<size_t>();
				}
			} else {
				Vector<int64_t> toRemove;
				for (auto &it : hint.asArray()) {
					if (auto id = it.asInteger()) {
						toRemove.push_back(id);
					}
				}

				return cleanupRefSet(s, oid, f, toRemove);
			}
		}
		break;
	default: {
		data::Value patch;
		patch.setValue(data::Value(), f.getName());
		patchObject(s, oid, patch);
	}
		break;
	}
	return true;
}

bool Handle::clearProperty(const Scheme &s, const data::Value &d, const Field &f, data::Value && hint) {
	return clearProperty(s, (uint64_t)d.getInteger("__oid"), f, move(hint));
}

data::Value Handle::appendProperty(const Scheme &s, uint64_t oid, const Field &f, data::Value && val) {
	ExecQuery query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		if (!val.isNull()) {
			if (insertIntoArray(query, s, oid, f, val)) {
				ret = move(val);
			}
		}
		return val;
		break;
	case storage::Type::Set:
		if (f.isReference()) {
			Vector<int64_t> toAdd;
			for (auto &it : val.asArray()) {
				if (auto id = it.asInteger()) {
					toAdd.push_back(id);
				}
			}
			if (patchRefSet(s, oid, f, toAdd)) {
				ret = move(val);
			}
		}
		break;
	default:
		break;
	}
	return ret;
}

data::Value Handle::appendProperty(const Scheme &s, const data::Value &d, const Field &f, data::Value && val) {
	return appendProperty(s, d.getInteger("__oid"), f, std::move(val));
}

bool Handle::insertIntoSet(ExecQuery &query, const Scheme &s, int64_t id, const FieldObject &field, const Field &ref, const Value &d) {
	if (field.type == Type::Object) {
		if (ref.getType() == Type::Object) {
			// object to object is not implemented
		} else {
			// object to set is maintained by trigger
		}
	} else if (field.type == Type::Set) {
		if (ref.getType() == Type::Object) {
			if (d.isArray() && d.getValue(0).isInteger()) {
				auto w = query.update(field.scheme->getName()).set(ref.getName(), id).where();
				for (auto & it : d.asArray()) {
					if (it.isInteger()) {
						w.where(Operator::Or, "__oid", Comparation::Equal, it.getInteger());
					}
				}
				w.finalize();
				return perform(query) != maxOf<size_t>();
			}
		} else {
			// set to set is not implemented
		}
	}
	return false;
}

bool Handle::insertIntoArray(ExecQuery &query, const Scheme &scheme, int64_t id, const Field &field, const Value &d) {
	if (d.isNull()) {
		query.remove(toString(scheme.getName(), "_f_", field.getName()))
				.where(toString(scheme.getName(), "_id"), Comparation::Equal, id).finalize();
		return perform(query) != maxOf<size_t>();
	} else {
		if (field.transform(scheme, const_cast<Value &>(d))) {
			auto &arrf = static_cast<const storage::FieldArray *>(field.getSlot())->tfield;
			if (!d.empty()) {
				auto vals = query.insert(toString(scheme.getName(), "_f_", field.getName()))
						.fields(toString(scheme.getName(), "_id"), "data").values();
				for (auto &it : d.asArray()) {
					vals.values(id, Binder::DataField {it, arrf.isDataLayout()});
				}
				vals.onConflictDoNothing().finalize();
				return perform(query) != maxOf<size_t>();
			}
		}
	}
	return false;
}

bool Handle::insertIntoRefSet(ExecQuery &query, const Scheme &scheme, int64_t id, const Field &field, const Vector<int64_t> &ids) {
	auto fScheme = field.getForeignScheme();
	if (!ids.empty() && fScheme) {
		auto vals = query.insert(toString(scheme.getName(), "_f_", field.getName()))
				.fields(toString(scheme.getName(), "_id"), toString(fScheme->getName(), "_id")).values();
		for (auto &it : ids) {
			vals.values(id, it);
		}
		vals.onConflictDoNothing().finalize();
		perform(query);
		return true;
	}
	return false;
}

bool Handle::patchArray(const Scheme &scheme, uint64_t oid, const Field &field, data::Value &d) {
	ExecQuery query;
	if (!d.isNull()) {
		return insertIntoArray(query, scheme, oid, field, d);
	}
	return false;
}

bool Handle::patchRefSet(const Scheme &scheme, uint64_t oid, const Field &field, const Vector<int64_t> &objsToAdd) {
	ExecQuery query;
	return insertIntoRefSet(query, scheme, oid, field, objsToAdd);
}

bool Handle::cleanupRefSet(const Scheme &scheme, uint64_t oid, const Field &field, const Vector<int64_t> &ids) {
	ExecQuery query;
	auto objField = static_cast<const storage::FieldObject *>(field.getSlot());
	auto fScheme = objField->scheme;
	if (!ids.empty() && fScheme) {
		if (objField->onRemove == RemovePolicy::Reference) {
			auto w = query.remove(toString(scheme.getName(), "_f_", field.getName()))
					.where(toString(scheme.getName(), "_id"), Comparation::Equal, oid);
			w.parenthesis(Operator::And, [&] (ExecQuery::WhereBegin &wh) {
				auto whi = wh.where();
				for (auto &it : ids) {
					whi.where(Operator::Or, toString(fScheme->getName(), "_id"), Comparation::Equal, it);
				}
			}).finalize();
			perform(query);
			return true;
		} else if (objField->onRemove == RemovePolicy::StrongReference) {
			auto w = query.remove(fScheme->getName()).where();
			for (auto &it : ids) {
				w.where(Operator::Or, "__oid", Comparation::Equal, it);
			}
			w.finalize();
			perform(query);
			return true;
		}
	}
	return false;
}

Vector<int64_t> Handle::performQueryListForIds(const QueryList &list, size_t count) {
	Vector<int64_t> ret;
	ExecQuery query;
	query.writeQueryList(list, true, count);
	query.finalize();

	auto res = select(query);
	for (auto it : res) {
		ret.push_back(it.toInteger(0));
	}

	return ret;
}

data::Value Handle::performQueryList(const QueryList &list, size_t count, bool forUpdate, const Field *f) {
	ExecQuery query;
	if (!f) {
		query.writeQueryList(list, false, count);
		if (forUpdate) {
			query << "FOR UPDATE";
		}
		query.finalize();
		return select(*list.getScheme(), query);
	} else if (f->isFile()) {
		query.writeQueryFile(list, f);
		query.finalize();

		auto fileScheme = Server(apr::pool::server()).getFileScheme();
		return select(*fileScheme, query).getValue(0);
	} else if (f->getType() == Type::Array) {
		query.writeQueryArray(list, f);
		query.finalize();
		return select(*f, query);
	}

	return data::Value();
}

NS_SA_EXT_END(pg)