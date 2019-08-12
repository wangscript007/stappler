/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STSqlQuery.h"
#include "STStorageScheme.h"
#include "STSqlHandle.h"

NS_DB_SQL_BEGIN

SqlQuery::SqlQuery(db::QueryInterface *iface) {
	binder.setInterface(iface);
}

void SqlQuery::clear() {
	stream.clear();
	binder.clear();
}

static inline bool SqlQuery_comparationIsValid(const Field &f, Comparation comp) {
	if (f.getType() == Type::Custom) {
		auto c = f.getSlot<FieldCustom>();
		return c->isComparationAllowed(comp);
	} else {
		return db::checkIfComparationIsValid(f.getType(), comp);
	}
}

static inline mem::StringView SqlQuery_getSoftLimitField(const db::Scheme &scheme, const db::Query &q, bool &hasAltLimit) {
	mem::StringView softLimitField;
	if (q.isSoftLimit()) {
		auto &field = q.getOrderField();
		auto f = scheme.getField(field);
		if (f) {
			softLimitField = f->getName();
			hasAltLimit = true;
		} else if (field == "__oid") {
			softLimitField = field;
		} else {
			messages::error("SqlQuery", "Invalid soft limit field", mem::Value(field));
			return mem::StringView();
		}
	}
	return softLimitField;
}

static inline auto SqlQuery_makeSoftLimitWith(SqlQuery &iquery, const db::Scheme &ischeme, const db::Query &iq,
		const mem::StringView &softLimitField, bool isSubField = false, const mem::StringView &lName = mem::StringView(), uint64_t oid = 0) {
	return [pquery = &iquery, pscheme = &ischeme, pq = &iq, isSubField, softLimitField, lName, oid] (SqlQuery::GenericQuery &subq) {
		SqlQuery &query = *pquery;
		const db::Scheme &scheme = *pscheme;
		const db::Query &q = *pq;

		auto sel = subq.select(SqlQuery::Field(scheme.getName(), "__oid"), SqlQuery::Field(scheme.getName(), softLimitField));
		auto s = isSubField ? sel.from(scheme.getName()).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
			q.where(SqlQuery::Field(scheme.getName(), "__oid"), Comparation::Equal, SqlQuery::Field("s", "id"));
		}) : sel.from(scheme.getName());

		if (auto &val = q.getSoftLimitValue()) {
			auto w = s.where(SqlQuery::Field(scheme.getName(), softLimitField),
					q.getOrdering() == Ordering::Ascending ? Comparation::GreatherThen : Comparation::LessThen, val.asInteger());
			if (!lName.empty()) {
				w.where(Operator::And, SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
			}
			query.writeWhere(w, Operator::And, scheme, q);
		} else if (q.hasSelect() || !lName.empty()) {
			auto w = lName.empty() ? s.where() : s.where(SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
			query.writeWhere(w, Operator::And, scheme, q);
		}

		query.writeOrdering(s, scheme, q);
	};
}

