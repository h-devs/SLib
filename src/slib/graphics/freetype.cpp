/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/graphics/freetype.h"

#include "slib/graphics/image.h"
#include "slib/graphics/sfnt.h"
#include "slib/core/file_io.h"
#include "slib/core/system.h"
#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftstroke.h"
#include "freetype/ftfntfmt.h"
#include "freetype/tttables.h"
#include "freetype/ttnameid.h"

#define TO_REAL_POS(x) (((sl_real)(x)) / 64.0f)

#define LIBRARY (((Library*)(m_lib.get()))->handle)

namespace slib
{

	namespace priv
	{
		namespace freetype
		{

			class Library : public Referable
			{
			public:
				FT_Library handle;

			protected:
				Library(): handle(sl_null)
				{
				}

				~Library()
				{
					FT_Done_FreeType(handle);
				}

			public:
				static Ref<Library> create()
				{
					FT_Library lib;
					FT_Error err = FT_Init_FreeType(&lib);
					if (!err) {
						Ref<Library> ret = new Library;
						if (ret.isNotNull()) {
							ret->handle = lib;
							return ret;
						}
						FT_Done_FreeType(lib);
					}
					return sl_null;
				}

			};

			static unsigned long ReadStreamCallback(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
			{
				if (!count) {
					return 0;
				}
				IBlockReader* reader = (IBlockReader*)(stream->descriptor.pointer);
				sl_int32 n = reader->readAt32(offset, buffer, (sl_uint32)count);
				if (n > 0) {
					return n;
				}
				return 0;
			}

			static void CloseStreamCallback(FT_Stream stream)
			{
				delete stream;
			}

			static FT_Face OpenFace(FT_Library lib, IBlockReader* reader, sl_uint64 size,  sl_int32 index)
			{
				FT_Stream stream = new FT_StreamRec;
				if (!stream) {
					return sl_null;
				}
				Base::zeroMemory(stream, sizeof(FT_StreamRec));
				stream->descriptor.pointer = reader;
				stream->size = (unsigned long)size;
				stream->read = &ReadStreamCallback;
				stream->close = &CloseStreamCallback;

				FT_Open_Args args;
				Base::zeroMemory(&args, sizeof(args));
				args.flags = FT_OPEN_STREAM;
				args.stream = stream;
				
				FT_Face face;
				FT_Error err = FT_Open_Face(lib, &args, (FT_Long)index, &face);
				if (!err) {
					return face;
				}
				return sl_null;
			}

			static sl_int32 GetFaceIndex(const FreeTypeLoadParam& param)
			{
				if (param.faceIndex < 0) {
					return param.faceIndex;
				}
				if (param.namedInstanceIndex < 0) {
					return -(param.faceIndex + 1);
				}
				return SLIB_MAKE_DWORD2(param.namedInstanceIndex, param.faceIndex);
			}

			class SystemLoader
			{
			private:
				struct Registry
				{
					String path;
					sl_uint32 faceIndex;
					sl_uint32 namedInstanceIndex;
				};

				CHashMap< String, Registry, HashIgnoreCase<String>, CompareIgnoreCase<String> > registries;

			public:
				SystemLoader()
				{
					ListElements<String> listDir(System::getFontsDirectories());
					if (!(listDir.count)) {
						return;
					}
					Ref<Library> lib = Library::create();
					if (lib.isNull()) {
						return;
					}
					for (sl_size i = 0; i < listDir.count; i++) {
						const String& dir = listDir[i];
						ListElements<String> files(File::getFiles(dir));
						for (sl_size k = 0; k < files.count; k++) {
							loadFile(lib->handle, File::concatPath(dir, files[k]));
						}
					}
				}

			private:
				void registerFont(const String& familyName, const String& path, sl_uint32 faceIndex, sl_uint32 instanceId)
				{
					Registry registry;
					registry.path = path;
					registry.faceIndex = faceIndex;
					registry.namedInstanceIndex = instanceId;
					registries.emplace_NoLock(familyName, Move(registry));
				}

				sl_bool loadFileBySFNT(const String& path, SeekableReader<File>* file)
				{
					ListElements< List<String> > faces(SFNT::getFontFamilyNames(file));
					if (!(faces.count)) {
						return sl_false;
					}
					for (sl_size i = 0; i < faces.count; i++) {
						ListElements<String> names(faces[i]);
						for (sl_size k = 0; k < names.count; k++) {
							registerFont(names[k], path, (sl_uint32)i, 0);
						}
					}
					return sl_true;
				}

				void loadFileByFreeType(FT_Library lib, const String& path, SeekableReader<File>* file, sl_uint64 size)
				{
					FT_Long nFaces = 0;
					FT_Long nInstances = 0;
					FT_Long faceId = 0;
					FT_Long instanceId = 0;
					for (;;) {
						FT_Face face = OpenFace(lib, file, size, (instanceId << 16) | faceId);
						if (face) {
							String name(face->family_name);
							if (name.isNotEmpty()) {
								registerFont(name, path, faceId, instanceId);
							}
							nFaces = face->num_faces;
							nInstances = face->style_flags >> 16;
							FT_Done_Face(face);
						}
						instanceId++;
						if (instanceId >= nInstances) {
							faceId++;
							if (faceId >= nFaces) {
								return;
							}
							instanceId = 0;
							nInstances = 0;
						}
					}
				}

				void loadFile(FT_Library lib, const String& path)
				{
					SeekableReader<File> file = File::openForRead(path);
					sl_uint64 size = file.getSize();
					if (!size) {
						return;
					}
					if (loadFileBySFNT(path, &file)) {
						return;
					}
					file.seekToBegin();
					loadFileByFreeType(lib, path, &file, size);
				}

			public:
				Ref<FreeType> open(const String& family)
				{
					Registry* reg = registries.getItemPointer(family);
					if (reg) {
						FreeTypeLoadParam param;
						param.faceIndex = reg->faceIndex;
						param.namedInstanceIndex = reg->namedInstanceIndex;
						return FreeType::loadFromFile(reg->path, param);
					}
					return sl_null;
				}

			};

			SLIB_SAFE_STATIC_GETTER(SystemLoader, GetSystemLoader)
			

			static FT_CharMap SelectCharmap(FT_Face face, FreeTypeKind kind, sl_bool flagSymbolic)
			{
				if (kind == FreeTypeKind::Type1) {
					for (FT_Int i = 0; i < face->num_charmaps; i++) {
						FT_CharMap charmap = face->charmaps[i];
						if (charmap->platform_id == TT_PLATFORM_ADOBE) {
							return charmap;
						}
					}
					if (face->num_charmaps > 0) {
						return face->charmaps[0];
					}
				} else if (kind == FreeTypeKind::TrueType) {
					if (flagSymbolic) {
						for (FT_Int i = 0; i < face->num_charmaps; i++) {
							FT_CharMap charmap = face->charmaps[i];
							if (charmap->platform_id == TT_PLATFORM_MICROSOFT && charmap->encoding_id == TT_MS_ID_SYMBOL_CS) {
								return charmap;
							}
						}
					}
					// look for a Microsoft Unicode cmap
					for (FT_Int i = 0; i < face->num_charmaps; i++) {
						FT_CharMap charmap = face->charmaps[i];
						if (charmap->platform_id == TT_PLATFORM_MICROSOFT && charmap->encoding_id == TT_MS_ID_UNICODE_CS) {
							if (FT_Get_CMap_Format(charmap) != -1) {
								return charmap;
							}
						}
					}
					// look for an Apple MacRoman cmap
					for (FT_Int i = 0; i < face->num_charmaps; i++) {
						FT_CharMap charmap = face->charmaps[i];
						if (charmap->platform_id == TT_PLATFORM_MACINTOSH && charmap->encoding_id == TT_MAC_ID_ROMAN) {
							if (FT_Get_CMap_Format(charmap) != -1) {
								return charmap;
							}
						}
					}
					if (face->num_charmaps > 0) {
						FT_CharMap charmap = face->charmaps[0];
						if (FT_Get_CMap_Format(charmap) != -1) {
							return charmap;
						}
					}
				} else {
					if (face->num_charmaps > 0) {
						return face->charmaps[0];
					}
				}
				return sl_null;
			}

			static int Outline_MoveTo(const FT_Vector* to, void* user)
			{
				((GraphicsPath*)user)->moveTo(TO_REAL_POS(to->x), TO_REAL_POS(to->y));
				return 0;
			}

			static int Outline_LineTo(const FT_Vector* to, void* user)
			{
				((GraphicsPath*)user)->lineTo(TO_REAL_POS(to->x), TO_REAL_POS(to->y));
				return 0;
			}

			static int Outline_ConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
			{
				((GraphicsPath*)user)->conicTo(TO_REAL_POS(control->x), TO_REAL_POS(control->y), TO_REAL_POS(to->x), TO_REAL_POS(to->y));
				return 0;
			}

			static int Outline_CubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
			{
				((GraphicsPath*)user)->cubicTo(TO_REAL_POS(control1->x), TO_REAL_POS(control1->y), TO_REAL_POS(control2->x), TO_REAL_POS(control2->y), TO_REAL_POS(to->x), TO_REAL_POS(to->y));
				return 0;
			}

			static Ref<GraphicsPath> ConvertOutlineToPath(FT_Outline* outline)
			{
				if (outline->n_points) {
					Ref<GraphicsPath> path = GraphicsPath::create();
					if (path.isNotNull()) {
						FT_Outline_Funcs funcs = {
							Outline_MoveTo,
							Outline_LineTo,
							Outline_ConicTo,
							Outline_CubicTo,
							0, 0
						};
						FT_Outline_Decompose(outline, &funcs, path.get());
						return path;
					}
				}
				return sl_null;
			}

		}
	}

