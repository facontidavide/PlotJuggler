/****************************************************************************
** Meta object code from reading C++ file 'DockAreaTitleBar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/Qt-Advanced-Docking/include/Qads/DockAreaTitleBar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockAreaTitleBar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ads__CDockAreaTitleBar_t {
    QByteArrayData data[13];
    char stringdata0[198];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ads__CDockAreaTitleBar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ads__CDockAreaTitleBar_t qt_meta_stringdata_ads__CDockAreaTitleBar = {
    {
QT_MOC_LITERAL(0, 0, 22), // "ads::CDockAreaTitleBar"
QT_MOC_LITERAL(1, 23, 13), // "tabBarClicked"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 5), // "index"
QT_MOC_LITERAL(4, 44, 21), // "onTabsMenuAboutToShow"
QT_MOC_LITERAL(5, 66, 20), // "onCloseButtonClicked"
QT_MOC_LITERAL(6, 87, 21), // "onUndockButtonClicked"
QT_MOC_LITERAL(7, 109, 25), // "onTabsMenuActionTriggered"
QT_MOC_LITERAL(8, 135, 8), // "QAction*"
QT_MOC_LITERAL(9, 144, 6), // "Action"
QT_MOC_LITERAL(10, 151, 19), // "onCurrentTabChanged"
QT_MOC_LITERAL(11, 171, 5), // "Index"
QT_MOC_LITERAL(12, 177, 20) // "markTabsMenuOutdated"

    },
    "ads::CDockAreaTitleBar\0tabBarClicked\0"
    "\0index\0onTabsMenuAboutToShow\0"
    "onCloseButtonClicked\0onUndockButtonClicked\0"
    "onTabsMenuActionTriggered\0QAction*\0"
    "Action\0onCurrentTabChanged\0Index\0"
    "markTabsMenuOutdated"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ads__CDockAreaTitleBar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   52,    2, 0x08 /* Private */,
       5,    0,   53,    2, 0x08 /* Private */,
       6,    0,   54,    2, 0x08 /* Private */,
       7,    1,   55,    2, 0x08 /* Private */,
      10,    1,   58,    2, 0x08 /* Private */,
      12,    0,   61,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void,

       0        // eod
};

void ads::CDockAreaTitleBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CDockAreaTitleBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->tabBarClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->onTabsMenuAboutToShow(); break;
        case 2: _t->onCloseButtonClicked(); break;
        case 3: _t->onUndockButtonClicked(); break;
        case 4: _t->onTabsMenuActionTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 5: _t->onCurrentTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->markTabsMenuOutdated(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CDockAreaTitleBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CDockAreaTitleBar::tabBarClicked)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ads::CDockAreaTitleBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_ads__CDockAreaTitleBar.data,
    qt_meta_data_ads__CDockAreaTitleBar,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ads::CDockAreaTitleBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ads::CDockAreaTitleBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ads__CDockAreaTitleBar.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ads::CDockAreaTitleBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ads::CDockAreaTitleBar::tabBarClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