template <typename Clause>
static inline auto SqlQuery_makeWhereClause(SqlQuery &query, Clause &tmp, const db::Scheme &scheme, const db::Query &q,
		const mem::StringView &softLimitField, const mem::StringView &lName = mem::StringView(), uint64_t oid = 0) {
	bool isAsc = q.getOrdering() == Ordering::Ascending;
	if (q.hasSelect() || !softLimitField.empty() || !lName.empty()) {
		if (softLimitField == "__oid") {
			if (auto &val = q.getSoftLimitValue()) {
				auto w = tmp.where(SqlQuery::Field(scheme.getName(), softLimitField),
						isAsc ? Comparation::GreatherThen : Comparation::LessThen, val.asInteger());
				if (!lName.empty()) {
					w.where(Operator::And, SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
				}
				query.writeWhere(w, Operator::And,scheme, q);
			} else if (q.hasSelect() || !lName.empty()) {
				auto w = lName.empty() ? tmp.where() : tmp.where(SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
				query.writeWhere(w, Operator::And, scheme, q);
			}
		} else if (softLimitField.empty()) {
			if (q.hasSelect() || !lName.empty()) {
				auto whi = lName.empty() ? tmp.where() : tmp.where(SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
				query.writeWhere(whi, db::Operator::And, scheme, q);
			}
		} else {
			// write soft limit query like WHERE __oid IN (SELECT __oid FROM u) OR (field = (SELECT MAX(mtime) FROM u))
			tmp.where(SqlQuery::Field(scheme.getName(), "__oid"), Comparation::In, mem::Callback<void(SqlQuery::Select &)>([&] (SqlQuery::Select &subQ) {
				subQ.field(SqlQuery::Field("u", "__oid")).from("u").finalize();
			})).parenthesis(Operator::Or, [&] (SqlQuery::WhereBegin &whi) {
				auto w = whi.where(SqlQuery::Field(scheme.getName(), softLimitField), Comparation::Equal,
						mem::Callback<void(SqlQuery::Select &)>([&] (SqlQuery::Select &subQ) {
					subQ.aggregate(isAsc ? "MAX" : "MIN", SqlQuery::Field("u", softLimitField)).from("u").finalize();
				}));
				if (!lName.empty()) {
					w.where(Operator::And, SqlQuery::Field(scheme.getName(), lName), Comparation::Equal, oid);
				}
				query.writeWhere(w, Operator::And, scheme, q);
			});
		}
	}
}

bool SqlQuery::writeQuery(Worker &worker, const db::Scheme &scheme, const db::Query &q) {\
	bool hasAltLimit = false;
	mem::StringView softLimitField = SqlQuery_getSoftLimitField(scheme, q, hasAltLimit);

	auto sel = (hasAltLimit)
		? with("u", SqlQuery_makeSoftLimitWith(*this, scheme, q, softLimitField, false)).select()
		: select();
	auto s = writeSelectFrom(sel, worker, q);

	SqlQuery_makeWhereClause(*this, s, scheme, q, softLimitField);

	writeOrdering(s, scheme, q, hasAltLimit);
	if (q.isForUpdate()) { s.forUpdate(); }
	s.finalize();

	return true;
}

bool SqlQuery::writeQuery(Worker &w, const db::Scheme &scheme, uint64_t oid, const db::Field &f, const db::Query &q) {
	auto type = f.getType();
	auto fs = f.getForeignScheme();
	if (!fs) {
		return false;
	}

	mem::StringView lName;
	if (type == Type::Set && !f.isReference()) {
		if (auto l = w.scheme().getForeignLink(f)) {
			lName = l->getName();
		}
	}

	bool hasAltLimit = false;
	mem::StringView softLimitField = SqlQuery_getSoftLimitField(*fs, q, hasAltLimit);

	auto writeFields = [&] (Select &sel) {
		writeFullTextRank(sel, *fs, q);
		if (w.shouldIncludeAll()) {
			sel = sel.field(SqlQuery::Field(fs->getName(), "*"));
		} else {
			w.readFields(*fs, [&] (const mem::StringView &name, const db::Field *) {
				sel = sel.field(SqlQuery::Field(fs->getName(), name));
			});
		}
	};

	auto writeSelect = [&] () -> Select {
		if (type == Type::View || (type == Type::Set && f.isReference())) {
			auto wtmp = with("s", [&] (SqlQuery::GenericQuery &q) {
				q.select(SqlQuery::Distinct::Distinct, SqlQuery::Field(mem::toString(fs->getName(), "_id")).as("id"))
					.from(mem::toString(w.scheme().getName(), "_f_", f.getName(), (type == Type::View ? "_view" : "")))
					.where(mem::toString(w.scheme().getName(), "_id"), Comparation::Equal, oid);
			});

			if (hasAltLimit) {
				wtmp.with("u", SqlQuery_makeSoftLimitWith(*this, *fs, q, softLimitField, true, lName, oid));
			}

			return wtmp.select();
		} else {
			return (hasAltLimit)
				? with("u", SqlQuery_makeSoftLimitWith(*this, *fs, q, softLimitField, false, lName, oid)).select()
				: select();
		}
	};

	auto sel = writeSelect();
	writeFields(sel);

	auto tmp = (type == Type::View || (type == Type::Set && f.isReference()))
		? sel.from(fs->getName()).innerJoinOn("s", [&] (SqlQuery::WhereBegin &q) {
				q.where(SqlQuery::Field(fs->getName(), "__oid"), Comparation::Equal, SqlQuery::Field("s", "id"));
			})
		: sel.from(fs->getName());

	SqlQuery_makeWhereClause(*this, tmp, *fs, q, softLimitField, lName, oid);

	writeOrdering(tmp, *fs, q, hasAltLimit);
	if (q.isForUpdate()) { tmp.forUpdate(); }
	finalize();

	return true;
}

void SqlQuery::writeWhere(SqlQuery::SelectWhere &w, db::Operator op, const db::Scheme &scheme, const db::Query &q) {
	SqlQuery::WhereContinue iw(w.query, w.state);
	writeWhere(iw, op, scheme, q);
}

void SqlQuery::writeWhere(SqlQuery::WhereContinue &w, db::Operator op, const db::Scheme &scheme, const db::Query &q) {
	if (q.getSingleSelectId()) {
		w.where(op, "__oid", db::Comparation::Equal, q.getSingleSelectId());
	} else if (!q.getSelectIds().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : q.getSelectIds()) {
				whi.where(db::Operator::Or, SqlQuery::Field(scheme.getName(), "__oid"), db::Comparation::Equal, it);
			}
		});
	} else if (!q.getSelectAlias().empty()) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			for (auto &it : scheme.getFields()) {
				if (it.second.getType() == db::Type::Text && it.second.getTransform() == db::Transform::Alias) {
					whi.where(db::Operator::Or, SqlQuery::Field(scheme.getName(), it.first), db::Comparation::Equal, q.getSelectAlias());
				}
			}
		});
	} else if (q.getSelectList().size() > 0) {
		w.parenthesis(op, [&] (SqlQuery::WhereBegin &wh) {
			auto whi = wh.where();
			auto &fields = scheme.getFields();
			for (auto &it : q.getSelectList()) {
				auto f_it = fields.find(it.field);
				if (f_it != fields.end()) {
					auto type = f_it->second.getType();
					if (type == db::Type::FullTextView) {
						auto ftsQuery = getFullTextQuery(scheme, f_it->second, it);
						if (!ftsQuery.empty()) {
							whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field),
									db::Comparation::Includes, RawStringView{ftsQuery});
						}
					} else if ((f_it->second.isIndexed() && SqlQuery_comparationIsValid(f_it->second, it.compare))) {
						if (type == Type::Custom) {
							auto c = f_it->second.getSlot<FieldCustom>();
							c->writeQuery(scheme, whi, db::Operator::And, it.field, it.compare, it.value1, it.value2);
						} else {
							if (it.compare == Comparation::Equal && (type == Type::Integer || type == Type::Float || type == Type::Object) && it.value1.isArray()) {
								whi.parenthesis(db::Operator::And, [&] (WhereBegin &wb) {
									auto wwb = wb.where();
									for (auto &id : it.value1.asArray()) {
										wwb.where(db::Operator::Or, SqlQuery::Field(scheme.getName(), it.field), it.compare, id);
									}
								});
							} else {
								whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field), it.compare, it.value1, it.value2);
							}
						}
					}
				} else if (it.field == "__oid" && db::checkIfComparationIsValid(db::Type::Integer, it.compare)) {
					whi.where(db::Operator::And, SqlQuery::Field(scheme.getName(), it.field), it.compare, it.value1, it.value2);
				}
			}
		});
	}
}