	using namespace priv::freetype;


	FreeTypeGlyph::FreeTypeGlyph(): bitmapLeft(0), bitmapTop(0), flagGrayBitmap(sl_true), advance(0), height(0)
	{
	}

	FreeTypeGlyph::~FreeTypeGlyph()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FreeTypeLoadParam)

	FreeTypeLoadParam::FreeTypeLoadParam(): faceIndex(0), namedInstanceIndex(0)
	{
	}


	SLIB_DEFINE_OBJECT(FreeType, Object)

	FreeType::FreeType(): m_face(sl_null)
	{
	}

	FreeType::~FreeType()
	{
		FT_Done_Face(m_face);
	}

	Ref<FreeType> FreeType::_create(Referable* lib, FT_FaceRec_* face, Referable* source)
	{
		Ref<FreeType> ret = new FreeType;
		if (ret.isNotNull()) {
			ret->m_lib = lib;
			ret->m_face = face;
			ret->m_source = source;
			return ret;
		}
		FT_Done_Face(face);
		return sl_null;
	}

	Ref<FreeType> FreeType::load(const Ptr<IBlockReader>& _reader, sl_uint64 size, const FreeTypeLoadParam& param)
	{
		Ptr<IBlockReader> reader = _reader.lock();
		if (reader.isNotNull()) {
			Ref<Library> lib = Library::create();
			if (lib.isNotNull()) {
				FT_Face face = OpenFace(lib->handle, reader.get(), size, GetFaceIndex(param));
				if (face) {
					return _create(lib.get(), face, reader.ref.get());
				}
			}
		}
		return sl_null;
	}

