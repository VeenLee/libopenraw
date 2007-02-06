/*
 * libopenraw - ifdfile.cpp
 *
 * Copyright (C) 2006 Hubert Figuiere
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <libopenraw++/thumbnail.h>

#include "debug.h"
#include "io/stream.h"
#include "io/streamclone.h"
#include "io/file.h"
#include "ifd.h"
#include "ifdfile.h"
#include "ifdfilecontainer.h"
#include "jfifcontainer.h"

using namespace Debug;

namespace OpenRaw {
	namespace Internals {


		IFDFile::IFDFile(const char *_filename, Type _type)
			: RawFile(_filename, _type),
				m_thumbLocations(),
				m_io(new IO::File(_filename)),
				m_container(new IFDFileContainer(m_io, 0))
		{

		}

		IFDFile::~IFDFile()
		{
			delete m_container;
			delete m_io;
		}

		::or_error IFDFile::_enumThumbnailSizes(std::vector<uint32_t> &list)
		{
			::or_error err = OR_ERROR_NONE;

			Trace(DEBUG1) << "_enumThumbnailSizes()\n";
			std::vector<IFDDir::Ref> & dirs = m_container->directories();
			std::vector<IFDDir::Ref>::iterator iter; 
			
			Trace(DEBUG1) << "num of dirs " << dirs.size() << "\n";
			int c = 0;
			for(iter = dirs.begin(); iter != dirs.end(); ++iter, ++c)
			{
				IFDDir::Ref & dir = *iter;
				dir->load();
				or_error ret = _locateThumbnail(dir, list);
				if (ret == OR_ERROR_NONE)
				{
					Trace(DEBUG1) << "Found " << list.back() << " pixels\n";
				}
			}
			if (list.size() <= 0) {
				err = OR_ERROR_NOT_FOUND;
			}
			return err;
		}


		::or_error IFDFile::_locateThumbnail(const IFDDir::Ref & dir,
																	 std::vector<uint32_t> &list)
		{
			::or_error ret = OR_ERROR_NOT_FOUND;
			bool got_it;
			uint32_t x = 0;
			uint32_t y = 0;
			::or_data_type type = OR_DATA_TYPE_NONE;
			uint32_t subtype = 0;

			Trace(DEBUG1) << "_locateThumbnail\n";

			got_it = dir->getValue(IFD::EXIF_TAG_NEW_SUBFILE_TYPE, subtype);
			Trace(DEBUG1) << "subtype " << subtype  << "\n";
			if (!got_it || (subtype == 1)) {

				uint16_t photom_int = 0;
				got_it = dir->getValue(IFD::EXIF_TAG_PHOTOMETRIC_INTERPRETATION, 
																		photom_int);

				if (got_it) {
					Trace(DEBUG1) << "photometric int " << photom_int  << "\n";
				}
				// photometric interpretation is RGB
				if (!got_it || (photom_int == 2)) {

					got_it = dir->getIntegerValue(IFD::EXIF_TAG_IMAGE_WIDTH, x);
					got_it = dir->getIntegerValue(IFD::EXIF_TAG_IMAGE_LENGTH, y);

					uint32_t offset = 0;
					got_it = dir->getValue(IFD::EXIF_TAG_STRIP_OFFSETS, offset);
					if (!got_it) {
						got_it = dir->getValue(IFD::EXIF_TAG_JPEG_INTERCHANGE_FORMAT,
																			 offset);
 						Trace(DEBUG1) << "looking for JPEG at " << offset << "\n";
						if (got_it) {
							type = OR_DATA_TYPE_JPEG;
							if (x == 0 || y == 0) {
								IO::StreamClone *s = new IO::StreamClone(m_io, offset);
								JFIFContainer *jfif = new JFIFContainer(s, 0);
								if (jfif->getDimensions(x,y)) {
									Trace(DEBUG1) << "JPEG dimensions x=" << x 
																<< " y=" << y << "\n";
								}
								else {
									type = OR_DATA_TYPE_NONE;
								}
								delete jfif;
								delete s;
							}
						}
					}
					else {
						Trace(DEBUG1) << "found strip offsets\n";
						if (x != 0 && y != 0) {
							type = OR_DATA_TYPE_PIXMAP_8RGB;
						}
					}
					if(type != OR_DATA_TYPE_NONE) {
						uint32_t dim = std::max(x, y);
						m_thumbLocations[dim] = IFDThumbDesc(x, y, type, dir);
						list.push_back(dim);
						ret = OR_ERROR_NONE;
					}
				}
			}

			return ret;
		}


		::or_error IFDFile::_getThumbnail(uint32_t size, Thumbnail & thumbnail)
		{
			::or_error ret = OR_ERROR_NOT_FOUND;
			ThumbLocations::iterator iter = m_thumbLocations.find(size);
			if(iter != m_thumbLocations.end()) 
			{
				bool got_it;

				IFDThumbDesc & desc = iter->second;
				thumbnail.setDataType(desc.type);
				uint32_t byte_length= 0; /**< of the buffer */
				uint32_t offset = 0;
				uint32_t x = desc.x;
				uint32_t y = desc.y;

				switch(desc.type)
				{
				case OR_DATA_TYPE_JPEG:
					got_it = desc.ifddir
						->getValue(IFD::EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH,
													 byte_length);
					got_it = desc.ifddir
						->getValue(IFD::EXIF_TAG_JPEG_INTERCHANGE_FORMAT,
													 offset);
					break;
				case OR_DATA_TYPE_PIXMAP_8RGB:
					got_it = desc.ifddir
						->getValue(IFD::EXIF_TAG_STRIP_OFFSETS, offset);
					got_it = desc.ifddir
						->getValue(IFD::EXIF_TAG_STRIP_BYTE_COUNTS, byte_length);

					got_it = desc.ifddir
						->getIntegerValue(IFD::EXIF_TAG_IMAGE_WIDTH, x);
					got_it = desc.ifddir
						->getIntegerValue(IFD::EXIF_TAG_IMAGE_LENGTH, y);
					break;
				default:
					break;
				}
				if (byte_length != 0) {
					void *p = thumbnail.allocData(byte_length);
					size_t real_size = m_container->fetchData(p, offset, 
																										byte_length);
					if (real_size < byte_length) {
						Trace(WARNING) << "Size mismatch for data: ignoring.\n";
					}

					thumbnail.setDimensions(x, y);
					ret = OR_ERROR_NONE;
				}
			}

			return ret;
		}


		::or_error IFDFile::_getRawData(RawData & data)
		{
			::or_error ret = OR_ERROR_NOT_FOUND;
			
			return ret;
		}

	}
}
