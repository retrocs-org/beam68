#ifndef BEAM68_CLIPBOARD_HISTORY_H
#define BEAM68_CLIPBOARD_HISTORY_H
#define BEAM_CLIP_HISTORY_MAX 16
#define BEAM_CLIP_HISTORY_TEXT 120
struct BeamClipEntry { unsigned long id,stamp;char text[BEAM_CLIP_HISTORY_TEXT+1]; };
struct BeamClipHistory {struct BeamClipEntry entries[BEAM_CLIP_HISTORY_MAX];unsigned int count,limit,budget;unsigned long expiry,next_id;};
void beam_clip_history_init(struct BeamClipHistory *h,unsigned int limit,unsigned int budget,unsigned long expiry);
int beam_clip_history_expire(struct BeamClipHistory *h,unsigned long now);
int beam_clip_history_add(struct BeamClipHistory *h,const char *text,unsigned long now);
const char *beam_clip_history_find(struct BeamClipHistory *h,unsigned long id,unsigned long now);
int beam_clip_history_match(const char *text,const char *filter);
int beam_clip_history_save(const struct BeamClipHistory *h,const char *path);
int beam_clip_history_load(struct BeamClipHistory *h,const char *path,unsigned long now);
#endif
