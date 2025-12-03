#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "mediabuffer.h"

#define NUMBYTES_ALIGNMENT	(4)	
#define FRAMER_HDR_MAGICNUM	(0x01234567)

typedef struct {
	int usShmId;
	void *pvShmAddr;
	uint32_t u32ShmSize;
	VV_RB_MEDIA_ROLES role;

	/* Only use for CONSUMER, preallocate memory, purpose avoid calls mallc()/free() many times */
	uint8_t *pBuffersPreallocate;
	uint32_t buffersPreallocateSize;

	/* Those variables are used for memory management */
	uint32_t u32MemSize;
	uint8_t *pu8HeadAddr;
	uint8_t *pu8TailAddr;
	uint8_t *pu8CurrAddr;
} RB_SHM_CONTEXT_S;

typedef struct {
	uint32_t u32ShmSize;
	uint32_t u32CommitId; /* Track to the current index in array has written data recently */
	uint32_t u32TracksId; /* Using to track index has been written recently */
} RB_SHM_HDR_S;

typedef struct {
	uint32_t id;
	uint32_t u32NextId;
	uint32_t u32PrevId;
	uint32_t u32Length;
	uint32_t u32Magicnum;
	uint32_t u32Checksum;
	uint64_t u64Timestamp;
	uint32_t u32FramePerSeconds;
	VV_RB_MEDIA_FRAME_HDR_TYPE type;
    VV_RB_MEDIA_ENCODE_TYPE encoder;
} RB_FRAMER_HDR_S;

static inline void assignHdrId(void *pvShmAddr, uint32_t u32CommitId);
static inline uint32_t readHdrId(void *pvShmAddr);
static inline uint32_t calcCSumFramerHdr(RB_FRAMER_HDR_S *pFramerHdr);

void * ringBufferCreateProducer(uint32_t wantedSize) {
	RB_SHM_CONTEXT_S *producer = (RB_SHM_CONTEXT_S*)malloc(sizeof(RB_SHM_CONTEXT_S));
	if (!producer) {
		return NULL;
	}

	producer->pvShmAddr = malloc(wantedSize);
	if (!producer->pvShmAddr) {
		free(producer);
		return NULL;
	}
	producer->u32ShmSize = wantedSize;
	producer->role = VV_RB_MEDIA_PRODUCER;
	producer->pBuffersPreallocate = NULL;
	producer->buffersPreallocateSize = 0;
	producer->pu8HeadAddr = (uint8_t*)producer->pvShmAddr + sizeof(RB_SHM_HDR_S);
	producer->pu8TailAddr = (uint8_t*)producer->pvShmAddr + producer->u32ShmSize;
	producer->pu8CurrAddr = producer->pu8HeadAddr;
	producer->u32MemSize = producer->pu8TailAddr - producer->pu8HeadAddr;

	/* Initialise header */
	RB_SHM_HDR_S *fHdr = (RB_SHM_HDR_S*)producer->pvShmAddr;
	fHdr->u32CommitId = 0;
	fHdr->u32TracksId = 0;
	fHdr->u32ShmSize = producer->u32ShmSize;

	return (void*)producer;
}

VV_RB_MEDIA_HANDLE_T ringBufferCreateConsumer(VV_RB_MEDIA_HANDLE_T pHdl, uint32_t preallocateSize) {
	if (!pHdl) {
		return NULL;
	}
	RB_SHM_CONTEXT_S *consumer = (RB_SHM_CONTEXT_S*)malloc(sizeof(RB_SHM_CONTEXT_S));
	if (!consumer) {
		return NULL;
	}
	RB_SHM_CONTEXT_S *producer = (RB_SHM_CONTEXT_S*)pHdl;

	RB_SHM_HDR_S *fHdr = (RB_SHM_HDR_S*)producer->pvShmAddr;
	consumer->pvShmAddr = producer->pvShmAddr;
	consumer->u32ShmSize = fHdr->u32ShmSize;
	consumer->role = VV_RB_MEDIA_CONSUMER;
	consumer->pu8HeadAddr = (uint8_t*)consumer->pvShmAddr + sizeof(RB_SHM_HDR_S);
	consumer->pu8TailAddr = (uint8_t*)consumer->pvShmAddr + consumer->u32ShmSize;
	consumer->pu8CurrAddr = consumer->pu8HeadAddr + fHdr->u32CommitId; /* Point to newest position has written */

	/* Preallocate buffers, avoid calling malloc()/free() many times */
	if (preallocateSize > 0) {
		consumer->buffersPreallocateSize = preallocateSize;
		consumer->pBuffersPreallocate = (uint8_t*)malloc(preallocateSize * sizeof(uint8_t));
		if (!consumer->pBuffersPreallocate) {
			free(consumer);
			return NULL;
		}
	}
	return (void*)consumer;
}

