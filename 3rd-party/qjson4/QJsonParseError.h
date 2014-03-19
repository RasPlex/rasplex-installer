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

#ifndef QJSON_PARSEERROR_H_
#define QJSON_PARSEERROR_H_

#include <QtCore/QtGlobal>

#if QT_VERSION >= 0x050000
#include <QtCore/QJsonParseError>
#else

#include <QtCore/QString>

class QJsonParseError {
public:
	enum ParseError {
		NoError 			   = 0,
		UnterminatedObject     = 1,
		MissingNameSeparator   = 2,
		UnterminatedArray	   = 3,
		MissingValueSeparator  = 4,
		IllegalValue		   = 5,
		TerminationByNumber    = 6,
		IllegalNumber		   = 7,
		IllegalEscapeSequence  = 8,
		IllegalUTF8String	   = 9,
		UnterminatedString     = 10,
		MissingObject		   = 11,
		DeepNesting 		   = 12,
		DocumentTooLarge       = 13
	};

public:
	QString errorString() const;

public:
	ParseError error;
	int        offset;
};

#endif

#endif
