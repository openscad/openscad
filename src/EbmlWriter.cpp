// Copyright (c) 2010 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include <limits.h>

#include "EbmlWriter.h"

#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64 */
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

#if defined(_MSC_VER)
#define LITERALU64(n) n
#else
#define LITERALU64(n) n##LLU
#endif

void Ebml_Write(EbmlGlobal *glob, const void *buffer_in, unsigned long len)
{
	fwrite(buffer_in, 1, len, glob->stream);
}

void Ebml_Serialize(EbmlGlobal *glob, const void *buffer_in, unsigned long len)
{
    const unsigned char *q = (const unsigned char *)buffer_in + len - 1;

    for(; len; len--)
        Ebml_Write(glob, q--, 1);
}

/* Need a fixed size serializer for the track ID. libmkv provdes a 64 bit
 * one, but not a 32 bit one.
 */
void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
    unsigned char sizeSerialized = 4 | 0x80;
    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &sizeSerialized, 1);
    Ebml_Serialize(glob, &ui, 4);
}

void
Ebml_StartSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc,
                          unsigned long class_id)
{
    //todo this is always taking 8 bytes, this may need later optimization
    //this is a key that says lenght unknown
    unsigned long long unknownLen =  LITERALU64(0x01FFFFFFFFFFFFFF);

    Ebml_WriteID(glob, class_id);
    *ebmlLoc = ftello(glob->stream);
    Ebml_Serialize(glob, &unknownLen, 8);
}

void
Ebml_EndSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc)
{
    off_t pos;
    uint64_t size;

    /* Save the current stream pointer */
    pos = ftello(glob->stream);

    /* Calculate the size of this element */
    size = pos - *ebmlLoc - 8;
    size |=  LITERALU64(0x0100000000000000);

    /* Seek back to the beginning of the element and write the new size */
    fseeko(glob->stream, *ebmlLoc, SEEK_SET);
    Ebml_Serialize(glob, &size, 8);

    /* Reset the stream pointer */
    fseeko(glob->stream, pos, SEEK_SET);
}

void Ebml_WriteLen(EbmlGlobal *glob, long long val)
{
    //TODO check and make sure we are not > than 0x0100000000000000LLU
    unsigned char size = 8; //size in bytes to output
    unsigned long long minVal = LITERALU64(0x00000000000000ff); //mask to compare for byte size

    for (size = 1; size < 8; size ++)
    {
        if (val < minVal)
            break;

        minVal = (minVal << 7);
    }

    val |= (LITERALU64(0x000000000000080) << ((size - 1) * 7));

    Ebml_Serialize(glob, (void *) &val, size);
}

void Ebml_WriteString(EbmlGlobal *glob, const char *str)
{
    const size_t size_ = strlen(str);
    const unsigned long long  size = size_;
    Ebml_WriteLen(glob, size);
    //TODO: it's not clear from the spec whether the nul terminator
    //should be serialized too.  For now we omit the null terminator.
    Ebml_Write(glob, str, size);
}

void Ebml_WriteUTF8(EbmlGlobal *glob, const wchar_t *wstr)
{
    const size_t strlen = wcslen(wstr);

    //TODO: it's not clear from the spec whether the nul terminator
    //should be serialized too.  For now we include it.
    const unsigned long long  size = strlen;

    Ebml_WriteLen(glob, size);
    Ebml_Write(glob, wstr, size);
}

void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id)
{
    if (class_id >= 0x01000000)
        Ebml_Serialize(glob, (void *)&class_id, 4);
    else if (class_id >= 0x00010000)
        Ebml_Serialize(glob, (void *)&class_id, 3);
    else if (class_id >= 0x00000100)
        Ebml_Serialize(glob, (void *)&class_id, 2);
    else
        Ebml_Serialize(glob, (void *)&class_id, 1);
}
void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
    unsigned char sizeSerialized = 8 | 0x80;
    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &sizeSerialized, 1);
    Ebml_Serialize(glob, &ui, 8);
}

void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui)
{
    unsigned char size = 8; //size in bytes to output
    unsigned char sizeSerialized = 0;
    unsigned long minVal;

    Ebml_WriteID(glob, class_id);
    minVal = 0x7fLU; //mask to compare for byte size

    for (size = 1; size < 4; size ++)
    {
        if (ui < minVal)
        {
            break;
        }

        minVal <<= 7;
    }

    sizeSerialized = 0x80 | size;
    Ebml_Serialize(glob, &sizeSerialized, 1);
    Ebml_Serialize(glob, &ui, size);
}
//TODO: perhaps this is a poor name for this id serializer helper function
void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long bin)
{
    int size;
    for (size=4; size > 1; size--)
    {
        if (bin & 0x000000ff << ((size-1) * 8))
            break;
    }
    Ebml_WriteID(glob, class_id);
    Ebml_WriteLen(glob, size);
    Ebml_WriteID(glob, bin);
}

