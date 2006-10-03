/*
 * libopenraw - arwfile.h
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




#ifndef __ARWFILE_H_
#define __ARWFILE_H_

#include "rawfile.h"

namespace OpenRaw {

	class Thumbnail;

	namespace Internals {
		class IOFile;
		class IFDFileContainer;

		class ARWFile
			: public OpenRaw::RawFile
		{
		public:
			static RawFile *factory(const char* _filename);
			ARWFile(const char* _filename);
			virtual ~ARWFile();

		protected:
			/** get the small size thumbnail */
			virtual bool _getSmallThumbnail(Thumbnail & thumbnail);
			virtual bool _getLargeThumbnail(Thumbnail & thumbnail);
			/** get the preview */
			virtual bool _getPreview(Thumbnail & thumbnail);
		private:

			ARWFile(const ARWFile&);
			ARWFile & operator=(const ARWFile&);

			IOFile *m_io; /**< the IO handle */
			IFDFileContainer *m_container; /**< the real container */
		};
	}

}

#endif