	Ref<FreeType> FreeType::load(const Ptr<IBlockReader>& reader, sl_uint64 size, sl_int32 index)
	{
		FreeTypeLoadParam param;
		param.faceIndex = index;
		return load(reader, size, param);
	}

	Ref<FreeType> FreeType::loadFromFile(const StringParam& path, const FreeTypeLoadParam& param)
	{
		Ref<FileIO> file = FileIO::openForRead(path);
		if (file.isNotNull()) {
			sl_uint64 size = file->getSize();
			if (size) {
				return load(file, size, param);
			}
		}
		return sl_null;
	}

	Ref<FreeType> FreeType::loadFromFile(const StringParam& path, sl_int32 index)
	{
		FreeTypeLoadParam param;
		param.faceIndex = index;
		return loadFromFile(path, param);
	}

	Ref<FreeType> FreeType::loadFromMemory(const Memory& mem, const FreeTypeLoadParam& param)
	{
		if (mem.isNotNull()) {
			Ref<Library> lib = Library::create();
			if (lib.isNotNull()) {
				FT_Face face;
				FT_Error err = FT_New_Memory_Face(lib->handle, (FT_Byte*)(mem.getData()), (FT_Long)(mem.getSize()), GetFaceIndex(param), &face);
				if (!err) {
					return _create(lib.get(), face, mem.ref.get());
				}
			}
		}
		return sl_null;
	}

