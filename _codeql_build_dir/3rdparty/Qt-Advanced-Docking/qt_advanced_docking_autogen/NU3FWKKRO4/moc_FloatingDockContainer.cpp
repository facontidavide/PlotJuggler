/****************************************************************************
** Meta object code from reading C++ file 'FloatingDockContainer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/FloatingDockContainer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FloatingDockContainer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CFloatingDockContainer_t {
    QByteArrayData data[5];
    char stringdata0[86];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CFloatingDockContainer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CFloatingDockContainer_t qt_meta_stringdata_ads__CFloatingDockContainer = {
    {
QT_MOC_LITERAL(0, 0, 27), // "ads::CFloatingDockContainer"
QT_MOC_LITERAL(1, 28, 25), // "onDockAreasAddedOrRemoved"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 24), // "onDockAreaCurrentChanged"
QT_MOC_LITERAL(4, 80, 5) // "Index"

    },
    "ads::CFloatingDockContainer\0"
    "onDockAreasAddedOrRemoved\0\0"
    "onDockAreaCurrentChanged\0Index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CFloatingDockContainer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x08 /* Private */,
       3,    1,   25,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,

       0        // eod
};

void ads::CFloatingDockContainer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CFloatingDockContainer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onDockAreasAddedOrRemoved(); break;
        case 1: _t->onDockAreaCurrentChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ads::CFloatingDockContainer::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_ads__CFloatingDockContainer.data,
    qt_meta_data_ads__CFloatingDockContainer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CFloatingDockContainer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CFloatingDockContainer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CFloatingDockContainer.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IFloatingWidget"))
        return static_cast< IFloatingWidget*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int ads::CFloatingDockContainer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