void ringBufferDelete(VV_RB_MEDIA_HANDLE_T pHdl) {
	if (!pHdl) {
		return;
	}
	RB_SHM_CONTEXT_S *pShm = (RB_SHM_CONTEXT_S*)pHdl;

	if (pShm->pBuffersPreallocate) {
		free(pShm->pBuffersPreallocate);
	}
	if (pShm->role == VV_RB_MEDIA_PRODUCER) {
		free(pShm->pvShmAddr);
	}
	free(pHdl);
}

int ringBufferSendTo(VV_RB_MEDIA_HANDLE_T pHdl, VV_RB_MEDIA_RESOURCE_S *pFramer) {
	if (!pHdl || !pFramer || !pFramer->pData) {
		return -1;
	}

	uint32_t remain;
	uint32_t totalBytes;
	RB_SHM_CONTEXT_S *pShm = (RB_SHM_CONTEXT_S*)pHdl;
	RB_SHM_HDR_S *pShmHdr = (RB_SHM_HDR_S*)pShm->pvShmAddr;

	if (pShm->role != VV_RB_MEDIA_PRODUCER) {
		return -1;
	}
	if (pFramer->dataLen > (pShm->u32MemSize + sizeof(RB_SHM_CONTEXT_S))) {
		/* Data exceed ring buffer size */
		return -2;
	}
	remain = pShm->pu8TailAddr - pShm->pu8CurrAddr;
	/* Point to begin position if size remain is not enough */
	if (remain < sizeof(RB_FRAMER_HDR_S)) {
		pShm->pu8CurrAddr = pShm->pu8HeadAddr;
	}

	RB_FRAMER_HDR_S *fHdr = (RB_FRAMER_HDR_S*)pShm->pu8CurrAddr;
	fHdr->id = pFramer->id;
	fHdr->type = pFramer->type;
	fHdr->u32Length = pFramer->dataLen;
	fHdr->encoder = pFramer->encoder;
	fHdr->u64Timestamp = pFramer->timestamp;
	fHdr->u32Magicnum = FRAMER_HDR_MAGICNUM;
	fHdr->u32FramePerSeconds = pFramer->framePerSeconds;

	/* Seek the current position & calculate size remain */
	pShm->pu8CurrAddr += sizeof(RB_FRAMER_HDR_S);
	remain = pShm->pu8TailAddr - pShm->pu8CurrAddr;

	/* Direct write data into share memory */
	totalBytes = pFramer->dataLen;
	if (totalBytes > remain) {
		memcpy(pShm->pu8CurrAddr, pFramer->pData, remain);
		pShm->pu8CurrAddr = pShm->pu8HeadAddr;
		totalBytes -= remain;
		memcpy(pShm->pu8CurrAddr, (uint8_t*)pFramer->pData + remain, totalBytes);
		pShm->pu8CurrAddr += totalBytes;
	}
	else {
		memcpy(pShm->pu8CurrAddr, pFramer->pData, totalBytes);
		pShm->pu8CurrAddr += totalBytes;
	}

	/* Update current position */
	remain = pShm->pu8TailAddr - pShm->pu8CurrAddr;
	if (remain < sizeof(RB_FRAMER_HDR_S)) {
		pShm->pu8CurrAddr = pShm->pu8HeadAddr;
	}
	else {
		/* 	
			IMPORTANT:
			We MUST-BE aligned 4 bytes before writing into memory because 
			some old architectures does not support unaligned access.
		*/
		int align = pShm->pu8CurrAddr - pShm->pu8HeadAddr;
		int divider = align % NUMBYTES_ALIGNMENT;
		if (divider != 0) {
			int padding = NUMBYTES_ALIGNMENT - divider;
			pShm->pu8CurrAddr = pShm->pu8HeadAddr + align + padding;
		}
	}

	/* Update framer header */	
	fHdr->u32NextId = ((uint8_t*)pShm->pu8CurrAddr - pShm->pu8HeadAddr);
	fHdr->u32PrevId = pShmHdr->u32TracksId;
	fHdr->u32Checksum = calcCSumFramerHdr(fHdr);

	/* Update ring buffer header commit index for reading */
	uint32_t u32CommitId = (uint8_t*)fHdr - pShm->pu8HeadAddr;
	pShmHdr->u32TracksId = u32CommitId;
	assignHdrId(pShm->pvShmAddr, u32CommitId);

	/* Setup next framer header */
	RB_FRAMER_HDR_S *pNextHdr = (RB_FRAMER_HDR_S*)pShm->pu8CurrAddr;
	memset(pNextHdr, 0, sizeof(RB_FRAMER_HDR_S));
	pNextHdr->u32Magicnum = FRAMER_HDR_MAGICNUM;
	pNextHdr->u32Checksum = calcCSumFramerHdr(pNextHdr);

	return 0;
}

