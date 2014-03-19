
TEMPLATE = app
TARGET   = test
DEPENDPATH  += .
INCLUDEPATH += .

# Input
HEADERS += QJsonArray.h        \
           QJsonDocument.h     \
           QJsonObject.h       \
           QJsonParseError.h   \
           QJsonValue.h        \
           QJsonValueRef.h     \
		   QJsonParser.h       \
		   QJsonRoot.h         \

SOURCES += main.cpp            \
           QJsonArray.cpp      \
           QJsonDocument.cpp   \
           QJsonObject.cpp     \
           QJsonParseError.cpp \
           QJsonValue.cpp      \
           QJsonValueRef.cpp   \
		   QJsonParser.cpp     \

CONFIG(debug, debug|release) {
	OBJECTS_DIR = $${OUT_PWD}/.debug/obj
	MOC_DIR     = $${OUT_PWD}/.debug/moc
	RCC_DIR     = $${OUT_PWD}/.debug/rcc
	UI_DIR      = $${OUT_PWD}/.debug/uic
}

CONFIG(release, debug|release) {
	OBJECTS_DIR = $${OUT_PWD}/.release/obj
	MOC_DIR     = $${OUT_PWD}/.release/moc
	RCC_DIR     = $${OUT_PWD}/.release/rcc
	UI_DIR      = $${OUT_PWD}/.release/uic
}
