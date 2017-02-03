// Begin contactlooper.c
// This file contains a python module written by Jesse B)loom, 2004
// It is designed for executing fast loops in the analysis of lattice
// protein conformation energies.
//
// The Python module name is 'contactlooper'
//
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// Handleing the String and Int conversions from Python 2 to 3
#if PY_MAJOR_VERSION >= 3
    #define PyInt_AS_LONG PyLong_AS_LONG
    #define PyString_AS_STRING PyBytes_AS_STRING
#endif
// Changes in module initialization
#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) void init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
#endif
//
// 'ContactLooper' error for module
static PyObject *ContactLooperError;
//
//
// Function 'NoTargetLooper'
static PyObject *NoTargetLooper(PyObject *self, PyObject *args) {
    PyObject *res_interactions, *contactsets, *contactsetdegeneracy, *cs;
    PyObject *ret;
    int single_native = 1;
    int folded = 1;
    double temp, minE, e_contactset, partitionsum = 0.0;
    long best, numinteractions, i, j, contactindex, totalcontacts;
    static long interactionslength = 0; // stores length of 'interactions'
    static long numcontactsets = 0;
    double *interactions;
    long *c_contactsets; // stores the contact sets.  The contacts for contact j
    // are the elements given by c_contactsets[i] where i >= c_contactstarts[j]
    // and i < c_contactstarts[j + 1].  Each contact set is several integers.
    // x = c_contactsets[i] describes contact i; it has the value
    // length * ires + jres where 0 <= ires, jres < length and ires <
    // jres + 1.
    long *c_contactstarts; // stores starts of contact sets as described above
    long *c_contactsetdegeneracy; // stores the contact set degeneracies.  The
    // degeneracy for contact set j is c_contactsetdegeneracy[j]
    // Parse the arguments
    if (! PyArg_ParseTuple(args, "OOOd", &res_interactions, &contactsets, &contactsetdegeneracy, &temp)) {
    	PyErr_SetString(ContactLooperError, "Error parsing arguments.");
    	return NULL;
    }
    // compute the number of interactions
    numinteractions = PyList_Size(res_interactions);
    //
    interactionslength = numinteractions;
	interactions = (double *) malloc(interactionslength * sizeof(double));
	numcontactsets = PyList_Size(contactsets);
	c_contactsetdegeneracy = (long *) malloc(numcontactsets * sizeof(long));
	totalcontacts = 0;
	for (i = 0; i < numcontactsets; i++) {
	    c_contactsetdegeneracy[i] = PyInt_AS_LONG(PyList_GetItem(contactsetdegeneracy, i));
	    cs = PyList_GetItem(contactsets, i);
	    totalcontacts += PyList_Size(cs);
	}
	c_contactsets = (long *) malloc(totalcontacts * sizeof(long));
	c_contactstarts = (long *) malloc((numcontactsets + 1) * sizeof(long));
	contactindex = 0;
	c_contactstarts[0] = 0;
	for (i = 0; i < numcontactsets; i++) {
	    cs = PyList_GetItem(contactsets, i);
	    for (j = 0; j < PyList_Size(cs); j++) {
    		c_contactsets[contactindex] = PyInt_AS_LONG(PyList_GET_ITEM(cs, j));
    		contactindex += 1;
	    }
	    c_contactstarts[i + 1] = contactindex;
	}
    // assign the values in res_interactions to interactions
    for (i = 0; i < numinteractions; i++) {
	    interactions[i] = PyFloat_AS_DOUBLE(PyList_GetItem(res_interactions, i));
    }
    // set initial values for minE and ibest
    e_contactset = 0.0;
    for (j = c_contactstarts[0]; j < c_contactstarts[1]; j++) {
	    e_contactset += interactions[c_contactsets[j]];
    }
    minE = e_contactset;
    partitionsum += exp(-e_contactset / temp) * c_contactsetdegeneracy[0];
    best = 1;
    // loop over remaining conformations to find the partition sum
    for (i = 1; i < numcontactsets; i++) {
    	e_contactset = 0.0;
    	for (j = c_contactstarts[i]; j < c_contactstarts[i + 1]; j++) {
        	    e_contactset += interactions[c_contactsets[j]];
    	}
    	partitionsum += exp(-e_contactset / temp) * c_contactsetdegeneracy[i];
    	if (e_contactset < minE) {
    	    minE = e_contactset;
    	    best = i;
            single_native = 1;
    	} else if (e_contactset == minE){
            // not a single native state.
            single_native = 0;
        }
    }
    // Clean up!
    if (interactions != NULL) {
        free(interactions);
    }
    if (c_contactsets != NULL) {
        free(c_contactsets);
    }
    if (c_contactstarts != NULL) {
        free(c_contactstarts);
    }
    if (c_contactsetdegeneracy != NULL) {
        free(c_contactsetdegeneracy);
    }
    if (single_native == 0){
        folded = 0;
    }
    // Construct the return tuple and return it
    ret = Py_BuildValue("dldi", minE, best, partitionsum, folded);
    return ret;
}
//
// Documentation string for 'NoTargetLooper'
static char NoTargetLooper_doc[] = "Evaluates a sequence on its lowest energy conformation.\n\nCall is: '(minE, ibest, partitionsum) = NoTargetLooper(res_interactions,\n\tcontactsets, contactsetdegeneracy, temp)'\n'res_interactions' is a list of numbers such that 'res_interactions[j]'\n\tis the energy of interaction for contact 'j' where 'j' is\n\tthe number of the contact as stored in the sublists of 'contactsets'.\n'contactsets' is a list of all contact sets.  Each contact set is\n\titself a list of integers, such that 'j = contactsets[i][k]'\n\tis the number of contact 'k' in contact set 'i' and\n\t'j' is the index for the energy of this contact in 'res_interactions'.\n'contactsetdegeneracy' is a list of the degeneracies of the contact sets'.\n\tElement 'i' is the degeneracy of contact set 'contactsets[i]'.\n\tDegeneracies are integers.\n'temp' is a float giving the temperature for computing the partition sum.\nThe returned variable is a 3-tuple.  The first element, 'minE',\n\tis the energy of the lowest energy conformation.  The second\n\telement, 'ibest', is the index of this lowest energy conformation\n\tin 'contactsets'.  The third element, 'partitionsum', is the partition\n\tsum at temperature 'temp'.\nNOTE: NO ERROR CHECKING IS PERFORMED ON THE PASSED VARIABLES.\n\tMAKE SURE THE PASSED VARIABLES ARE OF THE CORRECT TYPES.\n";
//
// Function 'TargetLooper'

