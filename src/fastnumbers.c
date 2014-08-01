/*
 * C implementation of the python library fastnumbers.
 *
 * Author: Seth M. Morton, July 30, 2014
 */

#include <Python.h>
#include "fast_conversions.h"
#include "version.h"

/* 
 * Convenience function to convert a string to a character array.
 * If unsuccessful, raise a TypeError.
 * A return value of NULL means an error occurred.
 */
inline char* convert_string(PyObject *input) {
    char* str;
#if PY_MAJOR_VERSION < 3
    PyObject *temp_bytes = NULL;
#endif
    /* Try Bytes (Python2 str). */
    if (PyBytes_Check(input)) {
        str = PyBytes_AS_STRING(input);        
    /* Try Unicode. */
    } else if (PyUnicode_Check(input)) {
        /* Now convert this unicode object to a char*. */
        /* There is a convenient way in newer Python versions. */
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 3
        str = PyUnicode_AsUTF8(input);
#else
        temp_bytes = PyUnicode_AsASCIIString(input);
        if (temp_bytes != NULL) {
            str = PyBytes_AS_STRING(temp_bytes);
            Py_DECREF(temp_bytes);
        }
        else
            return NULL;
#endif
    /* If none of the above, not a string type. */
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "expected str(), float(), or int() argument");
        return NULL;
    }
    /* There was an error with conversion. */
    if (str == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "expected str(), float(), or int() argument");
        return NULL;
    }
    return str;
}

/* Convenience to check for numeric types. */
#if PY_MAJOR_VERSION >= 3
#define ANYNUM(x) PyLong_Check(x) || PyFloat_Check(x)
#else
#define ANYNUM(x) PyInt_Check(x) || PyLong_Check(x) || PyFloat_Check(x)
#endif


const char safe_real_docstring[] = 
"Convert input to an *int* or *float* if possible, return the input otherwise.\n"
"\n"
"Convert input to an *int* or *float* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid input, although\n"
"a TypeError error will be raised for types the *float* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str*, *int*, or *float*."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def safe_real(input):\n"
"    ...     try:\n"
"    ...         a = float(x)\n"
"    ...     except ValueError:\n"
"    ...         return x\n"
"    ...     else:\n"
"    ...         b = int(a)\n"
"    ...         return b if a == b else b\n"
"    ...\n"
"\n"
"The main difference is that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data.\n"
"\n";
static PyObject *
fastnumbers_safe_real(PyObject *self, PyObject *args)
{
    PyObject *input = NULL, *result = NULL;
    PyObject *intresult = NULL, *isint = NULL;
    char *str;
    double maxsize = 9007199254740992;  /* 2^53 */
    double dresult;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now as-is. */
    if (ANYNUM(input))
        return Py_BuildValue("O", input);

    /* Attempt conversion of the (string) input object to a float. */
#if PY_MAJOR_VERSION >= 3
    result = PyFloat_FromString(input);
#else
    result = PyFloat_FromString(input, NULL);
#endif

    /* If unsuccessful, check to make sure input is a string. */
    /* If so, return that string.  If not, raise the error. */
    if (result == NULL) {
        if (PyUnicode_Check(input) || PyBytes_Check(input)) { 
            PyErr_Clear();
            return Py_BuildValue("O", input);
        }
        else
            return NULL;
    }

    /* Check if this float can be represented as an integer. */
    /* If so, return as an int object, otherwise as a float object. */
    isint = PyObject_CallMethod(result, "is_integer", NULL);
    if (PyObject_IsTrue(isint)) {
        /* If the float is over 2^53, re-read */
        /* the input string because we have lost some precision. */
        dresult = PyFloat_AsDouble(result);
        if (dresult > maxsize) {
            str = convert_string(input);
            if (str == NULL) return NULL;
#if PY_MAJOR_VERSION >= 3
            intresult = PyLong_FromString(str, NULL, 10);
#else
            intresult = PyInt_FromString(str, NULL, 10);
#endif
        } else {
            intresult = PyNumber_Int(result);
        }
        Py_DECREF(result);
        Py_DECREF(isint);
        return intresult;            
    }
    else {
        Py_DECREF(isint);
        return result;
    }
}


const char safe_float_docstring[] = 
"Convert input to a *float* if possible, return the input otherwise.\n"
"\n"
"Convert input to a *float* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid input, although\n"
"a TypeError error will be raised for types the *float* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str* or *float*."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def safe_float(input):\n"
"    ...    try:\n"
"    ...        return float(input)\n"
"    ...    except ValueError:\n"
"    ...        return input\n"
"    ...\n"
"\n"
"The main difference is that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data.\n"
"\n";
static PyObject *
fastnumbers_safe_float(PyObject *self, PyObject *args)
{
    PyObject *input = NULL, *result = NULL;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now. */
    if (ANYNUM(input))
        return PyNumber_Float(input);

    /* Attempt conversion of the (string) object to a float. */
#if PY_MAJOR_VERSION >= 3
    result = PyFloat_FromString(input);
#else
    result = PyFloat_FromString(input, NULL);
#endif

    /* If unsuccessful, check to make sure input is a string. */
    /* If so, return that string.  If not, raise the error. */
    if (result == NULL) {
        if (PyUnicode_Check(input) || PyBytes_Check(input)) { 
            PyErr_Clear();
            return Py_BuildValue("O", input);
        }
        else
            return NULL;
    }

    /* If successful, return this float object. */
    return result;
}


