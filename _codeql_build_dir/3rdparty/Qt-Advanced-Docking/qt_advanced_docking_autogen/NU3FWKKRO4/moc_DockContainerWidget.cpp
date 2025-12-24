/****************************************************************************
** Meta object code from reading C++ file 'DockContainerWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/DockContainerWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockContainerWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CDockContainerWidget_t {
    QByteArrayData data[8];
    char stringdata0[115];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CDockContainerWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CDockContainerWidget_t qt_meta_stringdata_ads__CDockContainerWidget = {
    {
QT_MOC_LITERAL(0, 0, 25), // "ads::CDockContainerWidget"
QT_MOC_LITERAL(1, 26, 14), // "dockAreasAdded"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 16), // "dockAreasRemoved"
QT_MOC_LITERAL(4, 59, 19), // "dockAreaViewToggled"
QT_MOC_LITERAL(5, 79, 21), // "ads::CDockAreaWidget*"
QT_MOC_LITERAL(6, 101, 8), // "DockArea"
QT_MOC_LITERAL(7, 110, 4) // "Open"

    },
    "ads::CDockContainerWidget\0dockAreasAdded\0"
    "\0dockAreasRemoved\0dockAreaViewToggled\0"
    "ads::CDockAreaWidget*\0DockArea\0Open"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CDockContainerWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    0,   30,    2, 0x06 /* Public */,
       4,    2,   31,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5, QMetaType::Bool,    6,    7,

       0        // eod
};

void ads::CDockContainerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CDockContainerWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dockAreasAdded(); break;
        case 1: _t->dockAreasRemoved(); break;
        case 2: _t->dockAreaViewToggled((*reinterpret_cast< ads::CDockAreaWidget*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CDockContainerWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockContainerWidget::dockAreasAdded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CDockContainerWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockContainerWidget::dockAreasRemoved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CDockContainerWidget::*)(ads::CDockAreaWidget * , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockContainerWidget::dockAreaViewToggled)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ads::CDockContainerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_ads__CDockContainerWidget.data,
    qt_meta_data_ads__CDockContainerWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CDockContainerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CDockContainerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CDockContainerWidget.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ads::CDockContainerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ads::CDockContainerWidget::dockAreasAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ads::CDockContainerWidget::dockAreasRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ads::CDockContainerWidget::dockAreaViewToggled(ads::CDockAreaWidget * _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