	Ref<FreeType> FreeType::loadFromMemory(const Memory& mem, sl_int32 index)
	{
		FreeTypeLoadParam param;
		param.faceIndex = index;
		return loadFromMemory(mem, param);
	}

	Ref<FreeType> FreeType::loadSystemFont(const String& family)
	{
		SystemLoader* loader = GetSystemLoader();
		if (loader) {
			return loader->open(family);
		}
		return sl_null;
	}

	sl_uint32 FreeType::getFacesCount()
	{
		return (sl_uint32)(m_face->num_faces);
	}

	sl_uint32 FreeType::getNamedInstancesCount()
	{
		return (sl_uint32)(m_face->style_flags >> 16);
	}

	FreeTypeKind FreeType::getKind()
	{
		StringView kind(FT_Get_Font_Format(m_face));
		if (kind == StringView::literal("TrueType")) {
			return FreeTypeKind::TrueType;
		} else if (kind == StringView::literal("Type 1") || kind == StringView::literal("CFF") || kind == StringView::literal("CID Type 1")) {
			return FreeTypeKind::Type1;
		}
		return FreeTypeKind::Unknown;
	}

	const char* FreeType::getFamilyName()
	{
		return m_face->family_name;
	}

	sl_uint32 FreeType::getGlyphsCount()
	{
		return (sl_uint32)(m_face->num_glyphs);
	}

	sl_bool FreeType::getGlyphIndex(sl_uint32 charcode, sl_uint32& outGlyphIndex)
	{
		if (m_face->charmap) {
			sl_uint32 id = (sl_uint32)(FT_Get_Char_Index(m_face, (FT_ULong)charcode));
			if (id) {
				outGlyphIndex = id;
				return sl_true;
			}
			id = (sl_uint32)(FT_Get_Char_Index(m_face, (FT_ULong)(charcode + 0xf000)));
			if (id) {
				outGlyphIndex = id;
				return sl_true;
			}
		}
		return sl_false;
	}

	void FreeType::selectCharmap(sl_bool flagSymbolic)
	{
		FT_CharMap charmap = SelectCharmap(m_face, getKind(), flagSymbolic);
		if (charmap) {
			FT_Set_Charmap(m_face, charmap);
		}
	}

	sl_bool FreeType::isUnicodeEncoding()
	{
		FT_CharMap charmap = m_face->charmap;
		if (charmap) {
			return charmap->encoding == FT_ENCODING_UNICODE;
		}
		return sl_false;
	}

	sl_bool FreeType::setSize(sl_uint32 width, sl_uint32 height)
	{
		ObjectLocker lock(this);
		FT_Error err = FT_Set_Pixel_Sizes(m_face, (FT_UInt)width, (FT_UInt)height);
		return !err;
	}

	sl_bool FreeType::setSize(sl_uint32 size)
	{
		return setSize(size, size);
	}

	sl_bool FreeType::setRealSize(sl_real width, sl_real height)
	{
		FT_Size_RequestRec req;
		if (width < SLIB_EPSILON) {
			width = height;
		} else if (height < SLIB_EPSILON) {
			height = width;
		}
		req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
		req.width = (FT_Long)(width * 64.0f);
		req.height = (FT_Long)(height * 64.0f);
		if (req.width < 1) {
			req.width = 1;
		}
		if (req.height < 1) {
			req.height = 1;
		}
		req.horiResolution = 0;
		req.vertResolution = 0;
		ObjectLocker lock(this);
		FT_Error err = FT_Request_Size(m_face, &req);
		return !err;
	}

	sl_bool FreeType::setRealSize(sl_real size)
	{
		return setRealSize(size, size);
	}

	sl_real FreeType::getFontHeight()
	{
		FT_Size_Metrics& m = m_face->size->metrics;
		return TO_REAL_POS(m.height);
	}

	Size FreeType::getCharExtent(sl_uint32 charcode)
	{
		ObjectLocker lock(this);
		FT_Error err = FT_Load_Char(m_face, (FT_ULong)charcode, FT_LOAD_BITMAP_METRICS_ONLY);
		if (!err) {
			sl_real dx = TO_REAL_POS(m_face->glyph->metrics.horiAdvance);
			sl_real dy = TO_REAL_POS(m_face->glyph->metrics.height);
			return Size(dx, dy);
		}
		return Size::zero();
	}

