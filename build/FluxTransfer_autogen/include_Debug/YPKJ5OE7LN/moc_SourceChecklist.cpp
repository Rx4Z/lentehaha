/****************************************************************************
** Meta object code from reading C++ file 'SourceChecklist.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/ui/SourceChecklist.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SourceChecklist.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSSourceChecklistENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSSourceChecklistENDCLASS = QtMocHelpers::stringData(
    "SourceChecklist",
    "sourcesChanged",
    "",
    "onItemChanged",
    "QTreeWidgetItem*",
    "item",
    "column",
    "onSizeReady",
    "path",
    "bytes",
    "onSelectAll",
    "onClearAll"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSSourceChecklistENDCLASS_t {
    uint offsetsAndSizes[24];
    char stringdata0[16];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[17];
    char stringdata5[5];
    char stringdata6[7];
    char stringdata7[12];
    char stringdata8[5];
    char stringdata9[6];
    char stringdata10[12];
    char stringdata11[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSSourceChecklistENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSSourceChecklistENDCLASS_t qt_meta_stringdata_CLASSSourceChecklistENDCLASS = {
    {
        QT_MOC_LITERAL(0, 15),  // "SourceChecklist"
        QT_MOC_LITERAL(16, 14),  // "sourcesChanged"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 13),  // "onItemChanged"
        QT_MOC_LITERAL(46, 16),  // "QTreeWidgetItem*"
        QT_MOC_LITERAL(63, 4),  // "item"
        QT_MOC_LITERAL(68, 6),  // "column"
        QT_MOC_LITERAL(75, 11),  // "onSizeReady"
        QT_MOC_LITERAL(87, 4),  // "path"
        QT_MOC_LITERAL(92, 5),  // "bytes"
        QT_MOC_LITERAL(98, 11),  // "onSelectAll"
        QT_MOC_LITERAL(110, 10)   // "onClearAll"
    },
    "SourceChecklist",
    "sourcesChanged",
    "",
    "onItemChanged",
    "QTreeWidgetItem*",
    "item",
    "column",
    "onSizeReady",
    "path",
    "bytes",
    "onSelectAll",
    "onClearAll"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSourceChecklistENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    2,   45,    2, 0x08,    2 /* Private */,
       7,    2,   50,    2, 0x08,    5 /* Private */,
      10,    0,   55,    2, 0x08,    8 /* Private */,
      11,    0,   56,    2, 0x08,    9 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 4, QMetaType::Int,    5,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    8,    9,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SourceChecklist::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSSourceChecklistENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSourceChecklistENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSourceChecklistENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SourceChecklist, std::true_type>,
        // method 'sourcesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onItemChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QTreeWidgetItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onSizeReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onSelectAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClearAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SourceChecklist::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SourceChecklist *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sourcesChanged(); break;
        case 1: _t->onItemChanged((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->onSizeReady((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 3: _t->onSelectAll(); break;
        case 4: _t->onClearAll(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SourceChecklist::*)();
            if (_t _q_method = &SourceChecklist::sourcesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *SourceChecklist::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SourceChecklist::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSourceChecklistENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SourceChecklist::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SourceChecklist::sourcesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