static PyObject *TargetLooper(PyObject *self, PyObject *args) {
    PyObject *res_interactions, *contactsets, *contactsetdegeneracy, *cs;
    PyObject *ret, *targetconf, *contactsetconformation;
    int use_dGf_cutoff;
    double cutoff, temp, dGf_cutoff, minE = 0.0, e_contactset, partitionsum = 0.0;
    long numinteractions, i, j, k, contactindex, totalcontacts;
    static long interactionslength = 0; // stores length of 'interactions'
    static long numcontactsets = 0;
    static long targetconformationindex = 0;
    static char *c_targetconf;
    char *targetconfstring;
    //
    double *interactions;
    long *c_contactsets; // stores the contact sets.  The contacts for contact j
    // are the elements given by c_contactsets[i] where i >= c_contactstarts[j]
    // and i < c_contactstarts[j + 1].  Each contact set is several integers.
    // x = c_contactsets[i] describes contact i; it has the value
    // length * ires + jres where 0 <= ires, jres < length and ires <
    // jres + 1.
    long *c_contactstarts; // stores starts of contact sets as described above
    long *c_contactsetdegeneracy; // stores the contact set degeneracies.  The
    // degeneracy for contact set j is c_contactsetdegeneracy[j]
    // Parse the arguments
    if (! PyArg_ParseTuple(args, "OOOdOOid", &res_interactions, &contactsets, &contactsetdegeneracy, &temp, &targetconf, &contactsetconformation, &use_dGf_cutoff, &dGf_cutoff)) {
        PyErr_SetString(ContactLooperError, "Error parsing arguments.");
        return NULL;
    }
    // compute the number of interactions
    numinteractions = PyList_GET_SIZE(res_interactions);
    interactionslength = numinteractions;
    //
    interactions = (double *) malloc(interactionslength * sizeof(double));
    numcontactsets = PyList_GET_SIZE(contactsets);
    c_contactsetdegeneracy = (long *) malloc(numcontactsets * sizeof(long));
    totalcontacts = 0;
    for (i = 0; i < numcontactsets; i++) {
        c_contactsetdegeneracy[i] = PyInt_AS_LONG(PyList_GET_ITEM(contactsetdegeneracy, i));
        cs = PyList_GET_ITEM(contactsets, i);
        totalcontacts += PyList_GET_SIZE(cs);
    }
    c_contactsets = (long *) malloc(totalcontacts * sizeof(long));
    c_contactstarts = (long *) malloc((numcontactsets + 1) * sizeof(long));
    contactindex = 0;
    c_contactstarts[0] = 0;
    for (i = 0; i < numcontactsets; i++) {
        cs = PyList_GET_ITEM(contactsets, i);
        for (j = 0; j < PyList_GET_SIZE(cs); j++) {
            c_contactsets[contactindex] = PyInt_AS_LONG(PyList_GET_ITEM(cs, j));
            contactindex += 1;
        }
        c_contactstarts[i + 1] = contactindex;
    }
    // find the target string if we have a new target

    if ((c_targetconf == NULL) || (strcmp(c_targetconf, PyString_AS_STRING(targetconf)))) {
        targetconfstring = PyString_AS_STRING(targetconf);
        c_targetconf = (char *) malloc((strlen(targetconfstring) + 1) * sizeof(char));
        strcpy(c_targetconf, targetconfstring);
        k = 0;
        targetconformationindex = 0;
        while (! targetconformationindex) {
            if (k > numcontactsets) {
                PyErr_SetString(ContactLooperError, "Did not find target conformations.");
                return NULL;
            }
            if ((c_contactsetdegeneracy[k] == 1) && ! strcmp(c_targetconf, PyString_AS_STRING(PyList_GET_ITEM(contactsetconformation, k)))) {
                targetconformationindex = k;
            }
            k += 1;
        }
    }
    // assign the values in res_interactions to interactions
    for (i = 0; i < numinteractions; i++) {
        interactions[i] = PyFloat_AS_DOUBLE(PyList_GET_ITEM(res_interactions, i));
    }
    // stuff for target conformation
    for (j = c_contactstarts[targetconformationindex]; j < c_contactstarts[targetconformationindex + 1]; j++) {
        minE += interactions[c_contactsets[j]];
    }
    // loop over conformations to find the partition sum
    if (use_dGf_cutoff) {
        i = 0;
        cutoff = 2.0 * exp(-minE / temp);
        while ((cutoff >= partitionsum) && (i < numcontactsets)) {
            e_contactset = 0.0;
            for (j = c_contactstarts[i]; j < c_contactstarts[i + 1]; j++) {
                e_contactset += interactions[c_contactsets[j]];
            }
            partitionsum += exp(-e_contactset / temp) * c_contactsetdegeneracy[i];
            i++;
        }
        if (cutoff < partitionsum) {
            partitionsum = -1.0;
        }
    }
    else {
        for (i = 0; i < numcontactsets; i++) {
            e_contactset = 0.0;
            for (j = c_contactstarts[i]; j < c_contactstarts[i + 1]; j++) {
                e_contactset += interactions[c_contactsets[j]];
            }
            partitionsum += exp(-e_contactset / temp) * c_contactsetdegeneracy[i];
        }
    }
    if (interactions != NULL) {
        free(interactions);
    }
    if (c_contactsets != NULL) {
        free(c_contactsets);
    }
    if (c_contactstarts != NULL) {
        free(c_contactstarts);
    }
    if (c_contactsetdegeneracy != NULL) {
        free(c_contactsetdegeneracy);
    }
    if (c_targetconf != NULL) {
        free(c_targetconf);
    }
    // Construct the return tuple and return it
    ret = Py_BuildValue("dld", minE, targetconformationindex, partitionsum);
    return ret;
}