void SqlQuery::writeOrdering(SqlQuery::SelectFrom &s, const db::Scheme &scheme, const db::Query &q, bool dropLimits) {
	if (q.hasOrder() || q.hasLimit() || q.hasOffset()) {
		auto ordering = q.getOrdering();
		mem::String orderField;
		mem::String schemeName = scheme.getName().str<mem::Interface>();
		if (q.hasOrder()) {
			if (auto f = scheme.getField(q.getOrderField())) {
				if (f->getType() == db::Type::FullTextView) {
					orderField = mem::toString("__ts_rank_", q.getOrderField());
					schemeName.clear();
				} else {
					orderField = q.getOrderField();
				}
			} else if (q.getOrderField() == "__oid") {
				orderField = q.getOrderField();
			} else {
				return;
			}
		} else if (q.getSelectList().size() == 1) {
			orderField = q.getSelectList().back().field;
			if (!scheme.getField(orderField)) {
				return;
			}
		} else {
			orderField = "__oid";
		}

		SelectOrder o = s.order(ordering, schemeName.empty() ? SqlQuery::Field(orderField) : SqlQuery::Field(scheme.getName(), orderField),
				ordering == db::Ordering::Descending ? stappler::sql::Nulls::Last : stappler::sql::Nulls::None);

		if (!dropLimits) {
			if (q.hasLimit() && q.hasOffset()) {
				o.limit(q.getLimitValue(), q.getOffsetValue());
			} else if (q.hasLimit()) {
				o.limit(q.getLimitValue());
			} else if (q.hasOffset()) {
				o.offset(q.getOffsetValue());
			}
		}
	}
}