	Size FreeType::getGlyphExtent(sl_uint32 glyphId)
	{
		ObjectLocker lock(this);
		FT_Error err = FT_Load_Glyph(m_face, (FT_UInt)glyphId, FT_LOAD_BITMAP_METRICS_ONLY);
		if (!err) {
			sl_real dx = TO_REAL_POS(m_face->glyph->metrics.horiAdvance);
			sl_real dy = TO_REAL_POS(m_face->glyph->metrics.height);
			return Size(dx, dy);
		}
		return Size::zero();
	}

	Size FreeType::getStringExtent(const StringParam& _text)
	{
		StringData32 text(_text);
		sl_uint32 nChars = (sl_uint32)text.getLength();
		if (!nChars) {
			return Size(0, 0);
		}
		const sl_char32* chars = text.getData();

		ObjectLocker lock(this);

		sl_int32 sizeX = 0;
		sl_int32 sizeY = 0;
		for (sl_uint32 iChar = 0; iChar < nChars; iChar++) {
			FT_Error err = FT_Load_Char(m_face, (FT_ULong)(chars[iChar]), FT_LOAD_BITMAP_METRICS_ONLY);
			if (!err) {
				sl_int32 dx = (sl_int32)(m_face->glyph->metrics.horiAdvance);
				sl_int32 dy = (sl_int32)(m_face->glyph->metrics.height);
				sizeX += dx;
				if (!iChar || sizeY < dy) {
					sizeY = dy;
				}
			}
		}
		return Size(TO_REAL_POS(sizeX), TO_REAL_POS(sizeY));
	}

