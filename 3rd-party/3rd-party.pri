
lessThan(QT_MAJOR_VERSION, 5): {
    DEPENDPATH += 3rd-party/qjson4
    INCLUDEPATH += 3rd-party/qjson4

    HEADERS += QJsonArray.h        \
               QJsonDocument.h     \
               QJsonObject.h       \
               QJsonParseError.h   \
               QJsonValue.h        \
               QJsonValueRef.h     \
               QJsonParser.h       \
               QJsonRoot.h         \

    SOURCES += QJsonArray.cpp      \
               QJsonDocument.cpp   \
               QJsonObject.cpp     \
               QJsonParseError.cpp \
               QJsonValue.cpp      \
               QJsonValueRef.cpp   \
               QJsonParser.cpp     \
}
