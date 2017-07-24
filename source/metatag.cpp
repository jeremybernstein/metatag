/* 
	metatag
	jeremy bernstein - jeremy@bootsquad.com	
*/

#include "ext.h"
#include "ext_user.h"
#include "ext_obex.h"
#include "ext_charset.h"
#include "ext_drag.h"

#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

typedef struct _metatag 
{
	t_object					ob;
	TagLib::FileRef				*fileref;
	t_symbol					*sdummy;
	long						idummy;
	t_symbol					*file;
	char						filepath[MAX_PATH_CHARS];
} t_metatag;

void *metatag_class; 				// actual class (new school)

void metatag_anything(t_metatag *x, t_symbol *s, long ac, t_atom *av);
void metatag_assist(t_metatag *x, void *b, long m, long a, char *s);
void metatag_free(t_metatag *x);
void *metatag_new(t_symbol *s, long ac, t_atom *av);
void metatag_open(t_metatag *x, t_symbol *s, long ac, t_atom *av);
void metatag_close(t_metatag *x);

t_max_err metatag_ssetter(t_metatag *x, void *attr, long ac, t_atom *av);
t_max_err metatag_sgetter(t_metatag *x, void *attr, long *ac, t_atom **av);
t_max_err metatag_isetter(t_metatag *x, void *attr, long ac, t_atom *av);
t_max_err metatag_igetter(t_metatag *x, void *attr, long *ac, t_atom **av);

long metatag_acceptsdrag(t_metatag *x, t_object *drag, t_object *view);
t_max_err metatag_file_get(t_metatag *x, void *attr, long *ac, t_atom **av);
t_max_err metatag_file_set(t_metatag *x, void *attr, long ac, t_atom *av);

static t_symbol *sps_title, *ps_artist, *ps_album, *ps_comment, *ps_genre;
static t_symbol *ps_year, *ps_track;
static t_symbol *ps_getname;
static t_symbol *ps_file, *ps_read, *ps__none;;

void ext_main(void *r)
{	
	t_class *c;
	
	c = class_new("metatag", (method)metatag_new, (method)metatag_free, (short)sizeof(t_metatag), 
		(method)0L, A_GIMME, 0);
	
	class_addmethod(c, (method)metatag_open,			"read",		A_GIMME, 0);
	class_addmethod(c, (method)metatag_open,			"open",		A_GIMME, 0);
	class_addmethod(c, (method)metatag_close,			"dispose",	0);
	class_addmethod(c, (method)metatag_close,			"close",	0);

    class_addmethod(c, (method)metatag_assist, 		"assist",	A_CANT, 0);
    class_addmethod(c, (method)object_obex_dumpout,	"dumpout",	A_CANT, 0);

	CLASS_ATTR_SYM(c, "title", 0, t_metatag, sdummy);
	CLASS_ATTR_ACCESSORS(c, "title", (method)metatag_sgetter, (method)metatag_ssetter);
	CLASS_ATTR_SYM(c, "artist", 0, t_metatag, sdummy);
	CLASS_ATTR_ACCESSORS(c, "artist", (method)metatag_sgetter, (method)metatag_ssetter);
	CLASS_ATTR_SYM(c, "album", 0, t_metatag, sdummy);
	CLASS_ATTR_ACCESSORS(c, "album", (method)metatag_sgetter, (method)metatag_ssetter);
	CLASS_ATTR_SYM(c, "comment", 0, t_metatag, sdummy);
	CLASS_ATTR_ACCESSORS(c, "comment", (method)metatag_sgetter, (method)metatag_ssetter);
	CLASS_ATTR_SYM(c, "genre", 0, t_metatag, sdummy);
	CLASS_ATTR_ACCESSORS(c, "genre", (method)metatag_sgetter, (method)metatag_ssetter);

	CLASS_ATTR_LONG(c, "year", 0, t_metatag, idummy);
	CLASS_ATTR_ACCESSORS(c, "year", (method)metatag_igetter, (method)metatag_isetter);
	CLASS_ATTR_LONG(c, "track", 0, t_metatag, idummy);
	CLASS_ATTR_ACCESSORS(c, "track", (method)metatag_igetter, (method)metatag_isetter);

	CLASS_ATTR_SYM(c, "file", 0L, t_metatag, file);
	CLASS_ATTR_STYLE_LABEL(c, "file", 0L, "file", "Media File"); 
	CLASS_ATTR_ACCESSORS(c, "file", (method)metatag_file_get, (method)metatag_file_set);

	class_addmethod(c, (method)metatag_acceptsdrag, "acceptsdrag_unlocked", A_CANT, 0);
	class_addmethod(c, (method)metatag_acceptsdrag, "acceptsdrag_locked", A_CANT, 0);
	
	class_register(CLASS_BOX,c);
	metatag_class = c;
	
	sps_title = gensym("title");
	ps_artist = gensym("artist");
	ps_album = gensym("album");
	ps_comment = gensym("comment");
	ps_genre = gensym("genre");
	
	ps_year = gensym("year");
	ps_track = gensym("track");
	
	ps_getname = gensym("getname");
	ps_file = gensym("file");
	ps_read = gensym("read");
	ps__none = gensym("<none>");
}

