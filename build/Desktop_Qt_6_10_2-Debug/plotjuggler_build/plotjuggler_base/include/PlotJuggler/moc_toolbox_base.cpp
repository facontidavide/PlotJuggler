/****************************************************************************
** Meta object code from reading C++ file 'toolbox_base.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../../PlotJuggler/plotjuggler_base/include/PlotJuggler/toolbox_base.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'toolbox_base.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PJ__ToolboxPlugin_t {
    QByteArrayData data[12];
    char stringdata0[130];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PJ__ToolboxPlugin_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PJ__ToolboxPlugin_t qt_meta_stringdata_PJ__ToolboxPlugin = {
    {
QT_MOC_LITERAL(0, 0, 17), // "PJ::ToolboxPlugin"
QT_MOC_LITERAL(1, 18, 11), // "plotCreated"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 11), // "std::string"
QT_MOC_LITERAL(4, 43, 9), // "plot_name"
QT_MOC_LITERAL(5, 53, 9), // "is_custom"
QT_MOC_LITERAL(6, 63, 10), // "importData"
QT_MOC_LITERAL(7, 74, 15), // "PlotDataMapRef&"
QT_MOC_LITERAL(8, 90, 8), // "new_data"
QT_MOC_LITERAL(9, 99, 10), // "remove_old"
QT_MOC_LITERAL(10, 110, 6), // "closed"
QT_MOC_LITERAL(11, 117, 12) // "onShowWidget"

    },
    "PJ::ToolboxPlugin\0plotCreated\0\0"
    "std::string\0plot_name\0is_custom\0"
    "importData\0PlotDataMapRef&\0new_data\0"
    "remove_old\0closed\0onShowWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PJ__ToolboxPlugin[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,
       1,    1,   44,    2, 0x26 /* Public | MethodCloned */,
       6,    2,   47,    2, 0x06 /* Public */,
      10,    0,   52,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   53,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool,    4,    5,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 7, QMetaType::Bool,    8,    9,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Bool,

       0        // eod
};

void PJ::ToolboxPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ToolboxPlugin *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->plotCreated((*reinterpret_cast< std::string(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: _t->plotCreated((*reinterpret_cast< std::string(*)>(_a[1]))); break;
        case 2: _t->importData((*reinterpret_cast< PlotDataMapRef(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: _t->closed(); break;
        case 4: { bool _r = _t->onShowWidget();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ToolboxPlugin::*)(std::string , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ToolboxPlugin::plotCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ToolboxPlugin::*)(PlotDataMapRef & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ToolboxPlugin::importData)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ToolboxPlugin::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ToolboxPlugin::closed)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PJ::ToolboxPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<PlotJugglerPlugin::staticMetaObject>(),
    qt_meta_stringdata_PJ__ToolboxPlugin.data,
    qt_meta_data_PJ__ToolboxPlugin,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PJ::ToolboxPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PJ::ToolboxPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PJ__ToolboxPlugin.stringdata0))
        return static_cast<void*>(this);
    return PlotJugglerPlugin::qt_metacast(_clname);
}

int PJ::ToolboxPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PlotJugglerPlugin::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void PJ::ToolboxPlugin::plotCreated(std::string _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void PJ::ToolboxPlugin::importData(PlotDataMapRef & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PJ::ToolboxPlugin::closed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