const char safe_int_docstring[] = 
"Convert input to a *int* if possible, return the input otherwise.\n"
"\n"
"Convert input to a *int* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid string input, although\n"
"a TypeError error will be raised for types the *int* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str* or *int* (or *long* on Python2 if the input was *long*)."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def safe_int(input):\n"
"    ...    try:\n"
"    ...        return int(input)\n"
"    ...    except ValueError:\n"
"    ...        return input\n"
"    ...\n"
"\n"
"The main difference is that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data.\n"
"\n";
static PyObject *
fastnumbers_safe_int(PyObject *self, PyObject *args)
{
    PyObject *input = NULL, *result = NULL;
    char *str;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now. */
    if (ANYNUM(input))
#if PY_MAJOR_VERSION >= 3
        return PyNumber_Long(input);
#else
        return PyNumber_Int(input);
#endif

    /* Attempt to convert to char*. */
    str = convert_string(input);
    if (str == NULL) return NULL;

    /* Attempt the conversion to a int. */
#if PY_MAJOR_VERSION >= 3
    result = PyLong_FromString(str, NULL, 10);
#else
    result = PyInt_FromString(str, NULL, 10);
#endif

    /* If unsuccessful, clear error stack and return input as-is */
    if (result == NULL) { PyErr_Clear(); return Py_BuildValue("O", input); }

    /* Otherwise, return the int object */
    return result;
}


const char fast_real_docstring[] = 
"Convert input to an *int* or *float* if possible, return the input otherwise.\n"
"\n"
"Convert input to an *int* or *float* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid input, although\n"
"a TypeError error will be raised for types the *float* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str*, *int*, or *float*."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def safe_real(input):\n"
"    ...     try:\n"
"    ...         a = float(x)\n"
"    ...     except ValueError:\n"
"    ...         return x\n"
"    ...     else:\n"
"    ...         b = int(a)\n"
"    ...         return b if a == b else b\n"
"    ...\n"
"\n"
"The main differences are that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data, and a fast implementation of *atof* is used to\n"
"convert strings to a *float* which for large exponents may be inaccurate\n"
"in the 12th decimal place (you should be mindful of this when converting\n"
"very large or small numbers, especially if a given string integer is\n"
"larger than can be contained in the C-type 'long'.)"
"\n";
static PyObject *
fastnumbers_fast_real(PyObject *self, PyObject *args)
{
    PyObject *input = NULL;
    char *str;
    double maxsize = 9007199254740992;  /* 2^53 */
    double result;
    long intresult;
    bool error;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now. */
    if (ANYNUM(input))
        return Py_BuildValue("O", input);

    /* Attempt to convert to char*. */
    str = convert_string(input);
    if (str == NULL) return NULL;

    /* Attempt to convert to a float */
    result = fast_atof(str, &error);

    /* If there was an error, return input as-is. */
    if (error) return Py_BuildValue("O", input);

    /* Make the integer version of the input. */
    /* If the value is greater than 2^53, */
    /* re-read because some precision was lost. */
    if (result > maxsize) {
        intresult = fast_atoi(str, &error);
        if (error) intresult = 0;  /* Set to 0 on error. */
    }
    else
        intresult = (long) result;

    /* Otherwise, return the float or int object */
    if (result == (double) intresult)
        return Py_BuildValue("l", intresult);
    else
        return Py_BuildValue("d", result);
}


const char fast_float_docstring[] = 
"Convert a string to a *float* if possible, return the input otherwise.\n"
"\n"
"Convert input to a *float* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid input, although\n"
"a TypeError error will be raised for types the *float* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str* or *float*."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def fast_float(input):\n"
"    ...    try:\n"
"    ...        return float(input)\n"
"    ...    except ValueError:\n"
"    ...        return input\n"
"    ...\n"
"\n"
"The main differences are that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data, and a fast implementation of *atof* is used to\n"
"convert strings to a *float* which for large exponents may be inaccurate\n"
"in the 12th decimal place (you should be mindful of this when converting\n"
"very large or small numbers).\n"
"\n";
static PyObject *
fastnumbers_fast_float(PyObject *self, PyObject *args)
{
    PyObject *input = NULL;
    char *str;
    double result;
    bool error;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now. */
    if (ANYNUM(input))
        return PyNumber_Float(input);

    /* Attempt to convert to char*. */
    str = convert_string(input);
    if (str == NULL) return NULL;

    /* Attempt to convert to a float */
    result = fast_atof(str, &error);

    /* If there was an error, return input as-is. Otherwise, return the float object */
    if (error) return Py_BuildValue("O", input);
    return Py_BuildValue("d", result);
}


