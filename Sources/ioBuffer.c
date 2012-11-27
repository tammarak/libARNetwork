/**
 *	@file ioBuffer.c
 *  @brief single buffer
 *  @date 28/09/2012
 *  @author maxime.maitre@parrot.com
**/

/*****************************************
 * 
 * 			include file :
 *
******************************************/

#include <stdlib.h>
#include <libSAL/print.h>
#include <libSAL/mutex.h>

#include <libNetwork/error.h>
#include <libNetwork/ringBuffer.h>
#include <libNetwork/frame.h>
#include <libNetwork/ioBuffer.h>

/*****************************************
 * 
 * 			define :
 *
******************************************/

#define NETWORK_IOBUFFER_ID_DEFAULT -1
#define NETWORK_IOBUFFER_DATATYPE_DEFAULT network_frame_t_TYPE_UNINITIALIZED
#define NETWORK_IOBUFFER_SENDINGWAITTIME_DEFAULT 1
#define NETWORK_IOBUFFER_ACKTILEOUTMS_DEFAULT -1
#define NETWORK_IOBUFFER_NBOFRETRY_DEFAULT -1
#define NETWORK_IOBUFFER_BUFFSIZE_DEFAULT 0
#define NETWORK_IOBUFFER_BUFFCELLSIZE_DEFAULT 0
#define NETWORK_IOBUFFER_OVERWRITING_DEFAULT 0

/*****************************************
 * 
 * 			implementation :
 *
******************************************/

void NETWORK_ParamNewIoBufferDefaultInit(network_paramNewIoBuffer_t *pParam)
{
	/** -- initialization of the paramNewIoBuffer with default parameters -- */
	
	pParam->id = NETWORK_IOBUFFER_ID_DEFAULT;
    pParam->dataType = NETWORK_IOBUFFER_DATATYPE_DEFAULT;	
    pParam->sendingWaitTime = NETWORK_IOBUFFER_SENDINGWAITTIME_DEFAULT;
    pParam->ackTimeoutMs = NETWORK_IOBUFFER_ACKTILEOUTMS_DEFAULT;
    pParam->nbOfRetry = NETWORK_IOBUFFER_NBOFRETRY_DEFAULT;
    
    pParam->buffSize = NETWORK_IOBUFFER_BUFFSIZE_DEFAULT;	
    pParam->buffCellSize = NETWORK_IOBUFFER_BUFFCELLSIZE_DEFAULT;
    pParam->overwriting = NETWORK_IOBUFFER_OVERWRITING_DEFAULT;
}

int NETWORK_ParamNewIoBufferCheck( const network_paramNewIoBuffer_t* pParam )
{
    /** -- check the values of the paramNewIoBuffer -- */
    
    /** local declarations */
    int ok = 0;
    
    if( pParam->id > NETWORK_IOBUFFER_ID_DEFAULT && 
        pParam->dataType != network_frame_t_TYPE_UNINITIALIZED &&
        pParam->sendingWaitTime > 0 &&
        pParam->ackTimeoutMs >= -1 &&
        pParam->nbOfRetry >= -1 &&
        pParam->buffSize > 0 &&
        pParam->buffCellSize > 0)
    {
        ok = 1;
    }
    else
    {
        sal_print(PRINT_ERROR," parameters for new IoBuffer are not correct. \n \
values expected: \n \
    - id > -1 \n \
    - dataType != network_frame_t_TYPE_UNINITIALIZED \n \
    - sendingWaitTime > 0 \n \
    - ackTimeoutMs > 0 or -1 if not used \n \
    - nbOfRetry > 0 or -1 if not used  \n \
    - buffSize > 0 \n \
    - buffCellSize > 0 \n \
    - overwriting = 0 or 1 ");
    }
    
   return ok; 
}

