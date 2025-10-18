#include "j-utils-archive.h"
#include "j-utils-fs.h"

#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>
#include <sys/stat.h>

namespace utils {

	namespace archive {
	
		void compress(std::filesystem::path inputPath, std::filesystem::path outFilePath) {
			// set up the archive writer
			struct archive* writer = archive_write_new();
			if (!writer) {
				throw std::runtime_error("Error creating archive writer");
			}
			
			// set the compression filter to GZip
			int returnCode = archive_write_add_filter_gzip(writer);
			if (returnCode != ARCHIVE_OK) {
				archive_write_free(writer);
				throw std::runtime_error("Error setting GZip compression filter");
			}

			// set the archive format to PAX
			returnCode = archive_write_set_format_pax_restricted(writer);
			if (returnCode != ARCHIVE_OK) {
				archive_write_free(writer);
				throw std::runtime_error("Error setting TAR format to PAX");
			}
			
			// open the output file for writing
			returnCode = archive_write_open_filename(writer, outFilePath.string().c_str());
			if (returnCode != ARCHIVE_OK) {
				archive_write_free(writer);
				throw std::runtime_error("Error opening output archive file for writing");
			}

			// set up the disk reader for traversing the directory
			struct archive* diskReader = archive_read_disk_new();
			if (!diskReader) {
				archive_write_close(writer);
				archive_write_free(writer);
				throw std::runtime_error("Error creating disk reader instance");
			}
			
			// default behavior for the disk reader
			archive_read_disk_set_standard_lookup(diskReader);

			// loop through the directory and add entries to the archive
			archive_entry* entry = archive_entry_new();
			if (!entry) {
				archive_read_free(diskReader);
				archive_write_close(writer);
				archive_write_free(writer);
				throw std::runtime_error("Error creating archive root entry");
			}
			
			// traverse the input directory
			returnCode = archive_read_disk_open(diskReader, inputPath.c_str());
			
			if (returnCode != ARCHIVE_OK) {
				throw std::runtime_error("Error opening input path for reading");
			} else {
				// read each item (file/directory) from the disk reader
				while (archive_read_next_header2(diskReader, entry) == ARCHIVE_OK) {
					// get the path relative to the original inputPath
					const char* pathAbsolute = archive_entry_pathname(entry);

					std::string pathRelative = utils::fs::relativePath(
						inputPath,
						pathAbsolute
					);
					
					// rewrite the path in the archive to be relative (e.g., remove the full path)
					archive_entry_set_pathname(entry, pathRelative.c_str());

					// preserve metadata
					returnCode = archive_read_disk_descend(diskReader);

					// write the entry header to the archive
					returnCode = archive_write_header(writer, entry);
					
					if (returnCode == ARCHIVE_OK && archive_entry_size(entry) != 0) {
						// regular file, copy its content
						size_t len;
						const void* buf;
						size_t offset;
							
						// loop to copy the file content block by block
						while ((len = archive_read_data(diskReader, &buf, offset)) > 0) {
							if (archive_write_data(writer, buf, len) != len) {
								// write error
								throw std::runtime_error("Error writing file to archive!");
							}
						}
					}

					archive_entry_clear(entry);
				}
			}
			
			// cleanup
			archive_entry_free(entry);
			archive_read_free(diskReader);
			archive_write_close(writer);
			archive_write_free(writer);
		}

	}

}