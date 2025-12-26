#include "CoalesceAllocator.h"
#include "Common.h"

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)

#define VALIDATE_BLOCK(block, free)                                                         \
    do {                                                                                    \
        ASSERT((block)->markerStart == FEEDFACE);                                           \
        ASSERT((block)->markerEnd == FEEDFACE);                                             \
        BlockEnd* blockEnd = (BlockEnd*)((BYTE*)(block) + (block)->size - sizeof(BlockEnd));\
        ASSERT(blockEnd->markerEnd == FEEDFACE);                                            \
        ASSERT(blockEnd->markerEnd == FEEDFACE);                                            \
        if (free) {                                                                         \
                ASSERT((block)->alloc == 0);                                                \
                ASSERT(*((uint32*)((BYTE*)(block) + sizeof(BlockStart))) == DEADBEEF);      \
        }                                                                                   \
        else {                                                                              \
                ASSERT((block)->alloc);                                                     \
        }                                                                                   \
    } while (0)

#else
#define VALIDATE_BLOCK(block, free) do {} while(0)
#endif

#include <cmath>
#include <algorithm>

#include "BitOps.h"

namespace CoalesceAllocator {
	CoalesceAllocator::CoalesceAllocator() :
		m_headPage(nullptr)
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		, m_StatReport{}
#endif
	{ }

	CoalesceAllocator::~CoalesceAllocator() {
		if (m_headPage != nullptr)
			destroy();
	}

	void CoalesceAllocator::init() {
		if (m_headPage != nullptr)
			return;

		uint32 binIdx;
		m_headPage = createPage(binIdx);
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		m_StatReport.pagesCount++;
#endif
	}

	void CoalesceAllocator::destroy() {
		ASSERT(m_headPage != nullptr);

		while (m_headPage) {
			Page* next = m_headPage->next;

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
			uint32 fxIdx = binIndex(sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd));
			ASSERT(m_headPage->fh[fxIdx]->size == sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd));
			ASSERT(m_headPage->fh[fxIdx]->next == nullptr);
#endif

			if (!VirtualFree(m_headPage, 0, MEM_RELEASE)) {
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
				printf("VirtualFree failed.\n");
#endif
				return;
			}