	void FreeType::drawString(const Ref<Image>& imageOutput, sl_int32 _x, sl_int32 _y, const StringParam& _text, const Color& color)
	{
		if (imageOutput.isNull()) {
			return;
		}
		sl_int32 widthImage = imageOutput->getWidth();
		sl_int32 heightImage = imageOutput->getHeight();
		if (widthImage <= 0 || heightImage <= 0) {
			return;
		}

		StringData32 text(_text);
		sl_uint32 nChars = (sl_uint32)(text.getLength());
		if (!nChars) {
			return;
		}
		const sl_char32* chars = text.getData();

		ObjectLocker lock(this);
		
		FT_GlyphSlot slot = m_face->glyph;
		sl_real x = (sl_real)_x;
		sl_real y = (sl_real)_y;
		for (sl_uint32 iChar = 0; iChar < nChars; iChar++) {
			FT_Error err = FT_Load_Char(m_face, (FT_ULong)(chars[iChar]), FT_LOAD_RENDER);
			if (!err) {
				sl_int32 dx = (sl_int32)x + slot->bitmap_left;
				sl_int32 dy = (sl_int32)y - slot->bitmap_top;
				sl_int32 widthChar = slot->bitmap.width;
				sl_int32 heightChar = slot->bitmap.rows;
				sl_int32 pitchChar = slot->bitmap.pitch;
				if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && slot->bitmap.pixel_mode != FT_PIXEL_MODE_MONO) {
					return;
				}
				if (widthChar > 0 && heightChar > 0 && dx < widthImage && dy < heightImage && dx > -widthChar && dy > -heightChar) {
					int sx = 0;
					int sy = 0;
					if (dx < 0) {
						sx -= dx;
						widthChar += dx;
						dx = 0;
					}
					if (dy < 0) {
						sy -= dy;
						heightChar += dy;
						dy = 0;
					}
					if (dx + widthChar > widthImage) {
						widthChar = widthImage - dx;
					}
					if (dy + heightChar > heightImage) {
						heightChar = heightImage - dy;
					}
					
					sl_uint8* bitmapChar = (sl_uint8*)(slot->bitmap.buffer) + (sy * pitchChar + sx);
					Color* colorsOutput = imageOutput->getColorsAt(dx, dy);
					sl_reg strideImage = imageOutput->getStride();

					sl_uint32 rs = color.r;
					sl_uint32 gs = color.g;
					sl_uint32 bs = color.b;
					sl_uint32 as = color.a;

					if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
						for (int y = 0; y < heightChar; y++) {
							sl_uint8* ps = bitmapChar;
							Color* pd = colorsOutput;
							for (int x = 0; x < widthChar; x++) {
								sl_uint32 _as = (as * (sl_uint32)(*ps)) / 255;
								if (_as == 255) {
									*pd = Color(rs, gs, bs);
								} else if (_as > 0) {
									pd->blend_NPA_NPA(rs, gs, bs, _as);
								}
								pd++;
								ps++;
							}
							bitmapChar += pitchChar;
							colorsOutput += strideImage;
						}
					} else {
						for (int y = 0; y < heightChar; y++) {
							sl_uint8* ps = bitmapChar;
							Color* pd = colorsOutput;
							for (int x = 0; x < widthChar; x++) {
								sl_uint8 B = (ps[x >> 3] >> (7 - (x & 7))) & 1;
								if (B) {
									*pd = Color(rs, gs, bs);
								}
								pd++;
							}
							bitmapChar += pitchChar;
							colorsOutput += strideImage;
						}
					}

				}

				x += TO_REAL_POS(slot->advance.x);
				y += TO_REAL_POS(slot->advance.y);
			}
		}
	}
	
	void FreeType::strokeString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& _text, const Color& color, sl_uint32 lineWidth)
	{
		StringData32 text(_text);
		_strokeString(imageOutput, x, y, text.getData(), (sl_uint32)(text.getLength()), sl_false, sl_false, lineWidth, color);
	}

	void FreeType::strokeStringInside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& _text, const Color& color, sl_uint32 lineWidth)
	{
		StringData32 text(_text);
		_strokeString(imageOutput, x, y, text.getData(), (sl_uint32)(text.getLength()), sl_true, sl_false, lineWidth, color);
	}

	void FreeType::strokeStringOutside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& _text, const Color& color, sl_uint32 lineWidth)
	{
		StringData32 text(_text);
		_strokeString(imageOutput, x, y, text.getData(), (sl_uint32)(text.getLength()), sl_true, sl_true, lineWidth, color);
	}

	void FreeType::_strokeString(const Ref<Image>& imageOutput, sl_int32 _x, sl_int32 _y, const sl_char32* chars, sl_uint32 nChars, sl_bool flagBorder, sl_bool flagOutside, sl_uint32 radius, const Color& color)
	{
		if (imageOutput.isNull()) {
			return;
		}
		sl_int32 widthImage = imageOutput->getWidth();
		sl_int32 heightImage = imageOutput->getHeight();
		if (widthImage <= 0 || heightImage <= 0) {
			return;
		}

		if (!nChars) {
			return;
		}

		ObjectLocker lock(this);
		
		FT_Stroker stroker = sl_null;
		FT_Stroker_New(LIBRARY, &stroker);
		if (!stroker) {
			return;
		}
		FT_Stroker_Set(stroker, radius * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

		sl_real x = (sl_real)_x;
		sl_real y = (sl_real)_y;
		for (sl_uint32 iChar = 0; iChar < nChars; iChar++) {
			FT_Error err = FT_Load_Char(m_face, (FT_ULong)(chars[iChar]), FT_LOAD_DEFAULT);
			if (!err) {
				FT_Glyph glyph = sl_null;
				err = FT_Get_Glyph(m_face->glyph, &glyph);
				if (!err && glyph) {
					if (flagBorder) {
						if (flagOutside) {
							err = FT_Glyph_StrokeBorder(&glyph, stroker, sl_false, sl_true);
						} else {
							err = FT_Glyph_StrokeBorder(&glyph, stroker, sl_true, sl_true);
						}
					} else {
						err = FT_Glyph_Stroke(&glyph, stroker, sl_true);
					}
					if (!err && glyph) {
						err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, sl_true);
						if (!err && glyph) {
							FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;
							sl_int32 dx = (sl_int32)x + bitmapGlyph->left;
							sl_int32 dy = (sl_int32)y - bitmapGlyph->top;
							sl_int32 widthChar = bitmapGlyph->bitmap.width;
							sl_int32 heightChar = bitmapGlyph->bitmap.rows;
							sl_int32 pitchChar = bitmapGlyph->bitmap.pitch;
							if (bitmapGlyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && bitmapGlyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO) {
								break;
							}
							if (widthChar > 0 && heightChar > 0 && dx < widthImage && dy < heightImage && dx > -widthChar && dy > -heightChar) {
								int sx = 0;
								int sy = 0;
								if (dx < 0) {
									sx -= dx;
									widthChar += dx;
									dx = 0;
								}
								if (dy < 0) {
									sy -= dy;
									heightChar += dy;
									dy = 0;
								}
								if (dx + widthChar > widthImage) {
									widthChar = widthImage - dx;
								}
								if (dy + heightChar > heightImage) {
									heightChar = heightImage - dy;
								}

								sl_uint8* bitmapChar = (sl_uint8*)(bitmapGlyph->bitmap.buffer) + (sy * pitchChar + sx);
								Color* colorsOutput = imageOutput->getColorsAt(dx, dy);
								sl_reg strideImage = imageOutput->getStride();

								sl_uint32 rs = color.r;
								sl_uint32 gs = color.g;
								sl_uint32 bs = color.b;
								sl_uint32 as = color.a;

								if (bitmapGlyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
									for (int y = 0; y < heightChar; y++) {
										sl_uint8* ps = bitmapChar;
										Color* pd = colorsOutput;
										for (int x = 0; x < widthChar; x++) {
											sl_uint32 _as = (as * (sl_uint32)(*ps)) / 255;
											if (_as == 255) {
												*pd = Color(rs, gs, bs);
											} else if (_as > 0) {
												pd->blend_NPA_NPA(rs, gs, bs, _as);
											}
											pd++;
											ps++;
										}
										bitmapChar += pitchChar;
										colorsOutput += strideImage;
									}
								} else {
									for (int y = 0; y < heightChar; y++) {
										sl_uint8* ps = bitmapChar;
										Color* pd = colorsOutput;
										for (int x = 0; x < widthChar; x++) {
											sl_uint8 B = (ps[x >> 3] >> (7 - (x & 7))) & 1;
											if (B) {
												*pd = Color(rs, gs, bs);
											}
											pd++;
										}
										bitmapChar += pitchChar;
										colorsOutput += strideImage;
									}
								}

							}
							x += TO_REAL_POS(glyph->advance.x);
							y += TO_REAL_POS(glyph->advance.y);
						}
					}
					if (glyph) {
						FT_Done_Glyph(glyph);
					}
				}
			}
		}

		FT_Stroker_Done(stroker);
	}

	Ref<FreeTypeGlyph> FreeType::getCharGlyph(sl_uint32 charcode)
	{
		ObjectLocker lock(this);
		sl_uint32 glyphId;
		if (getGlyphIndex(charcode, glyphId)) {
			return _getGlyph(glyphId);
		} else {
			return _getGlyph(charcode);
		}
	}

	Ref<FreeTypeGlyph> FreeType::getGlyph(sl_uint32 glyphId)
	{
		ObjectLocker lock(this);
		return _getGlyph(glyphId);
	}

	Ref<FreeTypeGlyph> FreeType::_getGlyph(sl_uint32 glyphId)
	{
		FT_Error err = FT_Load_Glyph(m_face, glyphId, FT_LOAD_NO_HINTING);
		if (!err) {
			Ref<FreeTypeGlyph> ret = new FreeTypeGlyph;
			FT_GlyphSlot slot = m_face->glyph;
			ret->outline = ConvertOutlineToPath(&(slot->outline));
			if (ret->outline.isNull()) {
				FT_Bitmap& bitmap = slot->bitmap;
				if (bitmap.width && bitmap.rows) {
					Ref<Image> image;
					if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
						image = Image::createCopyAlphaFromGray(bitmap.width, bitmap.rows, bitmap.buffer, 8, bitmap.pitch);
					} else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
						image = Image::createCopyAlphaFromGray(bitmap.width, bitmap.rows, bitmap.buffer, 1, bitmap.pitch);
					} else if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
						BitmapData bd;
						bd.format = BitmapFormat::BGRA_PA;
						bd.data = bitmap.buffer;
						bd.width = bitmap.width;
						bd.height = bitmap.rows;
						bd.pitch = bitmap.pitch;
						image = Image::create(bd);
					}
					if (image.isNotNull()) {
						ret->bitmap = Move(image);
						ret->bitmapLeft = (sl_int32)(slot->bitmap_left);
						ret->bitmapTop = (sl_int32)(slot->bitmap_top);
						ret->flagGrayBitmap = bitmap.pixel_mode != FT_PIXEL_MODE_BGRA;
					}
				}
			}
			ret->advance = TO_REAL_POS(slot->advance.x);
			ret->height = TO_REAL_POS(slot->metrics.height);
			return ret;
		}
		return sl_null;
	}

}
