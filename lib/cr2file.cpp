/*
 * libopenraw - cr2file.cpp
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


#include <libopenraw/libopenraw.h>

#include "debug.h"
#include "iofile.h"
#include "ifdfilecontainer.h"
#include "ifd.h"
#include "thumbnail.h"
#include "cr2file.h"

#include "rawfilefactory.h"

using namespace Debug;

namespace OpenRaw {

	namespace Internals {

		RawFile *CR2File::factory(const char* _filename)
		{
			return new CR2File(_filename);
		}

		CR2File::CR2File(const char* _filename)
			: RawFile(_filename, OR_RAWFILE_TYPE_CR2),
				m_io(new IOFile(_filename)),
				m_container(new IFDFileContainer(m_io, 0))
		{

		}

		CR2File::~CR2File()
		{
			delete m_container;
			delete m_io;
		}

		bool CR2File::_getSmallThumbnail(Thumbnail & thumbnail)
		{
			bool success;
			int c = m_container->countDirectories();
			if (c < 2) {
				return false;
			}
			IFDDir::Ref dir = m_container->setDirectory(1);
			if (dir == NULL) {
				Trace(WARNING) << "dir NULL\n";
				return false;
			}

			long offset = 0;
			success = dir->getLongValue(IFD::EXIF_TAG_JPEG_INTERCHANGE_FORMAT,
																	offset);				
			long size = 0;
			success = dir->getLongValue(IFD::EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH,
																	size);

			void *buf = thumbnail.allocData(size);

			size_t real_size = m_container->fetchData(buf, offset, size);
			if (real_size != size) {
				Trace(WARNING) << "wrong size\n";
			}
			thumbnail.setDataType(OR_DATA_TYPE_JPEG);
			thumbnail.setDimensions(160, 120);
			return true;
		}


		bool CR2File::_getLargeThumbnail(Thumbnail & thumbnail)
		{
			int c = m_container->countDirectories();
			if (c < 3) {
				return false;
			}
			IFDDir::Ref dir = m_container->setDirectory(2);
			if (dir == NULL) {
				Trace(WARNING) << "dir NULL\n";
				return false;
			}

			bool success;
			long offset = 0;
			success = dir->getLongValue(IFD::EXIF_TAG_STRIP_OFFSETS, offset);
			long size = 0;
			success = dir->getLongValue(IFD::EXIF_TAG_STRIP_BYTE_COUNTS, size);

			short x = 0;
			short y = 0;
			success = dir->getShortValue(IFD::EXIF_TAG_IMAGE_WIDTH, x);
			success = dir->getShortValue(IFD::EXIF_TAG_IMAGE_LENGTH, y);
			
			Trace(DEBUG1) << "x, y " << x << " " << y << "\n";
			void *buf = thumbnail.allocData(size);

			size_t real_size = m_container->fetchData(buf, offset, size);
			if (real_size != size) {
				Trace(WARNING) << "wrong size\n";
			}
			thumbnail.setDataType(OR_DATA_TYPE_PIXMAP_8RGB);
			thumbnail.setDimensions(x, y);
			return true;
		}

		bool CR2File::_getPreview(Thumbnail & thumbnail)
		{
			return false;
		}
	}
}