//
// Documentation string for 'TargetLooper'
static char TargetLooper_doc[] = "Evaluates a sequence on a target conformation.\n\nCall is: '(minE, ibest, partitionsum) = TargetLooper(res_interactions,\n\tcontactsets, contactsetdegeneracy, temp,\n\ttarget_conf, contactsetconformation, use_dGf_cutoff, dGf_cutoff)'\n'res_interactions' is a list of numbers such that 'res_interactions[j]'\n\tis the energy of interaction for contact 'j' where 'j' is\n\tthe number of the contact as stored in the sublists of 'contactsets'.\n'contactsets' is a list of all contact sets.  Each contact set is\n\titself a list of integers, such that 'j = contactsets[i][k]'\n\tis the number of contact 'k' in contact set 'i' and\n\t'j' is the index for the energy of this contact in 'res_interactions'.\n'contactsetdegeneracy' is a list of the degeneracies of the contact sets'.\n\tElement 'i' is the degeneracy of contact set 'contactsets[i]'.\n\tDegeneracies are integers.\n'temp' is a float giving the temperature for computing the partition sum.\n'target_conf' is a string specifying the target conformation.\n'contactsetconformation' is a list of the conformations of the contact sets.\n\tElement 'i' is the conformation for contact set 'contactsets[i]' if this\n\tis a unique contact set, or 'None' if otherwise.\n'use_dGf_cutoff' is an integer.  It is 1' if\n\twe are using the 'dGf_cutoff' variable, 0 otherwise.\n'dGf_cutoff' is a float.  It has meaning iff 'dGf_cutoff' is 1.\n\tIn this case, if we can show that the free energy of folding is >\n\t'dGf_cutoff', then we simply return then with 'partition sum' set to -1.\nThe returned variable is a 3-tuple.  The first element, 'minE',\n\tis the energy of the target conformation if this\n\tconformation is unique, or 'None' if this conformation is not unique.\n\tThe second element, 'ibest', is the index of the target conformation.\n\tThe third element, 'partitionsum', is the partition\n\tsum at temperature 'temp'.\nNOTE: NO ERROR CHECKING IS PERFORMED ON THE PASSED VARIABLES.\n\tMAKE SURE THE PASSED VARIABLES ARE OF THE CORRECT TYPES.\n";
//

// Module documentation string
static char contactlooper_doc[] = "Module implementing loops over contact sets.\n\nPublic attributes are:\n'NoTargetLooper' function.\n'TargetLooper' function.\n'ContactLooperError' exception.\nThis is a C-extension.  Written by Jesse Bloom, 2004.";
//
// The module methods
static PyMethodDef contactlooper_methods[] = {
    {"NoTargetLooper", (PyCFunction) NoTargetLooper, METH_VARARGS, NoTargetLooper_doc},
    {"TargetLooper", (PyCFunction) TargetLooper, METH_VARARGS, TargetLooper_doc},
    {NULL}
};
//
// Initialization function for the module
MOD_INIT(contactlooper){
    PyObject *m;

    MOD_DEF(m, "contactlooper", contactlooper_doc, contactlooper_methods)

    if (m == NULL)
        return MOD_ERROR_VAL;

    ContactLooperError = PyErr_NewException("contactlooper.ContactLooperError", NULL, NULL);
    if (ContactLooperError == NULL) {
    	PyErr_SetString(ContactLooperError, "Could not ready the 'ContactLooperError' type.");
    }

    PyModule_AddObject(m, "ContactLooperError", ContactLooperError);
    return MOD_SUCCESS_VAL(m);
}
