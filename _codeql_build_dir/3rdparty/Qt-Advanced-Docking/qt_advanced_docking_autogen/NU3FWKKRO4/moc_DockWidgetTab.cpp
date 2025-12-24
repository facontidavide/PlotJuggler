/****************************************************************************
** Meta object code from reading C++ file 'DockWidgetTab.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/DockWidgetTab.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockWidgetTab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CDockWidgetTab_t {
    QByteArrayData data[15];
    char stringdata0[177];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CDockWidgetTab_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CDockWidgetTab_t qt_meta_stringdata_ads__CDockWidgetTab = {
    {
QT_MOC_LITERAL(0, 0, 19), // "ads::CDockWidgetTab"
QT_MOC_LITERAL(1, 20, 16), // "activeTabChanged"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 7), // "clicked"
QT_MOC_LITERAL(4, 46, 14), // "closeRequested"
QT_MOC_LITERAL(5, 61, 23), // "closeOtherTabsRequested"
QT_MOC_LITERAL(6, 85, 5), // "moved"
QT_MOC_LITERAL(7, 91, 9), // "GlobalPos"
QT_MOC_LITERAL(8, 101, 13), // "elidedChanged"
QT_MOC_LITERAL(9, 115, 6), // "elided"
QT_MOC_LITERAL(10, 122, 16), // "detachDockWidget"
QT_MOC_LITERAL(11, 139, 10), // "setVisible"
QT_MOC_LITERAL(12, 150, 7), // "visible"
QT_MOC_LITERAL(13, 158, 9), // "activeTab"
QT_MOC_LITERAL(14, 168, 8) // "iconSize"

    },
    "ads::CDockWidgetTab\0activeTabChanged\0"
    "\0clicked\0closeRequested\0closeOtherTabsRequested\0"
    "moved\0GlobalPos\0elidedChanged\0elided\0"
    "detachDockWidget\0setVisible\0visible\0"
    "activeTab\0iconSize"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CDockWidgetTab[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       2,   68, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    0,   56,    2, 0x06 /* Public */,
       5,    0,   57,    2, 0x06 /* Public */,
       6,    1,   58,    2, 0x06 /* Public */,
       8,    1,   61,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    0,   64,    2, 0x08 /* Private */,
      11,    1,   65,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,    7,
    QMetaType::Void, QMetaType::Bool,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   12,

 // properties: name, type, flags
      13, QMetaType::Bool, 0x00495103,
      14, QMetaType::QSize, 0x00095103,

 // properties: notify_signal_id
       0,
       0,

       0        // eod
};

void ads::CDockWidgetTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CDockWidgetTab *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->activeTabChanged(); break;
        case 1: _t->clicked(); break;
        case 2: _t->closeRequested(); break;
        case 3: _t->closeOtherTabsRequested(); break;
        case 4: _t->moved((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 5: _t->elidedChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->detachDockWidget(); break;
        case 7: _t->setVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CDockWidgetTab::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::activeTabChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CDockWidgetTab::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::clicked)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CDockWidgetTab::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::closeRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CDockWidgetTab::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::closeOtherTabsRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CDockWidgetTab::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::moved)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CDockWidgetTab::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockWidgetTab::elidedChanged)) {
                *result = 5;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<CDockWidgetTab *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isActiveTab(); break;
        case 1: *reinterpret_cast< QSize*>(_v) = _t->iconSize(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<CDockWidgetTab *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setActiveTab(*reinterpret_cast< bool*>(_v)); break;
        case 1: _t->setIconSize(*reinterpret_cast< QSize*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject ads::CDockWidgetTab::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_ads__CDockWidgetTab.data,
    qt_meta_data_ads__CDockWidgetTab,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CDockWidgetTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CDockWidgetTab::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CDockWidgetTab.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ads::CDockWidgetTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ads::CDockWidgetTab::activeTabChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ads::CDockWidgetTab::clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ads::CDockWidgetTab::closeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ads::CDockWidgetTab::closeOtherTabsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ads::CDockWidgetTab::moved(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ads::CDockWidgetTab::elidedChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
