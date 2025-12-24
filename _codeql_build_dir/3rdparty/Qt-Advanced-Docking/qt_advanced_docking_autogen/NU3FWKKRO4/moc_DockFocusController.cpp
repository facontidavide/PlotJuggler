/****************************************************************************
** Meta object code from reading C++ file 'DockFocusController.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/DockFocusController.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockFocusController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CDockFocusController_t {
    QByteArrayData data[14];
    char stringdata0[203];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CDockFocusController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CDockFocusController_t qt_meta_stringdata_ads__CDockFocusController = {
    {
QT_MOC_LITERAL(0, 0, 25), // "ads::CDockFocusController"
QT_MOC_LITERAL(1, 26, 25), // "onApplicationFocusChanged"
QT_MOC_LITERAL(2, 52, 0), // ""
QT_MOC_LITERAL(3, 53, 8), // "QWidget*"
QT_MOC_LITERAL(4, 62, 3), // "old"
QT_MOC_LITERAL(5, 66, 3), // "now"
QT_MOC_LITERAL(6, 70, 28), // "onFocusedDockAreaViewToggled"
QT_MOC_LITERAL(7, 99, 4), // "Open"
QT_MOC_LITERAL(8, 104, 15), // "onStateRestored"
QT_MOC_LITERAL(9, 120, 29), // "onDockWidgetVisibilityChanged"
QT_MOC_LITERAL(10, 150, 7), // "Visible"
QT_MOC_LITERAL(11, 158, 20), // "setDockWidgetFocused"
QT_MOC_LITERAL(12, 179, 12), // "CDockWidget*"
QT_MOC_LITERAL(13, 192, 10) // "focusedNow"

    },
    "ads::CDockFocusController\0"
    "onApplicationFocusChanged\0\0QWidget*\0"
    "old\0now\0onFocusedDockAreaViewToggled\0"
    "Open\0onStateRestored\0onDockWidgetVisibilityChanged\0"
    "Visible\0setDockWidgetFocused\0CDockWidget*\0"
    "focusedNow"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CDockFocusController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x08 /* Private */,
       6,    1,   44,    2, 0x08 /* Private */,
       8,    0,   47,    2, 0x08 /* Private */,
       9,    1,   48,    2, 0x08 /* Private */,
      11,    1,   51,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    4,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void, 0x80000000 | 12,   13,

       0        // eod
};

void ads::CDockFocusController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CDockFocusController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onApplicationFocusChanged((*reinterpret_cast< QWidget*(*)>(_a[1])),(*reinterpret_cast< QWidget*(*)>(_a[2]))); break;
        case 1: _t->onFocusedDockAreaViewToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->onStateRestored(); break;
        case 3: _t->onDockWidgetVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->setDockWidgetFocused((*reinterpret_cast< CDockWidget*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QWidget* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CDockWidget* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ads::CDockFocusController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ads__CDockFocusController.data,
    qt_meta_data_ads__CDockFocusController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CDockFocusController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CDockFocusController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CDockFocusController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ads::CDockFocusController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