			m_headPage = next;
		}
	}

	void* CoalesceAllocator::alloc(uint32 size) {
		ASSERT(m_headPage != nullptr);

		if (size > PAGE_SIZE)
			return nullptr;

		Page* page = m_headPage;

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		m_StatReport.allocCallCount++;
		m_StatReport.totalAllocSize += size;
#endif

		while (true) {
			//<--------(fb->size)-------->
			// ↓(fb)
			//[BlockStart][size][BlockEnd]
			uint32 binIdx;
			BlockStart* fb = findFreeBlock(page, sizeof(BlockStart) + size + sizeof(BlockEnd), binIdx);

			if (fb != nullptr) {
				ASSERT(fb->size >= sizeof(BlockStart) + size + sizeof(BlockEnd));
				VALIDATE_BLOCK(fb, true);

				if (fb->next) fb->next->prev = fb->prev;
				if (fb->prev) fb->prev->next= fb->next;
				else page->fh[binIdx] = fb->next;

				// <----------------------(fb->size)-------------------------->
				//   ↓(fb)                         ↓(nfb)
				// <[BlockStart][size][BlockEnd]> <[BlockStart][...][BlockEnd]>
				if (fb->size >= size + 2 * sizeof(BlockStart) + 2 * sizeof(BlockEnd)) {
					auto* nfb = (BlockStart*)((BYTE*)fb + sizeof(BlockStart) + size + sizeof(BlockEnd));
					uint32 nfbSize = fb->size - size - sizeof(BlockStart) - sizeof(BlockEnd);
					fb->size -= nfbSize;

					uint32 nfdBinIdx = binIndex(nfbSize);
					ASSERT(page->fh[nfdBinIdx] == nullptr || page->fh[nfdBinIdx]->prev == nullptr);
					setupBlock(nfb, nfbSize, page->fh[nfdBinIdx], nullptr, true);
					page->fh[nfdBinIdx] = nfb;
				}

				setupBlock(fb, fb->size, nullptr, nullptr, false);
				return (BYTE*)fb + sizeof(BlockStart);
			}

			if (page->next == nullptr)
				break;

			page = page->next;
		}

		uint32 binIdx;
		page->next = createPage(binIdx);
		page = page->next;

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		m_StatReport.pagesCount++;
#endif

		uint32 pageSize = page->fh[binIdx]->size;
		if (pageSize >= size + 2 * sizeof(BlockStart) + 2 * sizeof(BlockEnd)) {
			uint32 nfbSize = pageSize - sizeof(BlockStart) - size - sizeof(BlockEnd);
			pageSize -= nfbSize;
			auto* nfb = (BlockStart*)((BYTE*)page + sizeof(Page) + pageSize);
			setupBlock(nfb, nfbSize, nullptr, nullptr, true);
			page->fh[binIdx] = nullptr;
			page->fh[binIndex(nfb->size)] = nfb;
		}
		else {
			page->fh[binIdx] = nullptr;
		}

		setupBlock(((BlockStart*)((BYTE*)page + sizeof(Page))), pageSize, nullptr, nullptr, false);
		return (BYTE*)page + sizeof(Page) + sizeof(BlockStart);
	}

	//             ↓(p)
	// [BlockStart][......][BlockEnd]
	void CoalesceAllocator::free(void* p) {
		ASSERT(m_headPage != nullptr);

		Page* page = m_headPage;
		while (page != nullptr) {
			// ↓(page)
			//[Page][BlockStart][..(p)..][BlockEnd]
			if (insidePage(page, p)) {
				VALIDATE_BLOCK((BlockStart*)((BYTE*)p - sizeof(BlockStart)), false);
				break;
			}
			page = page->next;
		}

		if (page == nullptr)
		{
			ASSERT(false);
			return;
		}

		auto* pageStart = (BYTE*)page + sizeof(Page);

		auto* cb = (BlockStart*)((BYTE*)p - sizeof(BlockStart));
		size_t lbs = (BYTE*)cb == pageStart ? 0 : ((BlockEnd*)((BYTE*)cb - sizeof(BlockEnd)))->size;
		auto* lb = (BlockStart*)((BYTE*)cb - lbs);
		auto* rb = (BlockStart*)((BYTE*)cb + cb->size);

		if (!insidePage(page, (BYTE*)lb + sizeof(BlockStart)) || lb->alloc)
			lb = nullptr;
		if (!insidePage(page, (BYTE*)rb + sizeof(BlockStart)) || rb->alloc)
			rb = nullptr;

		if (lb != nullptr) {
			VALIDATE_BLOCK(lb, true);

			if (lb->next) lb->next->prev = lb->prev;
			if (lb->prev) lb->prev->next = lb->next;
			else
			{
				ASSERT(page->fh[binIndex(lb->size)] == lb);
				page->fh[binIndex(lb->size)] = lb->next;
			}

			lb->size += cb->size;
			cb = lb;
		}

		if (rb != nullptr) {
			VALIDATE_BLOCK(rb, true);

			cb->size += rb->size;

			if (rb->next) rb->next->prev = rb->prev;
			if (rb->prev) rb->prev->next = rb->next;
			else
			{
				ASSERT(page->fh[binIndex(rb->size)] == rb);
				page->fh[binIndex(rb->size)] = rb->next;
			}
		}

		uint32 cbBinIdx = binIndex(cb->size);

		if (page->fh[cbBinIdx])
		{
			ASSERT(page->fh[cbBinIdx]->prev == nullptr);
			page->fh[cbBinIdx]->prev = cb;
		}

		cb->next = page->fh[cbBinIdx];
		cb->prev = nullptr;
		page->fh[cbBinIdx] = cb;

		setupBlock(cb, cb->size, cb->next, cb->prev, true);
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		m_StatReport.freeCallCount++;
#endif
	}

	CoalesceAllocator::Page* CoalesceAllocator::createPage(uint32& outBinIdx) {
		Page* page = (Page*)VirtualAlloc(nullptr, sizeof(Page) + sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd),
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if (page == nullptr) {
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
			printf("VirtualAlloc failed.\n");
#endif
			return nullptr;
		}

		page->next = nullptr;
		memset(page->fh, 0, sizeof(BlockStart*) * NUM_BINS);

		outBinIdx = binIndex(sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd));
		page->fh[outBinIdx] = (BlockStart*)((BYTE*)page + sizeof(Page));
		setupBlock(page->fh[outBinIdx], sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd), nullptr, nullptr, true);

		return page;
	}

	uint32 CoalesceAllocator::binIndex(uint32 size)
	{
		ASSERT(size > 0);
		return std::min(BitOps::log2_floor(size), NUM_BINS - 1);
	}

	CoalesceAllocator::BlockStart* CoalesceAllocator::findFreeBlock(const CoalesceAllocator::Page* page, uint32 size, uint32& outBinIdx) {
		for (outBinIdx = binIndex(size); outBinIdx < NUM_BINS; ++outBinIdx) {
			if (BlockStart* output = page->fh[outBinIdx]) {
				while (output && output->size < size) output = output->next;
				if (output)
					return output;
			}
		}

		outBinIdx = -1;
		return nullptr;
	}

	void CoalesceAllocator::setupBlock(BlockStart* block, uint32 size, BlockStart* next, BlockStart* prev, bool free) {
		block->alloc = free ? 0 : 1;
		block->size = size;
		block->next = next;
		block->prev = prev;
		if (block->next) block->next->prev = block;
		if (block->prev) block->prev->next = block;

		auto* end = (BlockEnd*)((BYTE*)block + size - sizeof(BlockEnd));
		end->size = size;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
		block->markerStart = FEEDFACE;
		block->markerEnd = FEEDFACE;
		end->markerStart = FEEDFACE;
		end->markerEnd = FEEDFACE;

		if (free) {
			*((uint32*)((BYTE*)block + sizeof(BlockStart))) = DEADBEEF;
		}
#endif
	}

	bool CoalesceAllocator::containsAddress(void* p) const {
		Page* page = m_headPage;
		while (page) {
			if (insidePage(page, p))
				return true;

			page = page->next;
		}

		return false;
	}

	bool CoalesceAllocator::insidePage(Page* page, void* p) {
		return ((BYTE*)p >= (BYTE*)page + sizeof(Page) + sizeof(BlockStart) &&
			(BYTE*)p <= (BYTE*)page + sizeof(Page) + PAGE_SIZE + sizeof(BlockStart));
	}

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
	BlockReport CoalesceAllocator::getNextBlock(uint32 pageNum, void* from) const {
		ASSERT(m_headPage != nullptr);

		Page* page = m_headPage;
		for (int i = 0; i < pageNum && page; ++i, page = page->next);

		if (page == nullptr)
			return BlockReport{};

		if (!from) {
			auto* block = (BlockStart*)((BYTE*)page + sizeof(Page));
			return BlockReport{
					(BYTE*)block + sizeof(BlockStart),
					block->size - (uint32)sizeof(BlockStart) - (uint32)sizeof(BlockEnd),
					block->alloc != 0,
			};
		}

		ASSERT(insidePage(page, from));

		auto* fromBlock = (BlockStart*)((BYTE*)from - sizeof(BlockStart));

		auto* next = (BlockStart*)((BYTE*)fromBlock + fromBlock->size);

		if (!insidePage(page, next))
			return BlockReport{};

		return BlockReport{
				(BYTE*)next + sizeof(BlockStart),
			next->size - (uint32)sizeof(BlockStart) - (uint32)sizeof(BlockEnd),
			next->alloc != 0,
		};
	}

#endif // DEBUG
}