int ringBufferReadFrom(VV_RB_MEDIA_HANDLE_T pHdl, VV_RB_MEDIA_RESOURCE_S *pFramer) {
	if (!pHdl || !pFramer) {
		return -1;
	}

	RB_SHM_CONTEXT_S *pShm = (RB_SHM_CONTEXT_S*)pHdl;
	uint8_t *pu8CurrAddr = pShm->pu8CurrAddr;
	RB_FRAMER_HDR_S *fHdr = (RB_FRAMER_HDR_S*)pu8CurrAddr;

	if (fHdr->u32Magicnum != FRAMER_HDR_MAGICNUM || fHdr->u32Checksum != calcCSumFramerHdr(fHdr) ||  fHdr->u32Length > pShm->u32MemSize - sizeof(RB_FRAMER_HDR_S)) 
	{
		uint32_t newId = readHdrId(pShm->pvShmAddr);
		pShm->pu8CurrAddr = pShm->pu8HeadAddr + newId;
		return -2;
	}

	/* Newest frames doesn't found */
	if (fHdr->u32Length == 0) {
		return 1;
	}

	/* We can reallocate buffer if suddenly frame bigger than preallocate buffers */
	if (fHdr->u32Length > pShm->buffersPreallocateSize) {
		printf("Reallocate SHM %d memory size from %d to %d\r\n", pShm->usShmId, pShm->buffersPreallocateSize, fHdr->u32Length);
		pShm->pBuffersPreallocate = realloc(pShm->pBuffersPreallocate, fHdr->u32Length);
		if (!pShm->pBuffersPreallocate) {
			return -3;
		}
		pShm->buffersPreallocateSize = fHdr->u32Length;
	}

	pFramer->id = fHdr->id;
	pFramer->type = fHdr->type;
	pFramer->pData = pShm->pBuffersPreallocate;
	pFramer->dataLen = fHdr->u32Length;
	pFramer->encoder = fHdr->encoder;
	pFramer->timestamp = fHdr->u64Timestamp;
	pFramer->framePerSeconds = fHdr->u32FramePerSeconds;

	/* Seek pointer to bytes data */
	pu8CurrAddr += sizeof(RB_FRAMER_HDR_S);
	uint32_t readBytes = fHdr->u32Length;
	uint32_t remain = pShm->pu8TailAddr - pu8CurrAddr;
	if (readBytes > remain) {
		memcpy(pFramer->pData, pu8CurrAddr, remain);
		pu8CurrAddr = pShm->pu8HeadAddr;
		readBytes -= remain;
		memcpy((uint8_t*)pFramer->pData + remain, pu8CurrAddr, readBytes);
		pu8CurrAddr += readBytes;
	}
	else {
		memcpy(pFramer->pData, pu8CurrAddr, readBytes);
		pu8CurrAddr += readBytes;
	}

	pShm->pu8CurrAddr = pShm->pu8HeadAddr + fHdr->u32NextId;

	return 0;
}

