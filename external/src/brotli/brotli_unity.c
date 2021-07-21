#include "common/constants.c"
#include "common/context.c"
#include "common/dictionary.c"
#include "common/platform.c"
#include "common/transform.c"

#include "dec/bit_reader.c"
#include "dec/decode.c"
#include "dec/huffman.c"
#include "dec/state.c"

#include "enc/encode.c"
#include "enc/backward_references.c"
#include "enc/backward_references_hq.c"
#include "enc/bit_cost.c"
#include "enc/block_splitter.c"
#include "enc/brotli_bit_stream.c"
#include "enc/cluster.c"
#include "enc/command.c"
#define kHashMul32 kHashMul32_
#include "enc/compress_fragment.c"
#undef kHashMul32
#define kHashMul32 kHashMul32__
#define Hash Hash_
#define HashBytesAtOffset HashBytesAtOffset_
#define IsMatch IsMatch_
#define BuildAndStoreCommandPrefixCode BuildAndStoreCommandPrefixCode_
#define EmitInsertLen EmitInsertLen_
#define EmitCopyLen EmitCopyLen_
#define EmitCopyLenLastDistance EmitCopyLenLastDistance_
#define EmitDistance EmitDistance_
#define BrotliStoreMetaBlockHeader BrotliStoreMetaBlockHeader_
#undef MIN_RATIO
#define RewindBitPosition RewindBitPosition_
#define EmitUncompressedMetaBlock EmitUncompressedMetaBlock_
#define ShouldCompress ShouldCompress_
#include "enc/compress_fragment_two_pass.c"
#include "enc/dictionary_hash.c"
#include "enc/encoder_dict.c"
#define SortHuffmanTree SortHuffmanTree_
#define BrotliReverseBits BrotliReverseBits_
#include "enc/entropy_encode.c"
#include "enc/fast_log.c"
#include "enc/histogram.c"
#include "enc/literal_cost.c"
#include "enc/memory.c"
#include "enc/metablock.c"
#undef Hash
#define Hash Hash__
#undef IsMatch
#define IsMatch IsMatch__
#include "enc/static_dict.c"
#include "enc/utf8_util.c"
