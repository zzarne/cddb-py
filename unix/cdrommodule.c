#include "Python.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/cdrom.h>
#endif

#if sun || __FreeBSD__
#include <sys/cdio.h>
#endif

/* 
 * Since FreeBSD has identical support but different names for lots
 * of these structs and constants, we'll just #define CDDB_WHATEVER
 * so that we don't have to repeat the code.
 */

#ifdef __FreeBSD__
#define CDDB_TOC_HEADER_STRUCT ioc_toc_header 
#define CDDB_STARTING_TRACK_FIELD starting_track 
#define CDDB_ENDING_TRACK_FIELD ending_track
#define CDDB_READ_TOC_HEADER_FLAG CDIOREADTOCHEADER 
#define CDDB_TOC_ENTRY_STRUCT ioc_read_toc_single_entry 
#define CDDB_TRACK_FIELD track 
#define CDDB_FORMAT_FIELD address_format 
#define CDDB_MSF_FORMAT CD_MSF_FORMAT 
#define CDDB_ADDR_FIELD entry.addr 
#define CDDB_READ_TOC_ENTRY_FLAG CDIOREADTOCENTRY 
#define CDDB_CDROM_LEADOUT 0xaa 
#else /* Linux and Solaris */
#define CDDB_TOC_HEADER_STRUCT cdrom_tochdr 
#define CDDB_STARTING_TRACK_FIELD cdth_trk0 
#define CDDB_ENDING_TRACK_FIELD cdth_trk1 
#define CDDB_READ_TOC_HEADER_FLAG CDROMREADTOCHDR 
#define CDDB_TOC_ENTRY_STRUCT cdrom_tocentry
#define CDDB_TRACK_FIELD cdte_track 
#define CDDB_FORMAT_FIELD cdte_format 
#define CDDB_MSF_FORMAT CDROM_MSF 
#define CDDB_ADDR_FIELD cdte_addr 
#define CDDB_READ_TOC_ENTRY_FLAG CDROMREADTOCENTRY 
#define CDDB_CDROM_LEADOUT CDROM_LEADOUT
#endif /* __FreeBSD__ */

/* 
 * cdrommodule.c 
 * Python extension module for reading in audio CD-ROM data
 *
 * Please port me to other OSes besides Linux, Solaris, and FreeBSD! 
 * See the README for info.
 *
 * Written 17 Nov 1999 by Ben Gertzfield <che@debian.org>
 * This work is released under the GNU GPL, version 2 or later.
 *
 * FreeBSD support by Michael Yoon <michael@yoon.org>
 *
 * Thanks to Viktor Fougstedt <viktor@dtek.chalmers.se> for info
 * on the <sys/cdio.h> include file to make this work on Solaris!
 *
 * Release version 1.0
 * CVS ID: $Id$
 */

static PyObject *cdrom_error;

static PyObject *cdrom_toc_header(PyObject *self, PyObject *args)
{
    struct CDDB_TOC_HEADER_STRUCT hdr;
    PyObject *cdrom_fileobj;
    int cdrom_fd;

    if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &cdrom_fileobj))
	return NULL;

    cdrom_fd = fileno(PyFile_AsFile(cdrom_fileobj));

    if (ioctl(cdrom_fd, CDDB_READ_TOC_HEADER_FLAG, &hdr) < 0) {
	PyErr_SetFromErrno(cdrom_error);
	return NULL;
    }
    
    return Py_BuildValue("bb", hdr.CDDB_STARTING_TRACK_FIELD, 
			 hdr.CDDB_ENDING_TRACK_FIELD);
}

static PyObject *cdrom_toc_entry(PyObject *self, PyObject *args)
{
    struct CDDB_TOC_ENTRY_STRUCT entry;
    PyObject *cdrom_fileobj;
    int cdrom_fd;
    unsigned char track;

    if (!PyArg_ParseTuple(args, "O!b", &PyFile_Type, &cdrom_fileobj, &track))
	return  NULL;

    cdrom_fd = fileno(PyFile_AsFile(cdrom_fileobj));

    entry.CDDB_TRACK_FIELD = track;
    entry.CDDB_FORMAT_FIELD = CDROM_MSF;

    if (ioctl(cdrom_fd, CDDB_READ_TOC_ENTRY_FLAG, &entry) < 0) {
	PyErr_SetFromErrno(cdrom_error);
	return NULL;
    }

    return Py_BuildValue("bbb", entry.CDDB_ADDR_FIELD.msf.minute, 
			 entry.CDDB_ADDR_FIELD.msf.second, 
			 entry.CDDB_ADDR_FIELD.msf.frame);
}

static PyObject *cdrom_leadout(PyObject *self, PyObject *args)
{
    struct CDDB_TOC_ENTRY_STRUCT entry;
    PyObject *cdrom_fileobj;
    int cdrom_fd;

    if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &cdrom_fileobj))
	return  NULL;

    cdrom_fd = fileno(PyFile_AsFile(cdrom_fileobj));

    entry.CDDB_TRACK_FIELD = CDDB_CDROM_LEADOUT;
    entry.CDDB_FORMAT_FIELD = CDDB_MSF_FORMAT;

    if (ioctl(cdrom_fd, CDDB_READ_TOC_ENTRY_FLAG, &entry) < 0) {
	PyErr_SetFromErrno(cdrom_error);
	return NULL;
    }

    return Py_BuildValue("bbb", entry.CDDB_ADDR_FIELD.msf.minute, 
			 entry.CDDB_ADDR_FIELD.msf.second, 
			 entry.CDDB_ADDR_FIELD.msf.frame);
}

static PyMethodDef cdrom_methods[] = {
    { "toc_header", cdrom_toc_header, METH_VARARGS },
    { "toc_entry", cdrom_toc_entry, METH_VARARGS },
    { "leadout", cdrom_leadout, METH_VARARGS},
    { NULL, NULL }
};

void initcdrom(void)
{
    PyObject *module, *dict;

    module = Py_InitModule("cdrom", cdrom_methods);
    dict = PyModule_GetDict(module);
    cdrom_error = PyErr_NewException("cdrom.error", NULL, NULL);
    PyDict_SetItemString(dict, "error", cdrom_error);
}