void SqlQuery::writeQueryReqest(SqlQuery::SelectFrom &s, const db::QueryList::Item &item) {
	auto &q = item.query;
	if (!item.all && !item.query.empty()) {
		auto w = s.where();
		writeWhere(w, db::Operator::And, *item.scheme, q);
	}

	writeOrdering(s, *item.scheme, q);
}

static void SqlQuery_writeJoin(SqlQuery::SelectFrom &s, const mem::StringView &sqName, const mem::StringView &schemeName, const db::QueryList::Item &item) {
	s.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
		mem::StringView fieldName = item.ref
				? ( item.ref->getType() == db::Type::Set ? mem::StringView("__oid") : item.ref->getName() )
				: mem::StringView("__oid");
		w.where(SqlQuery::Field(schemeName, fieldName), db::Comparation::Equal, SqlQuery::Field(sqName, "id"));
	});
}

auto SqlQuery::writeSelectFrom(GenericQuery &q, const db::QueryList::Item &item, bool idOnly, const mem::StringView &schemeName, const mem::StringView &fieldName, bool isSimpleGet) -> SelectFrom {
	if (idOnly) {
		return q.select(SqlQuery::Field(schemeName, fieldName).as("id")).from(schemeName);
	}

	auto sel = q.select();
	writeFullTextRank(sel, *item.scheme, item.query);
	item.readFields([&] (const mem::StringView &name, const db::Field *) {
		sel = sel.field(SqlQuery::Field(schemeName, name));
	}, isSimpleGet);
	return sel.from(schemeName);
}

auto SqlQuery::writeSelectFrom(Select &sel, db::Worker &worker, const db::Query &q) -> SelectFrom {
	writeFullTextRank(sel, worker.scheme(), q);
	if (worker.shouldIncludeAll()) {
		sel = sel.field(SqlQuery::Field("*"));
	} else {
		worker.readFields(worker.scheme(), q, [&] (const mem::StringView &name, const db::Field *) {
			sel = sel.field(SqlQuery::Field(name));
		});
	}
	return sel.from(worker.scheme().getName());
}

void SqlQuery::writeQueryListItem(GenericQuery &q, const db::QueryList &list, size_t idx, bool idOnly, const db::Field *field, bool forSubquery) {
	auto &items = list.getItems();
	const db::QueryList::Item &item = items.at(idx);
	const db::Field *sourceField = nullptr;
	const db::FieldView *viewField = nullptr;
	mem::String refQueryTag;
	if (idx > 0) {
		sourceField = items.at(idx - 1).field;
	}

	if (idx > 0 && !item.ref && sourceField && sourceField->getType() != db::Type::Object) {
		mem::String prevSq = mem::toString("sq", idx - 1);
		const db::QueryList::Item &prevItem = items.at(idx - 1);

		if (sourceField->getType() == db::Type::View) {
			viewField = static_cast<const db::FieldView *>(sourceField->getSlot());
		}
		mem::String tname = viewField
				? mem::toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName(), "_view")
				: mem::toString(prevItem.scheme->getName(), "_f_", prevItem.field->getName());

		mem::String targetIdField = mem::toString(item.scheme->getName(), "_id");
		mem::String sourceIdField = mem::toString(prevItem.scheme->getName(), "_id");

		if (idOnly && item.query.empty()) { // optimize id-only empty request
			q.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname)
					.innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(sourceIdField, stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
			return;
		}

		refQueryTag = mem::toString("sq", idx, "_ref");
		q.with(refQueryTag, [&] (GenericQuery &sq) {
			sq.select(SqlQuery::Field(targetIdField).as("id"))
					.from(tname).innerJoinOn(prevSq, [&] (WhereBegin &w) {
				w.where(SqlQuery::Field(sourceIdField), stappler::sql::Comparation::Equal, SqlQuery::Field(prevSq, "id"));
			});
		});
	}

	const db::Field * f = field?field:item.field;

	mem::StringView schemeName(item.scheme->getName());
	mem::StringView fieldName( (f && (
		(f->getType() == db::Type::Object && (forSubquery || !idOnly || idx + 1 == items.size()))
		|| f->isFile()))
			? f->getName()
			: mem::StringView("__oid") );

	auto s = writeSelectFrom(q, item, idOnly, schemeName, fieldName, list.hasFlag(db::QueryList::SimpleGet));
	if (idx > 0) {
		if (refQueryTag.empty()) {
			SqlQuery_writeJoin(s, mem::toString("sq", idx - 1), item.scheme->getName(), item);
		} else {
			SqlQuery_writeJoin(s, refQueryTag, item.scheme->getName(), item);
		}
	}
	writeQueryReqest(s, item);
}

