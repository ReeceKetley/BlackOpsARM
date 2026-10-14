import pathlib,shutil,difflib,subprocess,os,hashlib
repo=pathlib.Path(__file__).resolve().parents[2]; upstream=repo/'third_party/ffmpeg-7.1.2'; target=repo/'build-android-audit/media-source'
alias=pathlib.Path('D:/bo1-native-codec-audit')
if alias.exists():
 assert alias.resolve()==repo.resolve(), 'Codec alias is owned by another path'
else:
 subprocess.run(['cmd','/c','mklink','/J',str(alias),str(repo)],check=True)
if not target.exists():shutil.copytree(upstream,target)
source=(upstream/'libavcodec/wmadec.c').read_text()
old='''        frame->nb_samples = s->frame_len;
        if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
            return ret;

        frame->pts = AV_NOPTS_VALUE;
        for (i = 0; i < s->avctx->ch_layout.nb_channels; i++)
            memcpy(frame->extended_data[i], &s->frame_out[i][0],
                   frame->nb_samples * sizeof(s->frame_out[i][0]));
'''
new='''        /* Canonical xWMA ends without a following packet header. Decode
         * its buffered final compressed frame before retiring transform overlap.
         * Generic ASF flushing otherwise discards real resident-asset tail data.
         * No fabricated padding: malformed buffered data remains an error. */
        frame->nb_samples = s->frame_len * (s->last_superframe_len > 0 ? 2 : 1);
        if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
            return ret;

        frame->pts = AV_NOPTS_VALUE;
        samples_offset = 0;
        if (s->last_superframe_len > 0) {
            init_get_bits(&s->gb, s->last_superframe, s->last_superframe_len * 8);
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);
            if (wma_decode_frame(s, (float **)frame->extended_data, 0) < 0)
                return AVERROR_INVALIDDATA;
            samples_offset = s->frame_len;
        }
        for (i = 0; i < s->avctx->ch_layout.nb_channels; i++)
            memcpy((float *)frame->extended_data[i] + samples_offset, &s->frame_out[i][0],
                   s->frame_len * sizeof(s->frame_out[i][0]));
'''
assert source.count(old)==1
patched=source.replace(old,new)
(target/'libavcodec/wmadec.c').write_text(patched)
patch=repo/'tools/port/patches/ffmpeg-7.1.2-wmav2-eos.patch';patch.parent.mkdir(exist_ok=True)
actual_patch=''.join(difflib.unified_diff(source.splitlines(True),patched.splitlines(True),fromfile='a/libavcodec/wmadec.c',tofile='b/libavcodec/wmadec.c'))
assert patch.read_text()==actual_patch, 'Prepared code differs from the reviewed pinned patch'
print('Prepared explicit EOS patch; upstream remains unchanged')
