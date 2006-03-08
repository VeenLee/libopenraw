/*
 * libopenraw - cr2.c
 *
 * Copyright (C) 2005-2006 Hubert Figuiere
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

#include <tiffio.h>
#include <stdlib.h>
#include <libexif/exif-loader.h>

#include "cr2.h"


or_error cr2_get_thumbnail(RawFileRef raw_file, ORThumbnailRef thumbnail)
{
	or_error err = OR_ERROR_NONE;	

	long byte_count = 0;
	long offset = 0;

	ExifLoader *loader;
	ExifData   *exifData;
	char str[128];

	loader = exif_loader_new();
	exif_loader_write_file(loader, raw_get_path(raw_file));
	exifData = exif_loader_get_data(loader);
	exif_loader_unref(loader);

	
	ExifEntry *offset_entry = exif_content_get_entry(exifData->ifd[1],
													 EXIF_TAG_JPEG_INTERCHANGE_FORMAT);
	exif_entry_get_value(offset_entry, (char*)&offset, sizeof(offset));
	exif_entry_unref(offset_entry);

	ExifEntry *byte_count_entry = exif_content_get_entry(exifData->ifd[1],
														 EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH);
	exif_entry_get_value(byte_count_entry, str, 
						 sizeof(str));
	exif_entry_unref(offset_entry);
	fprintf(stderr, "offset = %s\n", str);
	
	fprintf(stderr, "%lx, %lx\n", offset,  byte_count);
	raw_seek(raw_file, offset, SEEK_SET);
	thumbnail->data = malloc(byte_count);
	raw_read(raw_file, thumbnail->data, byte_count); 
	thumbnail->data_size = byte_count;

	thumbnail->data_type = OR_DATA_TYPE_JPEG;
	thumbnail->thumb_size = OR_THUMB_SIZE_SMALL;
	thumbnail->x = 160;
	thumbnail->y = 120;

	exif_data_unref(exifData);


#if 0
	TIFF *tif = raw_tiff_open(raw_file);
	if (tif == NULL) {
		/*err = ;*/
	}
	else {		
		long offset, bytes;
		tdir_t n = TIFFNumberOfDirectories(tif);
		fprintf(stderr, "num of IFD = %ld\n", n);
		TIFFSetDirectory(tif, 1);
		
		offset = TIFFGetField(tif, TIFFTAG_JPEGIFOFFSET);
		bytes = TIFFGetField(tif, TIFFTAG_JPEGIFBYTECOUNT);
		
		fprintf(stderr, "%ld, %ld\n", offset,  bytes);
		raw_seek(raw_file, offset, SEEK_SET);
		thumbnail->data = malloc(bytes);
		raw_read(raw_file, thumbnail->data, bytes); 
		thumbnail->data_size = bytes;
		thumbnail->data_type = OR_DATA_TYPE_JPEG;
		thumbnail->thumb_size = OR_THUMB_SIZE_SMALL;
		thumbnail->x = 160;
		thumbnail->y = 120;

		TIFFPrintDirectory(tif, stderr, 0);
		TIFFClose(tif);
	}
#endif

	return err;

}