void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d)
{
    unsigned char len = 0x88;

    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &len, 1);
    Ebml_Serialize(glob,  &d, 8);
}

void Ebml_WriteSigned16(EbmlGlobal *glob, short val)
{
    signed long out = ((val & 0x003FFFFF) | 0x00200000) << 8;
    Ebml_Serialize(glob, &out, 3);
}

void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s)
{
    Ebml_WriteID(glob, class_id);
    Ebml_WriteString(glob, s);
}

void Ebml_SerializeUTF8(EbmlGlobal *glob, unsigned long class_id, wchar_t *s)
{
    Ebml_WriteID(glob,  class_id);
    Ebml_WriteUTF8(glob,  s);
}

void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length)
{
    unsigned char size = 4;
    Ebml_WriteID(glob, class_id);
    Ebml_WriteLen(glob, data_length);
    Ebml_Write(glob,  data, data_length);
}

void Ebml_WriteVoid(EbmlGlobal *glob, unsigned long vSize)
{
    unsigned char tmp = 0;
    unsigned long i = 0;

    Ebml_WriteID(glob, 0xEC);
    Ebml_WriteLen(glob, vSize);

    for (i = 0; i < vSize; i++)
    {
        Ebml_Write(glob, &tmp, 1);
    }
}

void Ebml_WriteWebMSeekElement(EbmlGlobal *ebml, unsigned long id, off_t pos)
{
    uint64_t offset = pos - ebml->position_reference;
    EbmlLoc start;
    Ebml_StartSubElement(ebml, &start, Seek);
    Ebml_SerializeBinary(ebml, SeekID, id);
    Ebml_SerializeUnsigned64(ebml, SeekPosition, offset);
    Ebml_EndSubElement(ebml, &start);
}

void Ebml_WriteWebMSeekInfo(EbmlGlobal *ebml)
{

    off_t pos;

    /* Save the current stream pointer */
    pos = ftello(ebml->stream);

    if(ebml->seek_info_pos)
        fseeko(ebml->stream, ebml->seek_info_pos, SEEK_SET);
    else
        ebml->seek_info_pos = pos;

    {
        EbmlLoc start;

        Ebml_StartSubElement(ebml, &start, SeekHead);
        Ebml_WriteWebMSeekElement(ebml, Tracks, ebml->track_pos);
        Ebml_WriteWebMSeekElement(ebml, Cues,   ebml->cue_pos);
        Ebml_WriteWebMSeekElement(ebml, Info,   ebml->segment_info_pos);
        Ebml_EndSubElement(ebml, &start);
    }
    {
        //segment info
        EbmlLoc startInfo;
        uint64_t frame_time;

        frame_time = (uint64_t)1000 * ebml->framerate.den
                     / ebml->framerate.num;
        ebml->segment_info_pos = ftello(ebml->stream);
        Ebml_StartSubElement(ebml, &startInfo, Info);
        Ebml_SerializeUnsigned(ebml, TimecodeScale, 1000000);
        Ebml_SerializeFloat(ebml, Segment_Duration,
                            ebml->last_pts_ms + frame_time);
        Ebml_SerializeString(ebml, 0x4D80, "vpxenc");
        Ebml_SerializeString(ebml, 0x5741, "vpxenc");
        Ebml_EndSubElement(ebml, &startInfo);
    }
}


void Ebml_WriteWebMFileHeader(EbmlGlobal                *glob,
                       const vpx_codec_enc_cfg_t *cfg,
                       const struct vpx_rational *fps)
{
    {
        EbmlLoc start;
        Ebml_StartSubElement(glob, &start, EBML);
        Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
        Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1); //EBML Read Version
        Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4); //EBML Max ID Length
        Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8); //EBML Max Size Length
        Ebml_SerializeString(glob, DocType, "webm"); //Doc Type
        Ebml_SerializeUnsigned(glob, DocTypeVersion, 2); //Doc Type Version
        Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2); //Doc Type Read Version
        Ebml_EndSubElement(glob, &start);
    }
    {
        Ebml_StartSubElement(glob, &glob->startSegment, Segment); //segment
        glob->position_reference = ftello(glob->stream);
        glob->framerate = *fps;
        Ebml_WriteWebMSeekInfo(glob);

        {
            EbmlLoc trackStart;
            glob->track_pos = ftello(glob->stream);
            Ebml_StartSubElement(glob, &trackStart, Tracks);
            {
                unsigned int trackNumber = 1;
                uint64_t     trackID = 0;

                EbmlLoc start;
                Ebml_StartSubElement(glob, &start, TrackEntry);
                Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
                glob->track_id_pos = ftello(glob->stream);
                Ebml_SerializeUnsigned32(glob, TrackUID, trackID);
                Ebml_SerializeUnsigned(glob, TrackType, 1); //video is always 1
                Ebml_SerializeString(glob, CodecID, "V_VP8");
                {
                    unsigned int pixelWidth = cfg->g_w;
                    unsigned int pixelHeight = cfg->g_h;
                    float        frameRate   = (float)fps->num/(float)fps->den;

                    EbmlLoc videoStart;
                    Ebml_StartSubElement(glob, &videoStart, Video);
                    Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
                    Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
                    Ebml_SerializeFloat(glob, FrameRate, frameRate);
                    Ebml_EndSubElement(glob, &videoStart); //Video
                }
                Ebml_EndSubElement(glob, &start); //Track Entry
            }
            Ebml_EndSubElement(glob, &trackStart);
        }
        // segment element is open
    }
}


