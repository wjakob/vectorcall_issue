#define TRIGGER_BUG 0

#include <Python.h>

/*
 * This testcase creates two types:
 *
 * - 'Meta', a metaclass that overrides 'tp_call' and also provides a vector call implementation
 * - 'Class', a class constructed via the metaclass 'Meta'
 *
 * The two constructors of 'Meta' output a message so that we can see which one is being called.
 */

/* Cached type object of 'Class' */
static PyTypeObject *class_tp = NULL;

static PyObject *meta_tp_call(PyObject *self, PyObject *args, PyObject *kwargs) {
    printf("Constructing using tp_call\n");
    return PyType_GenericAlloc(class_tp, 0);
}

static PyObject *meta_vectorcall(PyObject *callable, PyObject *const *args, size_t nargsf, PyObject *kwnames) {
    printf("Constructing using vector call\n");
    return PyType_GenericAlloc(class_tp, 0);
}

PyMemberDef meta_members [] = {
#if TRIGGER_BUG
    { "__vectorcalloffset__", Py_T_PYSSIZET, 0, Py_READONLY | Py_RELATIVE_OFFSET },
#else
    { "__vectorcalloffset__", Py_T_PYSSIZET, sizeof(PyHeapTypeObject), Py_READONLY },
#endif
    { NULL }
};

static PyType_Slot meta_slots[] = {
    { Py_tp_call, (void *) meta_tp_call },
    { Py_tp_members, (void *) meta_members },
    { Py_tp_base , (void *) &PyType_Type },
    { 0, NULL }
};

static PyType_Slot class_slots[] = {
    { 0, NULL }
};

static PyType_Spec meta_spec = {
    .name = "vectorcall_issue.Meta",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .slots = meta_slots,
    .basicsize = -(Py_ssize_t) sizeof(void *)
};

static PyType_Spec class_spec = {
    .name = "vectorcall_issue.Class",
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = class_slots
};

static PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "vectorcall_issue",
    .m_doc = "Reproducer for an issue involving type vector calls",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_vectorcall_issue(void)
{
    PyObject *m = PyModule_Create(&module);
    if (m == NULL)
        return NULL;

    PyObject *meta = PyType_FromMetaclass(NULL, m, &meta_spec, NULL);
    if (PyModule_AddObject(m, "Meta", meta) < 0) {
        Py_DECREF(meta);
        Py_DECREF(m);
        return NULL;
    }

    PyObject *class_ = PyType_FromMetaclass((PyTypeObject *) meta, m, &class_spec, NULL);
    class_tp = (PyTypeObject *) class_;
    if (PyModule_AddObject(m, "Class", class_) < 0) {
        Py_DECREF(class_);
        Py_DECREF(m);
        return NULL;
    }

    void **ptr = (void **) PyObject_GetTypeData(class_, (PyTypeObject *) meta);
    *ptr = (void *) meta_vectorcall;

    return m;
}
