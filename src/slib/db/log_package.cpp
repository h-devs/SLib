#include "slib/db/log_package.h"

#include "slib/core/mio.h"

namespace slib
{

	namespace {
		struct INDEX
		{
			sl_uint8 position[8];
			sl_uint8 size[8];
			sl_uint8 id[8];
			sl_uint8 reserved[8];
		};
	}

	LogPackageAppender::LogPackageAppender()
	{
	}

	LogPackageAppender::~LogPackageAppender()
	{
	}

	sl_bool LogPackageAppender::open(const StringParam& pathContent)
	{
		return open(pathContent, String::concat(pathContent, ".idx"));
	}

	sl_bool LogPackageAppender::open(const StringParam& pathContent, const StringParam& pathIndex)
	{
		m_fileContent = File::open(pathContent, FileMode::Append | FileMode::ShareAll);
		if (m_fileContent.isNone()) {
			return sl_false;
		}
		m_fileIndex = File::open(pathIndex, FileMode::Append | FileMode::ShareAll);
		if (m_fileIndex.isNone()) {
			return sl_false;
		}
		sl_uint64 pos = m_fileIndex.getPosition();
		if (pos & 31) { // sizeof(INDEX) == 32
			sl_uint64 nIndices = pos >> 5;
			if (!(m_fileIndex.seek(nIndices << 5, SeekPosition::Begin))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool LogPackageAppender::appendRecord(sl_uint64 id, const MemoryView& content)
	{
		sl_uint64 pos;
		if (!(m_fileContent.getPosition(pos))) {
			return sl_false;
		}
		if (m_fileContent.writeFully(content) != content.size) {
			return sl_false;
		}
		INDEX index = { 0 };
		MIO::writeUint64LE(index.position, pos);
		MIO::writeUint64LE(index.size, content.size);
		MIO::writeUint64LE(index.id, id);
		if (m_fileIndex.writeFully(&index, sizeof(index)) != sizeof(index)) {
			return sl_false;
		}
		return sl_true;
	}


	LogPackageReader::LogPackageReader(): m_indices(sl_null), m_nIndices(0)
	{
	}

	LogPackageReader::~LogPackageReader()
	{
		if (m_indices) {
			delete[] m_indices;
		}
	}

	sl_bool LogPackageReader::open(const StringParam& pathContent)
	{
		return open(pathContent, String::concat(pathContent, ".idx"));
	}

	sl_bool LogPackageReader::open(const StringParam& pathContent, const StringParam& pathIndex)
	{
		Memory memIndex = File::readAllBytes(pathIndex);
		if (memIndex.isNull()) {
			return sl_false;
		}
		m_fileContent = File::open(pathContent, FileMode::Read | FileMode::ShareAll);
		if (m_fileContent.isNone()) {
			return sl_false;
		}
		m_nIndices = memIndex.getSize() >> 5; // sizeof(INDEX) == 32
		if (!m_nIndices) {
			return sl_false;
		}
		m_indices = new Index[m_nIndices];
		if (!m_indices) {
			return sl_false;
		}
		INDEX* dataIndex = (INDEX*)(memIndex.getData());
		for (sl_size i = 0; i < m_nIndices; i++) {
			Index& index = m_indices[i];
			index.position = MIO::readUint64LE(dataIndex->position);
			index.size = MIO::readUint64LE(dataIndex->size);
			index.id = MIO::readUint64LE(dataIndex->id);
			dataIndex++;
		}
		return sl_true;
	}

	sl_size LogPackageReader::getRecordCount()
	{
		return m_nIndices;
	}

	Pair<sl_uint64, Memory> LogPackageReader::readRecordAt(sl_size n, sl_size maxSize)
	{
		Index& index = m_indices[n];
		if (index.size <= maxSize) {
			return { index.id, _readRecord(index.position, (sl_size)(index.size)) };
		}
		return { 0, sl_null };
	}

	Memory LogPackageReader::readRecord(sl_uint64 id, sl_size maxSize)
	{
		for (sl_size i = 0; i < m_nIndices; i++) {
			Index& index = m_indices[i];
			if (index.id == id) {
				if (index.size <= maxSize) {
					return _readRecord(index.position, (sl_size)(index.size));
				} else {
					return sl_null;
				}
			}
		}
		return sl_null;
	}

	List< Pair<sl_uint64, Memory> > LogPackageReader::readRecords(sl_uint64 start, sl_uint64 end, sl_size maxSize)
	{
		if (end == start) {
			end = start + 1;
		}
		List< Pair<sl_uint64, Memory> > ret;
		for (sl_size i = 0; i < m_nIndices; i++) {
			Index& index = m_indices[i];
			if (index.id >= start && index.id < end && index.size <= maxSize) {
				Memory mem = _readRecord(index.position, (sl_size)(index.size));
				if (mem.isNotNull()) {
					if (!(ret.add_NoLock(Pair<sl_uint64, Memory>(index.id, Move(mem))))) {
						return sl_null;
					}
				}
			}
		}
		return ret;
	}

	Memory LogPackageReader::_readRecord(sl_uint64 position, sl_size size)
	{
		Memory mem = Memory::create(size);
		if (mem.isNull()) {
			return sl_null;
		}
		if (m_fileContent.readFullyAt(position, mem.getData(), size) == size) {
			return mem;
		} else {
			return sl_null;
		}
	}

}
