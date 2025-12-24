/****************************************************************************
** Meta object code from reading C++ file 'DockAreaTitleBar_p.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/DockAreaTitleBar_p.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockAreaTitleBar_p.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CTitleBarButton_t {
    QByteArrayData data[1];
    char stringdata0[21];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CTitleBarButton_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CTitleBarButton_t qt_meta_stringdata_ads__CTitleBarButton = {
    {
QT_MOC_LITERAL(0, 0, 20) // "ads::CTitleBarButton"

    },
    "ads::CTitleBarButton"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CTitleBarButton[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void ads::CTitleBarButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject ads::CTitleBarButton::staticMetaObject = { {
    QMetaObject::SuperData::link<tTitleBarButton::staticMetaObject>(),
    qt_meta_stringdata_ads__CTitleBarButton.data,
    qt_meta_data_ads__CTitleBarButton,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CTitleBarButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CTitleBarButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CTitleBarButton.stringdata0))
        return static_cast<void*>(this);
    return tTitleBarButton::qt_metacast(_clname);
}

int ads::CTitleBarButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = tTitleBarButton::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ads__CSpacerWidget_t {
    QByteArrayData data[1];
    char stringdata0[19];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CSpacerWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CSpacerWidget_t qt_meta_stringdata_ads__CSpacerWidget = {
    {
QT_MOC_LITERAL(0, 0, 18) // "ads::CSpacerWidget"

    },
    "ads::CSpacerWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CSpacerWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void ads::CSpacerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject ads::CSpacerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ads__CSpacerWidget.data,
    qt_meta_data_ads__CSpacerWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CSpacerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CSpacerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CSpacerWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ads::CSpacerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
