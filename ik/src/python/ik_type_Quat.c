#include "ik/python/ik_type_Quat.h"
#include "ik/python/ik_type_Vec3.h"
#include "ik/quat.inl"
#include "structmember.h"

/* Macro and helper that convert PyObject obj to a C double and store
   the value in dbl.  If conversion to double raises an exception, obj is
   set to NULL, and the function invoking this macro returns NULL.  If
   obj is not of float or int type, Py_NotImplemented is incref'ed,
   stored in obj, and returned from the function invoking this macro.
*/
#define CONVERT_TO_DOUBLE(obj, dbl)                     \
    if (PyFloat_Check(obj))                             \
        dbl = PyFloat_AS_DOUBLE(obj);                   \
    else if (convert_to_double(&(obj), &(dbl)) < 0)     \
        return obj;

static int
convert_to_double(PyObject **v, double *dbl)
{
    PyObject *obj = *v;

    if (PyLong_Check(obj)) {
        *dbl = PyLong_AsDouble(obj);
        if (*dbl == -1.0 && PyErr_Occurred()) {
            *v = NULL;
            return -1;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Operation not supported");
        *v = NULL;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
static void
Quat_dealloc(PyObject* myself)
{
    Py_TYPE(myself)->tp_free(myself);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    ik_Quat* self;
    (void)args; (void)kwds;

    self = (ik_Quat*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->quatref = &self->quat;
    ik_quat_set_identity(self->quatref->f);

    return (PyObject*)self;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_set(PyObject* myself, PyObject* arg);
static int
Quat_init(PyObject* self, PyObject* args, PyObject* kwds)
{
    (void)kwds;

    assert(PyTuple_CheckExact(args));
    if (PyTuple_GET_SIZE(args) > 0)
    {
        PyObject* result = Quat_set(self, args);
        if (result == NULL)
            return -1;
        Py_DECREF(result);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_set_identity(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_set_identity(self->quatref->f);
    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_set(PyObject* myself, PyObject* args)
{
    double x, y, z, w;
    ik_Quat* self = (ik_Quat*)myself;
    assert(PyTuple_CheckExact(args));

    /* Can be single vector or single quaternion */
    if (PyTuple_GET_SIZE(args) == 1)
    {
        PyObject* arg = PyTuple_GET_ITEM(args, 0);
        if (ik_Quat_CheckExact(arg))
        {
            ik_Quat* other = (ik_Quat*)arg;
            ik_quat_copy(self->quatref->f, other->quatref->f);
            Py_RETURN_NONE;
        }
        else if (ik_Vec3_CheckExact(arg))
        {
            ik_Vec3* v = (ik_Vec3*)arg;
            ik_quat_angle_of(self->quatref->f, v->vecref->f);
            Py_RETURN_NONE;
        }

        arg = PySequence_Tuple(arg);
        if (arg == NULL)
            return NULL;
        if (PyTuple_GET_SIZE(arg) == 4)
        {
            if (!PyArg_ParseTuple(arg, "dddd", &x, &y, &z, &w))
            {
                Py_DECREF(arg);
                return NULL;
            }
            ik_quat_set(self->quatref->f, x, y, z, w);
        }
        else
        {
            ikreal v[3];
            if (!PyArg_ParseTuple(arg, "ddd", &x, &y, &z))
            {
                Py_DECREF(arg);
                return NULL;
            }
            ik_vec3_set(v, x, y, z);
            ik_quat_angle_of(self->quatref->f, v);
        }

        Py_DECREF(arg);
        Py_RETURN_NONE;
    }
    /* Can be a vector+angle or two vectors */
    else if (PyTuple_GET_SIZE(args) == 2)
    {
        ikreal v1[3];
        PyObject* arg1 = PyTuple_GET_ITEM(args, 0);
        PyObject* arg2 = PyTuple_GET_ITEM(args, 1);

        if (ik_Vec3_CheckExact(arg1))
        {
            ik_vec3_copy(v1, ((ik_Vec3*)arg1)->vecref->f);
        }
        else
        {
            arg1 = PySequence_Tuple(arg1);
            if (arg1 == NULL)
                return NULL;
            if (!PyArg_ParseTuple(arg1, "ddd", &x, &y, &z))
            {
                Py_DECREF(arg1);
                return NULL;
            }
            ik_vec3_set(v1, x, y, z);
            Py_DECREF(arg1);
        }

        if (ik_Vec3_CheckExact(arg2))
        {
            ik_quat_angle_between(self->quatref->f, v1, ((ik_Vec3*)arg2)->vecref->f);
            Py_RETURN_NONE;
        }
        else if (PyFloat_Check(arg2))
        {
            double angle = PyFloat_AS_DOUBLE(arg2);
            ik_quat_set_axis_angle(self->quatref->f, v1[0], v1[1], v1[2], angle);
            Py_RETURN_NONE;
        }
        else if (PyLong_Check(arg2))
        {
            double angle = PyLong_AsDouble(arg2);
            if (angle == -1.0 && PyErr_Occurred())
                return NULL;
            ik_quat_set_axis_angle(self->quatref->f, v1[0], v1[1], v1[2], angle);
            Py_RETURN_NONE;
        }
        else
        {
            ikreal v2[3];
            arg2 = PySequence_Tuple(arg2);
            if (arg2 == NULL)
                return NULL;
            if (!PyArg_ParseTuple(arg2, "ddd", &x, &y, &z))
            {
                Py_DECREF(arg2);
                return NULL;
            }
            ik_vec3_set(v2, x, y, z);
            ik_quat_angle_between(self->quatref->f, v1, v2);
            Py_DECREF(arg2);
            Py_RETURN_NONE;
        }
    }
    /* Has to be a tuple of 3 values representing a vector */
    else if (PyTuple_GET_SIZE(args) == 3)
    {
        ikreal v[3];
        if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z))
            return NULL;
        ik_vec3_set(v, x, y, z);
        ik_quat_angle_of(self->quatref->f, v);
        Py_RETURN_NONE;
    }

    if (!PyArg_ParseTuple(args, "dddd", &x, &y, &z, &w))
        return NULL;
    ik_quat_set(self->quatref->f, x, y, z, w);
    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_dup(PyObject* other)
{
    PyObject* copy;

    assert(ik_Quat_CheckExact(other));

    PyObject* args = PyTuple_New(1);
    if (args == NULL)
        return NULL;

    Py_INCREF(other);
    PyTuple_SET_ITEM(args, 0, other);  /* ref steal */

    copy = PyObject_CallObject((PyObject*)&ik_QuatType, args);
    Py_DECREF(args);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_add(PyObject* self, PyObject* arg)
{
    if (ik_Quat_CheckExact(arg))
    {
        ik_quat_add_quat(((ik_Quat*)self)->quatref->f, ((ik_Quat*)arg)->quatref->f);
        return Py_INCREF(self), self;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Operation not supported");
        return NULL;
    }
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_sub(PyObject* self, PyObject* arg)
{
    if (ik_Quat_CheckExact(arg))
    {
        ik_quat_sub_quat(((ik_Quat*)self)->quatref->f, ((ik_Quat*)arg)->quatref->f);
        return Py_INCREF(self), self;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Operation not supported");
        return NULL;
    }
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_mul(PyObject* self, PyObject* arg)
{
    if (ik_Quat_CheckExact(arg))
    {
        ik_quat_mul_quat(((ik_Quat*)self)->quatref->f, ((ik_Quat*)arg)->quatref->f);
    }
    else
    {
        double v;
        CONVERT_TO_DOUBLE(arg, v)
        ik_quat_mul_scalar(((ik_Quat*)self)->quatref->f, v);
    }

    return Py_INCREF(self), self;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_div(PyObject* self, PyObject* arg)
{
    if (ik_Quat_CheckExact(arg))
    {
        ik_quat_mul_quat_conj(((ik_Quat*)self)->quatref->f, ((ik_Quat*)arg)->quatref->f);
    }
    else
    {
        double v;
        CONVERT_TO_DOUBLE(arg, v)
        if (v == 0.0)
        {
            PyErr_SetString(PyExc_ZeroDivisionError, "Quaternion division by zero");
            return NULL;
        }
        ik_quat_mul_scalar(((ik_Quat*)self)->quatref->f, 1.0 / v);
    }

    return Py_INCREF(self), self;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_pow(PyObject* self, PyObject* arg, PyObject* mod)
{
    if (mod != Py_None)
    {
        PyErr_SetString(PyExc_TypeError, "pow() 3rd argument is not supported for quaternions");
        return NULL;
    }

    if (PyLong_Check(arg))
    {
        union ik_quat qorig;
        int do_normalize;
        long int pow = PyLong_AS_LONG(arg);
        ikreal* q = ((ik_Quat*)self)->quatref->f;

        if (pow < 1)
        {
            PyErr_SetString(PyExc_ValueError, "Can only raise quaternion to a positive integer");
            return NULL;
        }

        if (pow == 1)
            return Py_INCREF(self), self;

        ik_quat_copy(qorig.f, q);
        pow--;
        do_normalize = 10;
        while (pow--)
        {
            ik_quat_mul_quat_nn(q, qorig.f);
            if (do_normalize-- == 0)
            {
                do_normalize = 10;
            }
        }
        ik_quat_normalize(q);

        return Py_INCREF(self), self;
    }

    PyErr_SetString(PyExc_TypeError, "Power must be an integer value");
    return NULL;
}
static PyObject*
Quat_inplace_pow_meth(PyObject* self, PyObject* pow)
{
    /* required to be compatible with PyCFunction */
    return Quat_inplace_pow(self, pow, Py_None);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_conj(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_conj(self->quatref->f);
    return Py_INCREF(myself), myself;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_negate(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_negate(self->quatref->f);
    return Py_INCREF(myself), myself;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_invert(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_invert(self->quatref->f);
    return Py_INCREF(myself), myself;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_inplace_normalize(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_normalize(self->quatref->f);
    return Py_INCREF(myself), myself;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_add(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_add(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_sub(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_sub(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_mul(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_mul(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_div(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_div(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_pow(PyObject* myself, PyObject* arg, PyObject* mod)
{
    PyObject* op_result;
    PyObject* copy;

    if ((copy = Quat_dup(myself)) == NULL)
        return NULL;
    if ((op_result = Quat_inplace_pow(copy, arg, mod)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_mag(PyObject* myself)
{
    ik_Quat* self = (ik_Quat*)myself;
    return PyFloat_FromDouble(ik_quat_mag(self->quatref->f));
}
static PyObject*
Quat_mag_meth(PyObject* myself, PyObject* none)
{
    /* Required to be compatible with PyCFunction */
    (void)none;
    return Quat_mag(myself);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_dot(PyObject* myself, PyObject* arg)
{
    if (!ik_Quat_CheckExact(arg))
    {
        PyErr_SetString(PyExc_TypeError, "Expected a ik.Quat() type");
        return NULL;
    }

    return PyFloat_FromDouble(
        ik_quat_dot(((ik_Quat*)myself)->quatref->f, ((ik_Quat*)arg)->quatref->f));
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_conj(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_conj(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_negate(PyObject* myself)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_negate(copy, NULL)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}
static PyObject*
Quat_negate_meth(PyObject* myself, PyObject* none)
{
    /* Required to be compatible with PyCFunction */
    (void)none;
    return Quat_negate(myself);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_quat(PyObject* myself)
{
    return Quat_dup(myself);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_invert(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_invert(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_normalize(PyObject* myself, PyObject* arg)
{
    PyObject* op_result;
    PyObject* copy = Quat_dup(myself);
    if (copy == NULL)
        return NULL;
    if ((op_result = Quat_inplace_normalize(copy, arg)) == NULL)
        return Py_DECREF(copy), NULL;
    Py_DECREF(op_result);
    return copy;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_ensure_positive_sign(PyObject* myself, PyObject* arg)
{
    ik_Quat* self = (ik_Quat*)myself;
    (void)arg;
    ik_quat_ensure_positive_sign(self->quatref->f);
    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Quat_repr(PyObject* myself)
{
    PyObject *w, *x, *y, *z;
    PyObject* str = NULL;
    ik_Quat* self = (ik_Quat*)myself;

    if ((x = PyFloat_FromDouble(self->quatref->q.x)) == NULL) goto x_failed;
    if ((y = PyFloat_FromDouble(self->quatref->q.y)) == NULL) goto y_failed;
    if ((z = PyFloat_FromDouble(self->quatref->q.z)) == NULL) goto z_failed;
    if ((w = PyFloat_FromDouble(self->quatref->q.w)) == NULL) goto w_failed;
    str = PyUnicode_FromFormat("ik.Quat(%S, %S, %S, %S)", x, y, z, w);

               Py_DECREF(w);
    w_failed : Py_DECREF(z);
    z_failed : Py_DECREF(y);
    y_failed : Py_DECREF(x);
    x_failed : return str;
}

/* ------------------------------------------------------------------------- */
#define GETSET_COMPONENT(comp)                                                \
    static PyObject*                                                          \
    Quat_get##comp(PyObject* myself, void* closure)                           \
    {                                                                         \
        ik_Quat* self = (ik_Quat*)myself;                                     \
        (void)closure;                                                        \
        return PyFloat_FromDouble(self->quatref->q.comp);                     \
    }                                                                         \
    static int                                                                \
    Quat_set##comp(PyObject* myself, PyObject* value, void* closure)          \
    {                                                                         \
        ik_Quat* self = (ik_Quat*)myself;                                     \
        (void)closure;                                                        \
                                                                              \
        if (PyFloat_Check(value))                                             \
            self->quatref->q.comp = PyFloat_AS_DOUBLE(value);                 \
        else if (PyLong_Check(value))                                         \
        {                                                                     \
            double v = PyLong_AsDouble(value);                                \
            if (v == -1.0 && PyErr_Occurred())                                \
                return -1;                                                    \
            self->quatref->q.comp = v;                                        \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            PyErr_SetString(PyExc_TypeError, "Expected a value type");        \
            return -1;                                                        \
        }                                                                     \
                                                                              \
        return 0;                                                             \
    }
GETSET_COMPONENT(x)
GETSET_COMPONENT(y)
GETSET_COMPONENT(z)
GETSET_COMPONENT(w)
#undef GETSET_COMPONENT

/* ------------------------------------------------------------------------- */
static PyNumberMethods Quat_as_number = {
    .nb_add = Quat_add,
    .nb_subtract = Quat_sub,
    .nb_multiply = Quat_mul,
    .nb_true_divide = Quat_div,
    .nb_power = Quat_pow,
    .nb_negative = Quat_negate,
    .nb_positive = Quat_quat,
    .nb_absolute = Quat_mag,
    .nb_inplace_add = Quat_inplace_add,
    .nb_inplace_subtract = Quat_inplace_sub,
    .nb_inplace_multiply = Quat_inplace_mul,
    .nb_inplace_true_divide = Quat_inplace_div,
    .nb_inplace_power = Quat_inplace_pow
};

/* ------------------------------------------------------------------------- */
static PyMethodDef Quat_methods[] = {
    {"set_identity",         Quat_set_identity,         METH_NOARGS,   "Sets all components to 0.0"},
    {"set",                  Quat_set,                  METH_VARARGS,  "Copies components from another vector or tuple"},
    {"add",                  Quat_inplace_add,          METH_O,        "Adds another vector or scalar to this vector"},
    {"sub",                  Quat_inplace_sub,          METH_O,        "Adds another vector or scalar to this vector"},
    {"mul",                  Quat_inplace_mul,          METH_O,        "Adds another vector or scalar to this vector"},
    {"div",                  Quat_inplace_div,          METH_O,        "Adds another vector or scalar to this vector"},
    {"pow",                  Quat_inplace_pow_meth,     METH_O,        "Adds another vector or scalar to this vector"},
    {"mag",                  Quat_mag_meth,             METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"dot",                  Quat_dot,                  METH_O,        "Adds another vector or scalar to this vector"},
    {"conjugate",            Quat_inplace_conj,         METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"conjugated",           Quat_conj,                 METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"negate",               Quat_inplace_negate,       METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"negated",              Quat_negate_meth,          METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"invert",               Quat_inplace_invert,       METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"inverted",             Quat_invert,               METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"normalize",            Quat_inplace_normalize,    METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"normalized",           Quat_normalize,            METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {"ensure_positive_sign", Quat_ensure_positive_sign, METH_NOARGS,   "Adds another vector or scalar to this vector"},
    {NULL}
};

/* ------------------------------------------------------------------------- */
static PyGetSetDef Quat_getsetters[] = {
    {"x", Quat_getx, Quat_setx, "X component"},
    {"y", Quat_gety, Quat_sety, "Y component"},
    {"z", Quat_getz, Quat_setz, "Z component"},
    {"w", Quat_getw, Quat_setw, "W component"},
    {NULL}
};

/* ------------------------------------------------------------------------- */
PyDoc_STRVAR(QUAT_DOC, "");
PyTypeObject ik_QuatType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ik.Quat",
    .tp_basicsize = sizeof(ik_Quat),
    .tp_dealloc = Quat_dealloc,
    .tp_repr = Quat_repr,
    .tp_as_number = &Quat_as_number,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = QUAT_DOC,
    .tp_methods = Quat_methods,
    .tp_getset = Quat_getsetters,
    .tp_init = Quat_init,
    .tp_new = Quat_new
};

/* ------------------------------------------------------------------------- */
int
init_ik_QuatType(void)
{
    if (PyType_Ready(&ik_QuatType) < 0)
        return -1;
    return 0;
}

/* ------------------------------------------------------------------------- */
ik_Quat*
quat_ik_to_python(ikreal q[4])
{
    ik_Quat* qpy = (ik_Quat*)PyObject_CallObject((PyObject*)&ik_QuatType, NULL);
    if (qpy == NULL)
        return NULL;
    ik_quat_copy(qpy->quatref->f, q);
    return qpy;
}