long metatag_acceptsdrag(t_metatag *x, t_object *drag, t_object *view)
{
	// accept any file; drag role is useful if we know what is being dragged in 
	// (audio, video, etc.) but we might have an mp3, a flac or some other media file. 
	// we'll let the taglib determine if the file is valid.

	jdrag_object_add(drag, (t_object *)x, ps_read);
	return true;
/*	if (jdrag_matchdragrole(drag, ps_file, 0)) {
		jdrag_object_add(drag, (t_object *)x, ps_read);
		return true;

	}
	return false;
 */
}

t_max_err metatag_file_get(t_metatag *x, void *attr, long *ac, t_atom **av)
{
	char alloc;
	
	if (atom_alloc(ac, av, &alloc) == MAX_ERR_NONE) {
		atom_setsym(*av, x->file ? x->file : ps__none);
	}
	return MAX_ERR_NONE;
}

t_max_err metatag_file_set(t_metatag *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		metatag_open(x, NULL, ac, av);
	}
	return MAX_ERR_NONE;
}

t_max_err metatag_ssetter(t_metatag *x, void *attr, long ac, t_atom *av)
{
	t_symbol *attrname = (t_symbol *)object_method(attr, ps_getname);
	TagLib::String tstr;	

	if (!x->fileref) {
		return MAX_ERR_GENERIC;
	}
	
	if (ac && av && atom_gettype(av) == A_SYM) {
		tstr = TagLib::String(atom_getsym(av)->s_name, TagLib::String::UTF8);
	} else {
		tstr = TagLib::String::null;
	}
	if (attrname == sps_title) {
		x->fileref->tag()->setTitle(TagLib::String::null); // clear it first to avoid weirdness
		x->fileref->tag()->setTitle(tstr);
	} else if (attrname == ps_artist) {
		x->fileref->tag()->setArtist(TagLib::String::null); // clear it first to avoid weirdness
		x->fileref->tag()->setArtist(tstr);
	} else if (attrname == ps_album) {
		x->fileref->tag()->setAlbum(TagLib::String::null); // clear it first to avoid weirdness
		x->fileref->tag()->setAlbum(tstr);
	} else if (attrname == ps_comment) {
		x->fileref->tag()->setComment(TagLib::String::null); // clear it first to avoid weirdness
		x->fileref->tag()->setComment(tstr);
	} else if (attrname == ps_genre) {
		x->fileref->tag()->setGenre(TagLib::String::null); // clear it first to avoid weirdness
		x->fileref->tag()->setGenre(tstr);
	} else {
		return MAX_ERR_GENERIC;
	}
	x->fileref->save(); // save whenever a change is made
	return MAX_ERR_NONE;
}

t_max_err metatag_sgetter(t_metatag *x, void *attr, long *ac, t_atom **av)
{
	t_symbol *attrname = (t_symbol *)object_method(attr, ps_getname);
	TagLib::String tstr;	
	std::string cstr;
	char alloc;
	
	if (!x->fileref) {
		return MAX_ERR_GENERIC;
	}
	
	if (atom_alloc(ac, av, &alloc) != MAX_ERR_NONE) {
		return MAX_ERR_OUT_OF_MEM;
	}
	
	if (attrname == sps_title) {
		tstr = x->fileref->tag()->title();
	} else if (attrname == ps_artist) {
		tstr = x->fileref->tag()->artist();
	} else if (attrname == ps_album) {
		tstr = x->fileref->tag()->album();
	} else if (attrname == ps_comment) {
		tstr = x->fileref->tag()->comment();
	} else if (attrname == ps_genre) {
		tstr = x->fileref->tag()->genre();
	}
	
	if (!tstr.isEmpty() && !tstr.isNull()) {
		cstr = tstr.to8Bit(true);
	} else {
		cstr = "";
	}
	atom_setsym(*av, gensym((char *)cstr.c_str()));

	return MAX_ERR_NONE;
}

t_max_err metatag_isetter(t_metatag *x, void *attr, long ac, t_atom *av)
{
	t_symbol *attrname = (t_symbol *)object_method(attr, ps_getname);
	t_atom_long val;
	
	if (!x->fileref) {
		return MAX_ERR_GENERIC;
	}
	
	if (ac && av) {
		val = atom_getlong(av);
	} else {
		val = 0;
	}
	
	if (attrname == ps_year) {
		x->fileref->tag()->setYear(0); // clear it first to avoid weirdness
		x->fileref->tag()->setYear(val);
	} else if (attrname == ps_track) {
		x->fileref->tag()->setTrack(0);
		x->fileref->tag()->setTrack(val);
	} else {
		return MAX_ERR_GENERIC;
	}
	x->fileref->save(); // save whenever a change is made
	return MAX_ERR_NONE;
}