network_ioBuffer_t* NETWORK_NewIoBuffer( const network_paramNewIoBuffer_t* pParam )
{
	/** -- Create a new input or output buffer -- */
	
	/** local declarations */
	network_ioBuffer_t* pInOutBuff = NULL;
	int keepAliveData = 0x00;
    int error = NETWORK_OK;
	
	/** Create the input or output buffer in accordance with parameters set in pParam */
	pInOutBuff = malloc( sizeof(network_ioBuffer_t) );
	
	if( pInOutBuff )
	{ 
        /** Initialize to default values */
        pInOutBuff->pBuffer = NULL;
        sal_mutex_init( &(pInOutBuff->mutex) );
        
        if( NETWORK_ParamNewIoBufferCheck( pParam ) )
        {
            pInOutBuff->id = pParam->id;
            pInOutBuff->dataType = pParam->dataType;
            pInOutBuff->sendingWaitTime = pParam->sendingWaitTime;
            pInOutBuff->ackTimeoutMs = pParam->ackTimeoutMs;
        
            if(pParam->nbOfRetry > 0)
            {
                pInOutBuff->nbOfRetry = pParam->nbOfRetry;
            }
            else
            {
                /** if nbOfRetry equal 0 disable the retry function with -1 value */
                pInOutBuff->nbOfRetry = -1;
            }
            //	timeoutCallback(network_ioBuffer_t* this)
		
            pInOutBuff->isWaitAck = 0;
            pInOutBuff->seqWaitAck = 0;
            pInOutBuff->waitTimeCount = pParam->sendingWaitTime;
            pInOutBuff->ackWaitTimeCount = pParam->ackTimeoutMs;
            pInOutBuff->retryCount = 0;
		
            /** Create the RingBuffer */
            pInOutBuff->pBuffer = newRingBufferWithOverwriting(	pParam->buffSize, pParam->buffCellSize,
                                                                pParam->overwriting);
            if(pInOutBuff->pBuffer != NULL)
            {
                /** if it is a keep alive buffer, push in the data send for keep alive */ 
                if( pInOutBuff->dataType == network_frame_t_TYPE_KEEP_ALIVE )
                {
                    ringBuffPushBack(pInOutBuff->pBuffer, &keepAliveData);
                }
            }
            else
            {
                error = NETWORK_ERROR_NEW_RINGBUFFER;
            }
        }
        else
        {
            error = NETWORK_ERROR_BAD_PARAMETER;
        }
        
        if( error )
		{
			/** delete the inOutput buffer if an error occurred */
            sal_print(PRINT_ERROR,"error: %d occurred \n", error );
			NETWORK_DeleteIotBuffer(&pInOutBuff);
		}
    }
    
    return pInOutBuff;
}

void NETWORK_DeleteIotBuffer( network_ioBuffer_t** ppInOutBuff )
{	
	/** -- Delete the input or output buffer -- */
	
	/** local declarations */
	network_ioBuffer_t* pInOutBuff = NULL;
	
	if(ppInOutBuff)
	{
		pInOutBuff = *ppInOutBuff;
		
		if(pInOutBuff)
		{	
			sal_mutex_destroy( &(pInOutBuff->mutex) );
			
			deleteRingBuffer( &(pInOutBuff->pBuffer) );
		
			free(pInOutBuff);
            pInOutBuff = NULL;
		}
		*ppInOutBuff = NULL;
	}	

}

int NETWORK_IoBufferAckReceived( network_ioBuffer_t* pInOutBuff, int seqNum )
{
	/** -- Receive an acknowledgement to a inOutBuffer -- */ 
	
	/** local declarations */
	int error = NETWORK_IOBUFFER_ERROR_BAD_ACK;
	
	sal_mutex_lock( &(pInOutBuff->mutex) );
	
	/** delete the data if the sequence number received is same as the sequence number expected */
	if( pInOutBuff->isWaitAck && pInOutBuff->seqWaitAck == seqNum )
	{
		pInOutBuff->isWaitAck = 0;
		ringBuffPopFront( pInOutBuff->pBuffer, NULL );
		error = NETWORK_OK;
	}
	
	sal_mutex_unlock(&(pInOutBuff->mutex));
	
	return error;
}

network_ioBuffer_t* NETWORK_IoBufferWithId( network_ioBuffer_t** pptabInOutBuff,
												int tabSize, int id )
{
	/** -- Search a inOutBuffer with its identifier, in a table -- */
	
	/** local declarations */
	network_ioBuffer_t** it = pptabInOutBuff ;
	network_ioBuffer_t** itEnd = pptabInOutBuff + (tabSize);
	network_ioBuffer_t* pInOutBuffSearched = NULL;
	int find = 0;
	
	/** for each inoutBuffer of the table check if the ID is the same as the ID searched */
	for(it = pptabInOutBuff ; ( it != itEnd ) && !find ; ++it )
	{
		if( (*it)->id == id)
		{
			pInOutBuffSearched = *it;
			find = 1;
		}
	}
	
	return pInOutBuffSearched;
}

int NETWORK_IoBuffeIsWaitAck(	network_ioBuffer_t* pInOutBuff)
{
	/** -- Get if the inOutBuffer is waiting an acknowledgement -- */
	
	/** local declarations */
	int isWaitAckCpy = 0;
	
	sal_mutex_lock(&(pInOutBuff->mutex));
	
	isWaitAckCpy = pInOutBuff->isWaitAck;
	
	sal_mutex_unlock(&(pInOutBuff->mutex));
	
	return isWaitAckCpy;
}