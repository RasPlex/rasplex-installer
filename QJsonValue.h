/*
Copyright (C) 2014 - 2014 Evan Teran
                          eteran@alum.rit.edu

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef QJSON_VALUE_H_
#define QJSON_VALUE_H_

#include <QtCore/QtGlobal>

#if QT_VERSION >= 0x050000
#include <QtCore/QJsonValue>
#else

class QString;

#include <QtCore/QVariant>

class QJsonRoot;
class QJsonArray;
class QJsonObject;

class QJsonValue {
public:
	enum Type {
		Null      = 0x0,
		Bool      = 0x1,
		Double    = 0x2,
		String    = 0x3,
		Array     = 0x4,
		Object    = 0x5,
		Undefined = 0x80
	};

public:
	QJsonValue(Type type = Null);
	QJsonValue(bool b);
	QJsonValue(double n);
	QJsonValue(const QString &s);
	QJsonValue(QLatin1String s);
	QJsonValue(const QJsonArray &a);
	QJsonValue(const QJsonObject &o);
	QJsonValue(const QJsonValue &other);
	QJsonValue(int n);
	~QJsonValue();

private:
	// to protect against incorrect usage due to passing a const char *
	QJsonValue(const void *);

public:
	QJsonValue &operator=(const QJsonValue &other);

public:
	bool operator!=(const QJsonValue &other) const;
	bool operator==(const QJsonValue &other) const;

public:
	bool isArray() const;
	bool isBool() const;
	bool isDouble() const;
	bool isNull() const;
	bool isObject() const;
	bool isString() const;
	bool isUndefined() const;

public:
	QJsonArray toArray(const QJsonArray &defaultValue) const;
	QJsonArray toArray() const;
	bool toBool(bool defaultValue = false) const;
	double toDouble(double defaultValue = 0) const;
	QJsonObject toObject(const QJsonObject &defaultValue) const;
	QJsonObject toObject() const;
	QString toString(const QString &defaultValue = QString()) const;
	QVariant toVariant() const;

public:
	Type type() const;

public:
	static QJsonValue fromVariant(const QVariant &variant);

private:
	void swap(QJsonValue &other);

private:
	Type type_;
	union {
		bool       b;
		double     n;
		QString   *s;
		QJsonRoot *r; // OJsonObject or QJsonArray
	} value_;
};

#endif

#endif
