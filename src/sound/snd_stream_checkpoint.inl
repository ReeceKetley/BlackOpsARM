#if defined(__ANDROID__)
bool Snd_StreamCheckpointPlan(const snd_asset &h, bool looping, unsigned long long cursor, unsigned int *byteOffset, unsigned int *skipFrames)
{
    if (!byteOffset || !skipFrames || h.version != 1 || h.header_size != 2096 || !h.frame_count
        || h.frame_rate < 1000 || h.frame_rate > 100000 || (h.channel_count != 1 && h.channel_count != 2)
        || looping != ((h.flags & 1) != 0) || h.seek_table_count || !h.data_size || h.data_size > 0xFFFFFFFFu - 2096) return false;
    unsigned int unit, framesPerUnit;
    if (h.format == SND_ASSET_FORMAT_PCMS16) { unit = h.channel_count * 2; framesPerUnit = 1; }
    else if (h.format == SND_ASSET_FORMAT_MSADPCM) { unit = h.channel_count * 262; framesPerUnit = 512; }
    else return false;
    if (h.data_size % unit || static_cast<unsigned long long>(h.data_size / unit) * framesPerUnit != h.frame_count) return false;
    const auto offset = looping ? cursor % h.frame_count : cursor;
    if (offset >= h.frame_count) return false;
    *byteOffset = static_cast<unsigned int>(offset / framesPerUnit * unit);
    *skipFrames = static_cast<unsigned int>(offset % framesPerUnit);
    return true;
}

bool Snd_StreamCheckpointHeader(unsigned int index, snd_asset *header)
{
    if (index >= 10 || !g_snd_streams || !header) return false;
    auto &s = g_snd_streams[index];
    s.mutex.Lock();
    const bool ready = s.in_use && s.have_header && !s.error && !s.header.seek_table_count;
    if (ready) { *header = s.header; header->seek_table = nullptr; header->data = nullptr; }
    s.mutex.Unlock();
    return ready;
}

bool Snd_StreamCheckpointReadHeader(const char *filename, bool looping, snd_asset *header)
{
    if (!filename || !*filename || !header) return false;
    snd_stream_file file{};
    if (!Snd_FileOpen(filename, &file)) return false;
    char bytes[2096];
    const unsigned int fileSize = file.size;
    const bool read = fileSize >= sizeof(bytes) && Snd_FileRead(&file, 0, sizeof(bytes), reinterpret_cast<unsigned char *>(bytes));
    Snd_FileClose(&file);
    if (!read) return false;
    // Check the canonical decoder's asserted header-size contract before it
    // reads the fixed disk header. No disk pointer token is ever published.
    unsigned int headerSize, dataSize;
    memcpy(&headerSize, bytes + 16, sizeof(headerSize));
    memcpy(&dataSize, bytes + 48, sizeof(dataSize));
    if (headerSize != sizeof(bytes) || dataSize > 0xFFFFFFFFu - sizeof(bytes)) return false;
    snd_stream scratch{}; scratch.looping = looping;
    Snd_StreamLoadHeader(&scratch, bytes, filename, fileSize);
    unsigned int offset, skip;
    if (scratch.error || !scratch.have_header || !Snd_StreamCheckpointPlan(scratch.header, looping, 0, &offset, &skip)) return false;
    *header = scratch.header; header->seek_table = nullptr; header->data = nullptr;
    return true;
}

bool Snd_StreamCheckpointSeek(unsigned int index, unsigned int byteOffset)
{
    if (index >= 10 || !g_snd_streams) return false;
    auto &s = g_snd_streams[index];
    s.mutex.Lock();
    Snd_StreamReleaseProcess(index);
    unsigned int unusedOffset, unusedSkip;
    bool allowed = s.in_use && s.have_header && !s.error && !s.windows_in_use
        && !s.buffer_references[0] && !s.buffer_references[1]
        && Snd_StreamCheckpointPlan(s.header, s.looping, 0, &unusedOffset, &unusedSkip);
    if (allowed)
    {
        const unsigned int unit = (s.header.format == SND_ASSET_FORMAT_MSADPCM ? 262 : 2) * s.header.channel_count;
        allowed = byteOffset < s.header.data_size && byteOffset % unit == 0;
    }
    for (unsigned int i = 0; i < 3; ++i) allowed = allowed && !s.window_return[i];
    if (allowed)
    {
        // In-flight read requests are matched against the new read offset by
        // Snd_StreamSetRequest. Reusing immutable bytes for that same file and
        // offset is valid; old offsets cannot publish into this seek window.
        for (unsigned int i = 0; i < 2; ++i)
        {
            if (s.buffers[i]) { _InterlockedExchangeAdd(&s.buffers[i]->reference_count, 0xFFFFFFFF); s.buffers[i] = nullptr; }
        }
        s.header.seek_table = nullptr;
        s.head = 2096 + byteOffset;
        s.read = s.head / 536576 * 536576;
        s.head_at_eof = false;
    }
    s.mutex.Unlock();
    if (allowed) Sys_WakeStream();
    return allowed;
}
#endif