void SqlQuery::writeQueryList(const db::QueryList &list, bool idOnly, size_t count) {
	const db::QueryList::Item &item = list.getItems().back();
	if (item.query.hasDelta() && list.isDeltaApplicable()) {
		if (!list.isView()) {
			writeQueryDelta(*item.scheme, stappler::Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		} else {
			writeQueryViewDelta(list, stappler::Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
		}
		return;
	} else if (item.query.hasDelta()) {
		messages::error("Query", "Delta is not applicable for this query");
	}

	auto &items = list.getItems();
	count = std::min(items.size(), count);

	GenericQuery q(this);
	size_t i = 0;
	if (count > 0) {
		for (; i < count - 1; ++ i) {
			q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
				writeQueryListItem(sq, list, i, true, nullptr, true);
			});
		}
	}

	writeQueryListItem(q, list, i, idOnly, nullptr, false);
}

void SqlQuery::writeQueryFile(const db::QueryList &list, const db::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count - 1; ++ i) {
		q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	q.with(mem::toString("sq", count - 1), [&] (GenericQuery &sq) {
		writeQueryListItem(sq, list, count - 1, true, field);
	});

	auto fileScheme = internals::getFileScheme();
	q.select(SqlQuery::Field::all(fileScheme->getName()))
			.from(fileScheme->getName())
			.innerJoinOn(mem::toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field(fileScheme->getName(), "__oid"), db::Comparation::Equal, SqlQuery::Field(mem::toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryArray(const db::QueryList &list, const db::Field *field) {
	auto &items = list.getItems();
	auto count = items.size();
	GenericQuery q(this);
	for (size_t i = 0; i < count; ++ i) {
		q.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
			writeQueryListItem(sq, list, i, true);
		});
	}

	auto scheme = items.back().scheme;

	q.select(SqlQuery::Field("t", "data"))
			.from(SqlQuery::Field(mem::toString(scheme->getName(), "_f_", field->getName())).as("t"))
			.innerJoinOn(mem::toString("sq", count - 1), [&] (SqlQuery::WhereBegin &w) {
		w.where(SqlQuery::Field("t", mem::toString(scheme->getName(), "_id")), db::Comparation::Equal, SqlQuery::Field(mem::toString("sq", count - 1), "id"));
	});
}

void SqlQuery::writeQueryDelta(const db::Scheme &scheme, const stappler::Time &time, const mem::Set<const db::Field *> &fields, bool idOnly) {
	GenericQuery q(this);
	auto s = q.with("d", [&] (SqlQuery::GenericQuery &sq) {
		sq.select()
			.aggregate("max", Field("time").as("time"))
			.aggregate("max", Field("action").as("action"))
			.field("object")
			.from(SqlHandle::getNameForDelta(scheme))
			.where("time", db::Comparation::GreatherThen, time.toMicroseconds())
			.group("object")
			.order(db::Ordering::Descending, "time");
	}).select();
	if (!idOnly) {
		db::QueryList::readFields(scheme, fields, [&] (const mem::StringView &name, const db::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("d", "action").as("__d_action"), Field("d", "time").as("__d_time"), Field("d", "object").as("__d_object"))
		.from(Field(scheme.getName()).as("t"))
		.rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("d", "object"), db::Comparation::Equal, Field("t", "__oid"));
	});
}

