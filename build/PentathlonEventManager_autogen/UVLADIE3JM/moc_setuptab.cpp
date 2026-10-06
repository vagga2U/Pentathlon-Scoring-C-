/****************************************************************************
** Meta object code from reading C++ file 'setuptab.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/setuptab.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'setuptab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.3. It"
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
struct qt_meta_stringdata_CLASSSetupTabENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSSetupTabENDCLASS = QtMocHelpers::stringData(
    "SetupTab",
    "eventDetailsSubmitted",
    "",
    "EventDetails",
    "details",
    "importEntriesRequested",
    "csvPath",
    "manualCompetitorSubmitted",
    "Competitor",
    "competitor",
    "exportCsvRequested",
    "clearAllRequested",
    "chooseLogo",
    "chooseCsv",
    "submitDetails",
    "submitManualCompetitor"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSetupTabENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,    1 /* Public */,
       5,    1,   71,    2, 0x06,    3 /* Public */,
       7,    1,   74,    2, 0x06,    5 /* Public */,
      10,    0,   77,    2, 0x06,    7 /* Public */,
      11,    0,   78,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    0,   79,    2, 0x08,    9 /* Private */,
      13,    0,   80,    2, 0x08,   10 /* Private */,
      14,    0,   81,    2, 0x08,   11 /* Private */,
      15,    0,   82,    2, 0x08,   12 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SetupTab::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSSetupTabENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSetupTabENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSetupTabENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SetupTab, std::true_type>,
        // method 'eventDetailsSubmitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const EventDetails &, std::false_type>,
        // method 'importEntriesRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'manualCompetitorSubmitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Competitor &, std::false_type>,
        // method 'exportCsvRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearAllRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chooseLogo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chooseCsv'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'submitDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'submitManualCompetitor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SetupTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SetupTab *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->eventDetailsSubmitted((*reinterpret_cast< std::add_pointer_t<EventDetails>>(_a[1]))); break;
        case 1: _t->importEntriesRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->manualCompetitorSubmitted((*reinterpret_cast< std::add_pointer_t<Competitor>>(_a[1]))); break;
        case 3: _t->exportCsvRequested(); break;
        case 4: _t->clearAllRequested(); break;
        case 5: _t->chooseLogo(); break;
        case 6: _t->chooseCsv(); break;
        case 7: _t->submitDetails(); break;
        case 8: _t->submitManualCompetitor(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SetupTab::*)(const EventDetails & );
            if (_t _q_method = &SetupTab::eventDetailsSubmitted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const QString & );
            if (_t _q_method = &SetupTab::importEntriesRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const Competitor & );
            if (_t _q_method = &SetupTab::manualCompetitorSubmitted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)();
            if (_t _q_method = &SetupTab::exportCsvRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)();
            if (_t _q_method = &SetupTab::clearAllRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *SetupTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SetupTab::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSetupTabENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SetupTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void SetupTab::eventDetailsSubmitted(const EventDetails & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SetupTab::importEntriesRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SetupTab::manualCompetitorSubmitted(const Competitor & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SetupTab::exportCsvRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SetupTab::clearAllRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