t_max_err metatag_igetter(t_metatag *x, void *attr, long *ac, t_atom **av)
{
	t_symbol *attrname = (t_symbol *)object_method(attr, ps_getname);
	long val = 0;
	char alloc;
	
	if (!x->fileref) {
		return MAX_ERR_GENERIC;
	}
	
	if (attrname == ps_year) {
		val = x->fileref->tag()->year();
	} else if (attrname == ps_track) {
		val = x->fileref->tag()->track();
	}
	
	if (val) {
		if (atom_alloc(ac, av, &alloc) != MAX_ERR_NONE) {
			return MAX_ERR_OUT_OF_MEM;
		}
		atom_setlong(*av, val);
	} else {
		*ac = 0;
	}
	return MAX_ERR_NONE;
}

// this function doesn't work yet; haven't decided how to implement it
#if NOTYET
void metatag_picture_get(t_metatag *x, t_symbol *s, long *ac, t_atom **av)
{
	TagLib::MPEG::File *mf;
	
	if (*x->filepath) {
		// mp3 first
#ifdef WIN_VERSION
		WCHAR *zWide;
		
		zWide = (WCHAR*) charset_utf8tounicode(x->filepath,NULL); 
		if (zWide) {
			mf = new TagLib::MPEG::File(zWide);
			sysmem_freeptr(zWide);
		}
#else
		mf = new TagLib::MPEG::File(x->filepath);
#endif
		if (mf) { // it's some kind of an MPEG file; if it has an id3v2 tag, it might have a picture
			if (mf->isValid() && mf->ID3v2Tag()) {
				TagLib::ID3v2::FrameList l = mf->ID3v2Tag()->frameList("APIC");
				if (!l.isEmpty()) {
					TagLib::ID3v2::AttachedPictureFrame *f =
						static_cast<TagLib::ID3v2::AttachedPictureFrame *>(l.front());
					// f->picture().data() is the binary data
				}
			}
			delete mf;
		}
	}
}
#endif

void metatag_close(t_metatag *x)
{
	if (x->fileref) {
		delete x->fileref;
		x->fileref = NULL;
	}
}

void metatag_open(t_metatag *x, t_symbol *s, long ac, t_atom *av)
{
	char fname[MAX_PATH_CHARS] = "";
	char aname[MAX_PATH_CHARS] = "";
	short fpath = 0;
	t_fourcc type = 0;
	TagLib::FileRef *fileref = NULL;
	
	if (ac && av && atom_gettype(av) == A_SYM) {
		strncpy(fname, atom_getsym(av)->s_name, MAX_PATH_CHARS);
		if (locatefile_extended(fname, &fpath, &type, &type, 0)) {
			object_error((t_object *)x, "file not found: %s", atom_getsym(av)->s_name);
			return;
		}
	} else {
		if (open_dialog(fname, &fpath, &type, &type, 0)) {
			return;
		}
	}
	path_topotentialname(fpath, fname, aname, true);
#ifdef WIN_VERSION
	path_nameconform(aname, fname, PATH_STYLE_NATIVE_WIN, PATH_TYPE_ABSOLUTE);
	{
		WCHAR *zWide;

		zWide = (WCHAR*) charset_utf8tounicode(fname,NULL); 
		if (zWide) {
			fileref = new TagLib::FileRef(zWide);
			sysmem_freeptr(zWide);
		} else {
			object_error((t_object *)x, "error opening file: %s", fname);
			return;
		}
	}
#else
	path_nameconform(aname, fname, PATH_STYLE_NATIVE, PATH_TYPE_BOOT);
	fileref = new TagLib::FileRef(fname);
#endif
//	post("path: %s", fname);
	
	if (fileref->isNull()) {
		object_error((t_object *)x, "error opening file: %s", fname);
		delete fileref;
		return;
	}
	if (x->fileref)
		delete x->fileref;
	x->fileref = fileref;
	strncpy(x->filepath, fname, MAX_PATH_CHARS); // save the path used; we may need it for extracting cover art
	x->file = gensym(aname); // use abs path, I guess; 
}

void metatag_assist(t_metatag *x, void *b, long m, long a, char *s)
{
	if (m == 1) { //input
		sprintf(s,"messages in");			
	} 
	else {	//output
		switch(a) {
			case 0:
			default: sprintf(s, "out"); break;
		}
	}
}

void metatag_free(t_metatag *x)
{
	if (x->fileref)
		delete x->fileref;
}

void *metatag_new(t_symbol *s, long ac, t_atom *av)
{
	t_metatag *x;
	
	if ((x = (t_metatag *)object_alloc((t_class *)metatag_class))) {
		object_obex_store(x, gensym("dumpout"), (t_object *)outlet_new(x,NULL));
		x->fileref = NULL;
		x->filepath[0] = '\0';
	}
	return (x);
}