int ringBufferSeekTo(VV_RB_MEDIA_HANDLE_T pHdl, uint64_t u64Timestamp) {
	if (!pHdl) {
		return -1;
	}

	uint8_t *addr = NULL;
	bool bHasFound = false;
	uint32_t u32TracksId = 0;
	RB_FRAMER_HDR_S *fHdr = NULL;
	RB_SHM_CONTEXT_S *pShm = NULL;

	pShm = (RB_SHM_CONTEXT_S*)pHdl;
	u32TracksId = readHdrId(pShm->pvShmAddr);
	addr = pShm->pu8HeadAddr + u32TracksId;
	fHdr = (RB_FRAMER_HDR_S*)addr;

	if (fHdr->u32Magicnum != FRAMER_HDR_MAGICNUM || fHdr->u32Checksum != calcCSumFramerHdr(fHdr))  {
		return -2;
	}

	while (1) {
		uint64_t ts = fHdr->u64Timestamp;

		if (fHdr->encoder == VV_RB_MEDIA_ENCODER_FRAME_H264 || fHdr->encoder == VV_RB_MEDIA_ENCODER_FRAME_H265) {
			if (fHdr->type == VV_RB_MEDIA_FRAME_HDR_TYPE_I && ts <= u64Timestamp) {
				bHasFound = true;
				break;
			}
		}
		else { 
			if (ts <= u64Timestamp) {
				bHasFound = true;
				break;
			}
		}

		u32TracksId = fHdr->u32PrevId;
		addr = pShm->pu8HeadAddr + u32TracksId;
		RB_FRAMER_HDR_S *fHdrPrevious = (RB_FRAMER_HDR_S *)addr;
		if (fHdrPrevious->u32Magicnum != FRAMER_HDR_MAGICNUM || fHdrPrevious->u32Checksum != calcCSumFramerHdr(fHdrPrevious))  {
			break;
		}
		fHdr = fHdrPrevious;
	}

	if (bHasFound) {
		pShm->pu8CurrAddr = (uint8_t*)fHdr;
		return 0;
	}
	return -3;
}

void closeShm(int usShmId, void *pShm, bool erase) {
	if (pShm) {
		shmdt(pShm);
	}
	if (erase && usShmId >= 0) {
		shmctl(usShmId, IPC_RMID, NULL);
	}
}

inline void assignHdrId(void *pvShmAddr, uint32_t u32CommitId) {
	RB_SHM_HDR_S *pShmHdr = (RB_SHM_HDR_S*)pvShmAddr;
	pShmHdr->u32CommitId = u32CommitId;
}

inline uint32_t calcCSumFramerHdr(RB_FRAMER_HDR_S *pFramerHdr) {
	assert(pFramerHdr);
	
	uint32_t xor = 0;
    xor ^= pFramerHdr->id;
	xor ^= pFramerHdr->u32NextId;
	xor ^= pFramerHdr->u32PrevId;
    xor ^= pFramerHdr->u32Length;
	xor ^= pFramerHdr->u32Magicnum;
    /* Split 64-bit u64Timestamp into two 32-bit parts */
    xor ^= (uint32_t)(pFramerHdr->u64Timestamp & 0xFFFFFFFF);
    xor ^= (uint32_t)((pFramerHdr->u64Timestamp >> 32) & 0xFFFFFFFF);
	
    return xor;
}

inline uint32_t readHdrId(void *pvShmAddr) {
	RB_SHM_HDR_S *pShmHdr = (RB_SHM_HDR_S*)pvShmAddr;
	return pShmHdr->u32CommitId;
}