void Ebml_WriteWebMBlock(EbmlGlobal                *glob,
                 const vpx_codec_enc_cfg_t *cfg,
                 const vpx_codec_cx_pkt_t  *pkt)
{
    unsigned long  block_length;
    unsigned char  track_number;
    unsigned short block_timecode = 0;
    unsigned char  flags;
    int64_t        pts_ms;
    int            start_cluster = 0, is_keyframe;

    /* Calculate the PTS of this frame in milliseconds */
    pts_ms = pkt->data.frame.pts * 1000
             * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;
    if(pts_ms <= glob->last_pts_ms)
        pts_ms = glob->last_pts_ms + 1;
    glob->last_pts_ms = pts_ms;

    /* Calculate the relative time of this block */
    if(pts_ms - glob->cluster_timecode > SHRT_MAX)
        start_cluster = 1;
    else
        block_timecode = pts_ms - glob->cluster_timecode;

    is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);
    if(start_cluster || is_keyframe)
    {
        if(glob->cluster_open)
            Ebml_EndSubElement(glob, &glob->startCluster);

        /* Open the new cluster */
        block_timecode = 0;
        glob->cluster_open = 1;
        glob->cluster_timecode = pts_ms;
        glob->cluster_pos = ftello(glob->stream);
        Ebml_StartSubElement(glob, &glob->startCluster, Cluster); //cluster
        Ebml_SerializeUnsigned(glob, Timecode, glob->cluster_timecode);

        /* Save a cue point if this is a keyframe. */
        if(is_keyframe)
        {
            struct cue_entry *cue;

            glob->cue_list = (cue_entry*)realloc(glob->cue_list, (glob->cues+1) * sizeof(cue_entry));
            cue = &glob->cue_list[glob->cues];
            cue->time = glob->cluster_timecode;
            cue->loc = glob->cluster_pos;
            glob->cues++;
        }
    }

    /* Write the Simple Block */
    Ebml_WriteID(glob, SimpleBlock);

    block_length = pkt->data.frame.sz + 4;
    block_length |= 0x10000000;
    Ebml_Serialize(glob, &block_length, 4);

    track_number = 1;
    track_number |= 0x80;
    Ebml_Write(glob, &track_number, 1);

    Ebml_Serialize(glob, &block_timecode, 2);

    flags = 0;
    if(is_keyframe)
        flags |= 0x80;
    if(pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
        flags |= 0x08;
    Ebml_Write(glob, &flags, 1);

    Ebml_Write(glob, pkt->data.frame.buf, pkt->data.frame.sz);
}


void Ebml_WriteWebMFileFooter(EbmlGlobal *glob, long hash)
{

    if(glob->cluster_open)
        Ebml_EndSubElement(glob, &glob->startCluster);

    {
        EbmlLoc start;
        int i;

        glob->cue_pos = ftello(glob->stream);
        Ebml_StartSubElement(glob, &start, Cues);
        for(i=0; i<glob->cues; i++)
        {
            struct cue_entry *cue = &glob->cue_list[i];
            EbmlLoc start;

            Ebml_StartSubElement(glob, &start, CuePoint);
            {
                EbmlLoc start;

                Ebml_SerializeUnsigned(glob, CueTime, cue->time);

                Ebml_StartSubElement(glob, &start, CueTrackPositions);
                Ebml_SerializeUnsigned(glob, CueTrack, 1);
                Ebml_SerializeUnsigned64(glob, CueClusterPosition,
                                         cue->loc - glob->position_reference);
                //Ebml_SerializeUnsigned(glob, CueBlockNumber, cue->blockNumber);
                Ebml_EndSubElement(glob, &start);
            }
            Ebml_EndSubElement(glob, &start);
        }
        Ebml_EndSubElement(glob, &start);
    }

    Ebml_EndSubElement(glob, &glob->startSegment);

    /* Patch up the seek info block */
    Ebml_WriteWebMSeekInfo(glob);

    /* Patch up the track id */
    fseeko(glob->stream, glob->track_id_pos, SEEK_SET);
    Ebml_SerializeUnsigned32(glob, TrackUID, glob->debug ? 0xDEADBEEF : hash);

    fseeko(glob->stream, 0, SEEK_END);
}

