/*
 * Copyright 2011-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#ifndef ARX_IO_SAVEBLOCK_H
#define ARX_IO_SAVEBLOCK_H

#include <stddef.h>
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <vector>

#include "platform/Platform.h"
#include "io/fs/FilePath.h"
#include "io/fs/FileStream.h"

/*!
 * Interface to read and write save block files. (used for savegames)
 */
class SaveBlock {
	
private:
	
	struct File {
		
		struct Chunk {
			
			size_t size;
			size_t offset;
			
			Chunk() : size(0), offset(0) { }
			Chunk(size_t _size, size_t _offset) : size(_size), offset(_offset) { }
			
		};
		
		typedef std::vector<Chunk> ChunkList;
		
		enum Compression {
			Unknown,
			None,
			ImplodeCrypt,
			Deflate
		};
		
		size_t storedSize;
		size_t uncompressedSize;
		ChunkList chunks;
		Compression comp;
		
		const char * compressionName() const;
		
		bool loadOffsets(std::istream & handle, u32 version);
		
		void writeEntry(std::ostream & handle, std::string_view name) const;
		
		std::string loadData(std::istream & handle, std::string_view name) const;
		
	};
	
	typedef std::map<std::string, File, std::less<>> Files; // TODO switch back to std::unordered_map with C++20
	
	fs::path m_savefile;
	fs::fstream m_handle;
	size_t m_totalSize;
	size_t m_usedSize;
	size_t m_chunkCount;
	Files m_files;
	
	bool defragment();
	bool loadFileTable();
	void writeFileTable(std::string_view important);
	
public:
	
	explicit SaveBlock(fs::path savefile);
	
	/*!
	 * Destructor: this will not finalize the save block.
	 * 
	 * If the SaveBlock vas changed (via save()) and not flushed since, the save fill will be corrupted.
	 */
	~SaveBlock() = default;
	
	/*!
	 * Open a save block.
	 * \param writable must be true if the block is going to be changed
	 */
	bool open(bool writable = false);
	
	/*!
	 * Finalize the save block: defragment if needed and write the file table.
	 */
	bool flush(std::string_view important);
	
	/*!
	 * Save a file to the save block.
	 * This only writes the file data and does not add the file to the file table.
	 * Also, it may destroy any previous on-disk file table.
	 * flush() should be called before destructing this SaveBlock instance
	 */
	bool save(std::string && name, const char * data, size_t size);
	bool save(std::string_view name, const char * data, size_t size) {
		return save(std::string(name), data, size);
	}
	bool save(const char * name, const char * data, size_t size) {
		return save(std::string(name), data, size);
	}
	
	/*!
	 * Remove a file from the save block.
	 */
	void remove(std::string_view name);
	
	std::string load(std::string_view name);
	bool hasFile(std::string_view name) const;
	
	std::vector<std::string_view> getFiles() const;
	
	/*!
	 * Load a single file from the save block.
	 * 
	 * This is semantically equivalent to, but hopefully faster than:
	 * <pre>
	 *  SaveBlock block(savefile);
	 *  return block.open(false) ? block.load(name, size) : nullptr
	 * </pre>
	 * 
	 * This is optimized for loading the file named in flush().
	 * 
	 * \param savefile the save block to load from
	 * \param name the file to load
	 */
	static std::string load(const fs::path & savefile, std::string_view name);
	
};

#endif // ARX_IO_SAVEBLOCK_H