void SqlQuery::writeQueryViewDelta(const db::QueryList &list, const stappler::Time &time, const mem::Set<const db::Field *> &fields, bool idOnly) {
	auto &items = list.getItems();
	const db::QueryList::Item &item = items.back();
	auto prevScheme = items.size() > 1 ? items.at(items.size() - 2).scheme : nullptr;
	auto viewField = items.size() > 1 ? items.at(items.size() - 2).field : items.back().field;
	auto view = static_cast<const db::FieldView *>(viewField->getSlot());

	GenericQuery q(this);
	const db::Scheme &scheme = *item.scheme;
	mem::String deltaName = mem::toString(prevScheme->getName(), "_f_", view->name, "_delta");
	mem::String viewName = mem::toString(prevScheme->getName(), "_f_", view->name, "_view");
	auto s = q.with("dv", [&] (SqlQuery::GenericQuery &sq) {
		uint64_t id = 0;
		mem::String sqName;
		// optimize id-only
		if (items.size() != 2 || items.front().query.getSingleSelectId() == 0) {
			size_t i = 0;
			for (; i < items.size() - 1; ++ i) {
				sq.with(mem::toString("sq", i), [&] (GenericQuery &sq) {
					writeQueryListItem(sq, list, i, true);
				});
			}
			sqName = mem::toString("sq", i - 1);
		} else {
			id = items.front().query.getSingleSelectId();
		}

		sq.with("d", [&] (SqlQuery::GenericQuery &sq) {
			if (id) {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field("tag")
					.from(deltaName)
					.where(SqlQuery::Field("tag"), db::Comparation::Equal, id)
					.where(db::Operator::And, "time", db::Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field("tag");
			} else {
				sq.select()
					.aggregate("max", Field("time").as("time"))
					.field("object")
					.field(Field(sqName, "id").as("tag"))
					.from(deltaName)
					.innerJoinOn(sqName, [&] (SqlQuery::WhereBegin &w) {
						w.where(SqlQuery::Field(deltaName, "tag"), db::Comparation::Equal, SqlQuery::Field(sqName, "id"));
				})
					.where("time", db::Comparation::GreatherThen, time.toMicroseconds())
					.group("object").field(SqlQuery::Field(sqName, "id"));;
			}
		}).select().fields(Field("d", "time"), Field("d", "object"), Field("__vid"))
			.from(viewName).rightJoinOn("d", [&] (SqlQuery::WhereBegin &w) {
				w.where(SqlQuery::Field("d", "tag"), db::Comparation::Equal, SqlQuery::Field(viewName, mem::toString(prevScheme->getName(), "_id")))
						.where(db::Operator::And, SqlQuery::Field("d", "object"), db::Comparation::Equal, SqlQuery::Field(viewName, mem::toString(scheme.getName(), "_id")));
			});
	}).select();

	if (!idOnly) {
		db::QueryList::readFields(scheme, fields, [&] (const mem::StringView &name, const db::Field *field) {
			s.field(SqlQuery::Field("t", name));
		});
	} else {
		s.field(Field("t", "__oid"));
	}
	s.fields(Field("dv", "time").as("__d_time"), Field("dv", "object").as("__d_object"), Field("dv", "__vid"))
		.from(Field(view->scheme->getName()).as("t"))
		.rightJoinOn("dv", [&] (SqlQuery::WhereBegin &w) {
			w.where(Field("dv", "object"), db::Comparation::Equal, Field("t", "__oid"));
	});
}

const mem::StringStream &SqlQuery::getQuery() const {
	return stream;
}

db::QueryInterface * SqlQuery::getInterface() const {
	return binder.getInterface();
}

void SqlQuery::writeFullTextRank(Select &sel, const db::Scheme &scheme, const db::Query &q) {
	for (auto &it : q.getSelectList()) {
		if (auto f = scheme.getField(it.field)) {
			if (f->getType() == db::Type::FullTextView) {

				auto ftsQuery = getFullTextQuery(scheme, *f, it);
				if (!ftsQuery.empty()) {
					sel.query->writeBind(db::Binder::FullTextRank{scheme.getName(), f, ftsQuery});
					sel.query->getStream() << " AS __ts_rank_" << it.field << ", ";
				}
			}
		}
	}
}

mem::StringView SqlQuery::getFullTextQuery(const db::Scheme &scheme, const db::Field &f, const db::Query::Select &it) {
	if (f.getType() != Type::FullTextView) {
		return mem::StringView();
	}

	mem::String key = mem::toString(scheme.getName(), ":", f.getName());

	auto fit = _fulltextQueries.find(key);
	if (fit != _fulltextQueries.end()) {
		return fit->second;
	}

	if (!it.searchData.empty()) {
		mem::StringStream queryFrom;
		for (auto &searchIt : it.searchData) {
			if (!queryFrom.empty()) {
				queryFrom  << " && ";
			}
			binder.writeBind(queryFrom, searchIt);
		}
		return _fulltextQueries.emplace(std::move(key), queryFrom.str()).first->second;
	} else if (it.value1) {
		auto d = f.getSlot<db::FieldFullTextView>();
		auto q = d->parseQuery(it.value1);
		if (!q.empty()) {
			mem::StringStream queryFrom;
			for (auto &searchIt : q) {
				if (!queryFrom.empty()) {
					queryFrom  << " && ";
				}
				binder.writeBind(queryFrom, searchIt);
			}
			return _fulltextQueries.emplace(std::move(key), queryFrom.str()).first->second;
		}
	}

	return mem::StringView();
}

NS_DB_SQL_END