const char fast_int_docstring[] = 
"Convert a string to a *int* if possible, return the input otherwise.\n"
"\n"
"Convert input to a *int* if possible, return unchanged if not\n"
"possible; no ValueError will be raised for invalid string input, although\n"
"a TypeError error will be raised for types the *int* function also does\n"
"not accept, like a *list*.  The return value is guaranteed to be of\n"
"type *str*, or *int* (or *long* on Python2 if the input was *long*)."
"\n"
"It is roughly equivalent to\n"
"\n"
"    >>> def fast_int(input):\n"
"    ...    try:\n"
"    ...        return int(input)\n"
"    ...    except ValueError:\n"
"    ...        return input\n"
"    ...\n"
"\n"
"The main differences are that much of Python's exception mechanism is\n"
"bypassed so that this will be more efficient for attempted conversions\n"
"on large sets of data, and a fast implementation of *atoi* is used to\n"
"convert strings to an *int* which does not check for overflow/underflow\n"
"(you should be mindful of this when converting very large or small\n"
"numbers).\n"
"\n";
static PyObject *
fastnumbers_fast_int(PyObject *self, PyObject *args)
{
    PyObject *input = NULL;
    char *str;
    long result;
    bool error;

    /* Read the function argument. */
    if (!PyArg_ParseTuple(args, "O", &input))
        return NULL;

    /* If the input is already a number, return now. */
    if (ANYNUM(input))
#if PY_MAJOR_VERSION >= 3
        return PyNumber_Long(input);
#else
        return PyNumber_Int(input);
#endif

    /* Attempt to convert to char*. */
    str = convert_string(input);
    if (str == NULL) return NULL;

    /* Attempt to convert to a int */
    result = fast_atoi(str, &error);

    /* If there was an error, return input as-is. Otherwise, return the int object */
    if (error) return Py_BuildValue("O", input);
    return Py_BuildValue("l", result);
}


/* This defines the methods contained in this module. */
static PyMethodDef FastnumbersMethods[] = {
    {"safe_real", fastnumbers_safe_real, METH_VARARGS, safe_real_docstring},
    {"safe_float", fastnumbers_safe_float, METH_VARARGS, safe_float_docstring},
    {"safe_int", fastnumbers_safe_int, METH_VARARGS, safe_int_docstring},
    // {"safe_forceint", fastnumbers_safe_forceint, METH_VARARGS, safe_forceint_docstring},
    {"fast_real", fastnumbers_fast_real, METH_VARARGS, fast_real_docstring},
    {"fast_float", fastnumbers_fast_float, METH_VARARGS, fast_float_docstring},
    {"fast_int", fastnumbers_fast_int, METH_VARARGS, fast_int_docstring},
    // {"fast_forceint", fastnumbers_fast_forceint, METH_VARARGS, fast_forceint_docstring},
    // {"str_isreal", fastnumbers_str_isreal, METH_VARARGS, str_isreal_docstring},
    // {"str_isfloat", fastnumbers_str_isfloat, METH_VARARGS, str_isfloat_docstring},
    // {"str_isint", fastnumbers_str_isint, METH_VARARGS, str_isint_docstring},
    // {"fast_isreal", fastnumbers_fast_isreal, METH_VARARGS, fast_isreal_docstring},
    // {"fast_isfloat", fastnumbers_fast_isfloat, METH_VARARGS, fast_isfloat_docstring},
    // {"fast_isint", fastnumbers_fast_isint, METH_VARARGS, fast_isint_docstring},
    // {"fast_isintlike", fastnumbers_fast_isintlike, METH_VARARGS, fast_isintlike_docstring},}
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/* Define how modules are returned. */
#if PY_MAJOR_VERSION >= 3
#define INITERROR return NULL
#else
#define INITERROR return
#endif

#if PY_MAJOR_VERSION >= 3
/* Module properties are given in a separate variable in Python3. */
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "fastnumbers",
        "Quickly convert strings to numbers.",
        -1,
        FastnumbersMethods,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

/* We want a module-level variable that is the version. */
static PyObject *fastnumbers_version;

/* This is the module initialization step. */ 
#if PY_MAJOR_VERSION >= 3
PyObject *
PyInit_fastnumbers(void)
#else
PyMODINIT_FUNC
initfastnumbers(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *m = PyModule_Create(&moduledef);
#else
    PyObject *m = Py_InitModule3("fastnumbers",
                                 FastnumbersMethods,
                                 "Quickly convert strings to numbers.");
#endif

    if (m == NULL)
        INITERROR;

    fastnumbers_version = PyUnicode_FromString(FASTNUMBERS_VERSION);
    Py_INCREF(fastnumbers_version);
    PyModule_AddObject(m, "__version__", fastnumbers_version);

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
