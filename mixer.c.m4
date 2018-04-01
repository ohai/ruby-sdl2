/* -*- mode: C -*- */
#ifdef HAVE_SDL_MIXER_H
#include "rubysdl2_internal.h"
#include <SDL_mixer.h>

static VALUE mMixer;
static VALUE cChunk;
static VALUE cMusic;
static VALUE mChannels;
static VALUE cGroup;
static VALUE mMusicChannel;

static VALUE playing_chunks = Qnil;
static VALUE playing_music = Qnil;

#define MIX_ERROR() do { HANDLE_ERROR(SDL_SetError("%s", Mix_GetError())); } while(0)
#define HANDLE_MIX_ERROR(code) \
    do { if ((code) < 0) { MIX_ERROR(); } } while (0)

typedef struct Chunk {
    Mix_Chunk* chunk;
} Chunk;

typedef struct Music {
    Mix_Music* music;
} Music;

static void Chunk_free(Chunk* c)
{
    if (rubysdl2_is_active() && c->chunk) 
        Mix_FreeChunk(c->chunk);
    free(c);
}

static VALUE Chunk_new(Mix_Chunk* chunk)
{
    Chunk* c = ALLOC(Chunk);
    c->chunk = chunk;
    return Data_Wrap_Struct(cChunk, 0, Chunk_free, c);
}

DEFINE_WRAPPER(Mix_Chunk, Chunk, chunk, cChunk, "SDL2::Mixer::Chunk");

static void Music_free(Music* m)
{
    if (rubysdl2_is_active() && m->music) 
        Mix_FreeMusic(m->music);
    free(m);
}

static VALUE Music_new(Mix_Music* music)
{
    Music* c = ALLOC(Music);
    c->music = music;
    return Data_Wrap_Struct(cMusic, 0, Music_free, c);
}
                            
DEFINE_WRAPPER(Mix_Music, Music, music, cMusic, "SDL2::Mixer::Music");

/*
 * Document-module: SDL2::Mixer
 *
 * Sound mixing module.
 *
 * With this module, you can play many kinds of sound files such as:
 *
 * * WAVE/RIFF (.wav)
 * * AIFF (.aiff)
 * * VOC (.voc)
 * * MOD (.mod .xm .s3m .669 .it .med etc.)
 * * MIDI (.mid)
 * * OggVorbis (.ogg)
 * * MP3 (.mp3)
 * * FLAC (.flac)
 *
 * Before playing sounds, 
 * you need to initialize this module by {.init} and
 * open a sound device by {.open}. 
 *
 * This module mixes multiple sound sources in parallel.
 * To play a sound source, you assign the source to a "channel"
 * and this module mixes all sound sources assigned to the channels.
 * 
 * In this module, there are two types of sound sources:
 * {SDL2::Mixer::Chunk} and {SDL2::Mixer::Music}.
 * And there are two corresponding types of channels:
 * {SDL2::Mixer::Channels} and {SDL2::Mixer::MusicChannel}.
 *
 * {SDL2::Mixer::Channels} module plays {SDL2::Mixer::Chunk} objects,
 * through multiple (default eight) channels. This module is suitable
 * for the sound effects.
 * The number of channels is variable with {SDL2::Mixer::Channels.allocate}.
 * 
 * {SDL2::Mixer::MusicChannel} module plays {SDL2::Mixer::Music} objects.
 * This module has only one playing channel, and you cannot play
 * multiple music in parallel. However an {SDL2::Mixer::Music} object
 * is more efficient for memory, and this module supports more file formats
 * than {SDL2::Mixer::Channels}.
 * This module is suitable for playing "BGMs" of your application.
 * 
 */

/*
 * @overload init(flags)
 *   Initialize the mixer library.
 *
 *   This module function load dynamically-linked libraries for sound file
 *   formats such as ogg and flac.
 *
 *   You can give the initialized libraries (file formats) with OR'd bits of the
 *   following constants:
 *
 *   * SDL2::Mixer::INIT_FLAC
 *   * SDL2::Mixer::INIT_MOD
 *   * SDL2::Mixer::INIT_MODPLUG
 *   * SDL2::Mixer::INIT_MP3
 *   * SDL2::Mixer::INIT_OGG
 *   * SDL2::Mixer::INIT_FLUIDSYNTH
 *   
 *   @param flags [Integer] intialized sublibraries
 *   @return [nil]
 * 
 */
static VALUE Mixer_s_init(VALUE self, VALUE f)
{
    int flags = NUM2INT(f);
    if ((Mix_Init(flags) & flags) != flags) 
        rb_raise(eSDL2Error, "Couldn't initialize SDL_mixer");
    
    return Qnil;
}

static void check_channel(VALUE ch, int allow_minus_1)
{
    int channel = NUM2INT(ch);
    if (channel >= Mix_AllocateChannels(-1))
        rb_raise(rb_eArgError, "too large number of channel (%d)", channel);
    if (channel == -1 && !allow_minus_1 || channel < -1)
        rb_raise(rb_eArgError, "negative number of channel is not allowed");
}

/*
 * @overload open(freq=22050, format=SDL2::Mixer::DEFAULT_FORMAT, channels=2, chunksize=1024)
 *   Open a sound device.
 *
 *   Before calling loading/playing methods in the mixer module,
 *   this method must be called.
 *   Before calling this method,
 *   {SDL2.init} must be called with SDL2::INIT_AUDIO.
 *
 *   @param freq [Integer] output sampling frequency in Hz.
 *     Normally 22050 or 44100 is used.
 *     44100 is CD audio rate. SDL2::Mixer::DEFAULT_FREQUENCY(22050) is best for
 *     many kinds of game because 44100 requires too much CPU power on older computers.
 *   @param format [Integer] output sample format
 *   @param channels 1 is for mono, and 2 is for stereo.
 *   @param chunksize bytes used per output sample
 *
 *   @return [nil]
 *
 *   @raise [SDL2::Error] raised when a device cannot be opened
 *   
 *   @see .init   
 *   @see .close
 *   @see .query
 */
static VALUE Mixer_s_open(int argc, VALUE* argv, VALUE self)
{
    VALUE freq, format, channels, chunksize;
    rb_scan_args(argc, argv, "04", &freq, &format, &channels, &chunksize);
    HANDLE_MIX_ERROR(Mix_OpenAudio((freq == Qnil) ? MIX_DEFAULT_FREQUENCY : NUM2INT(freq),
                                   (format == Qnil) ? MIX_DEFAULT_FORMAT : NUM2UINT(format),
                                   (channels == Qnil) ? 2 : NUM2INT(channels),
                                   (chunksize == Qnil) ? 1024 : NUM2INT(chunksize)));
    playing_chunks = rb_ary_new();
    return Qnil;
}

/*
 * Close the audio device.
 *
 * @return [nil]
 */
static VALUE Mixer_s_close(VALUE self)
{
    Mix_CloseAudio();
    return Qnil;
}


/*
 * Query a sound device spec.
 *
 * This method returns the most suitable setting for {.open} the device.
 *
 * @return [[Integer, Integer, Integer, Integer]]
 *   the suitable frequency in Hz, the suitable format,
 *   the suitable number of channels (1 for mono, 2 for stereo),
 *   and the number of call of {.open}.
 *
 */
static VALUE Mixer_s_query(VALUE self)
{
    int frequency = 0, channels = 0, num_opened;
    Uint16 format = 0;

    num_opened = Mix_QuerySpec(&frequency, &format, &channels);
    return rb_ary_new3(4, INT2NUM(frequency), UINT2NUM(format),
                       INT2NUM(channels), INT2NUM(num_opened));
}

/*
 * Document-module: SDL2::Mixer::Channels
 *
 * This module plays {SDL2::Mixer::Chunk} objects in parallel.
 *
 * Each virtual sound output device is called channel, and
 * the number of channels determines the f
 */

/*
 * @overload allocate(num_channels)
 *   Set the number of channels being mixed.
 *
 *   @param num_channels [Integer] Number of channels prepared for mixing.
 *
 *   @return [Integer] the number of prepared channels.
 */
static VALUE Channels_s_allocate(VALUE self, VALUE num_channels)
{
    return INT2NUM(Mix_AllocateChannels(NUM2INT(num_channels)));
}

/*
 * @overload reserve(num)
 *   Reserve channel from 0 to num-1 and reserved channels are not used by
 *   {Channels.play} and {Channels.fade_in} with **channels**==-1.
 *   
 *   @param num [Integer]
 *   @return [Integer]
 */
static VALUE Channels_s_reserve(VALUE self, VALUE num)
{
    return INT2NUM(Mix_ReserveChannels(NUM2INT(num)));
}

/*
 * @overload volume(channel)
 *   Get the volume of specified channel.
 *
 *   @param channel [Integer] the channel to get volume for.
 *     If the specified channel is -1, this method returns
 *     the average volume of all channels.
 *   @return [Integer] the volume, 0-128
 *
 *   @see .set_volume
 */ 
static VALUE Channels_s_volume(VALUE self, VALUE channel)
{
    return INT2NUM(Mix_Volume(NUM2INT(channel), -1));
}

/*
 * @overload set_volume(channel, volume)
 *   Set the volume of specified channel.
 *
 *   The volume should be from 0 to {SDL2::Mixer::MAX_VOLUME}(128).
 *   If the specified channel is -1, set volume for all channels.
 *
 *   @param channel [Integer] the channel to set volume for.
 *   @param volume [Integer] the volume to use
 *   @return [void]
 *
 *   @see .volume
 */
static VALUE Channels_s_set_volume(VALUE self, VALUE channel, VALUE volume)
{
    return INT2NUM(Mix_Volume(NUM2INT(channel), NUM2INT(volume)));
}

static void protect_playing_chunk_from_gc(int channel, VALUE chunk)
{
    rb_ary_store(playing_chunks, channel, chunk);
}

/*
 * @overload play(channel, chunk, loops, ticks = -1)
 *   Play a {SDL2::Mixer::Chunk} on **channel**.
 *   
 *   @param channel [Integer] the channel to play, or -1 for the first free unreserved
 *     channel
 *   @param chunk [SDL2::Mixer::Chunk] the chunk to play
 *   @param loops [Integer] the number of loops, or -1 for infite loops.
 *     passing 1 plays the sample twice (1 loop).
 *   @param ticks [Integer] milliseconds limit to play, at most.
 *     If the chunk is long enough and **loops** is large enough,
 *     the play will stop after **ticks** milliseconds.
 *     Otherwise, the play will stop when  the loop ends.
 *     -1 means infinity.
 *   @return [Integer] the channel that plays the chunk.
 *
 *   @raise [SDL2::Error] raised on a playing error. For example,
 *     **channel** is out of the allocated channels, or
 *     there is no free channels when **channel** is -1.
 *
 *   @see .fade_in
 */
static VALUE Channels_s_play(int argc, VALUE* argv, VALUE self)
{
    VALUE channel, chunk, loops, ticks;
    int ch;
    rb_scan_args(argc, argv, "31", &channel, &chunk, &loops, &ticks);
    if (ticks == Qnil)
        ticks = INT2FIX(-1);
    check_channel(channel, 1);
    ch = Mix_PlayChannelTimed(NUM2INT(channel), Get_Mix_Chunk(chunk),
                              NUM2INT(loops), NUM2INT(ticks));
    HANDLE_MIX_ERROR(ch);
    protect_playing_chunk_from_gc(ch, chunk);
    return INT2FIX(ch);
}

/*
 * @overload fade_in(channel, chunk, loops, ms, ticks = -1)
 *   Play a {SDL2::Mixer::Chunk} on **channel** with fading in.
 *   
 *   @param channel [Integer] the channel to play, or -1 for the first free unreserved
 *     channel
 *   @param chunk [SDL2::Mixer::Chunk] the chunk to play
 *   @param loops [Integer] the number of loops, or -1 for infite loops.
 *     passing 1 plays the sample twice (1 loop).
 *   @param ms [Integer] milliseconds of time of fade-in effect.
 *   @param ticks [Integer] milliseconds limit to play, at most.
 *     If the chunk is long enough and **loops** is large enough,
 *     the play will stop after **ticks** milliseconds.
 *     Otherwise, the play will stop when  the loop ends.
 *     -1 means infinity.
 *   @return [Integer] the channel that plays the chunk.
 *
 *   @raise [SDL2::Error] raised on a playing error. For example,
 *     **channel** is out of the allocated channels, or
 *     there is no free channels when **channel** is -1.
 *
 *   @see .play
 *   @see .fade_out
 */
static VALUE Channels_s_fade_in(int argc, VALUE* argv, VALUE self)
{
    VALUE channel, chunk, loops, ms, ticks;
    int ch;
    rb_scan_args(argc, argv, "41", &channel, &chunk, &loops, &ms, &ticks);
    if (ticks == Qnil)
        ticks = INT2FIX(-1);
    check_channel(channel, 1);
    ch = Mix_FadeInChannelTimed(NUM2INT(channel), Get_Mix_Chunk(chunk),
                                NUM2INT(loops), NUM2INT(ms), NUM2INT(ticks));
    HANDLE_MIX_ERROR(ch);
    protect_playing_chunk_from_gc(ch, chunk);
    return INT2FIX(ch);
}

/*
 * @overload pause(channel)
 *   Pause a specified channel.
 *
 *   @param channel [Integer] the channel to pause, or -1 for all channels.
 *   @return [nil]
 *   
 *   @see .resume
 *   @see .pause?
 */
static VALUE Channels_s_pause(VALUE self, VALUE channel)
{
    check_channel(channel, 1);
    Mix_Pause(NUM2INT(channel));
    return Qnil;
}

/*
 * @overload resume(channel)
 *   Resume a specified channel that already pauses.
 *
 *   @note This method has no effect to unpaused channels.
 *   @param channel [Integer] the channel to be resumed, or -1 for all channels.
 *   @return [nil]
 *
 *   @see .pause
 *   @see .pause?
 */
static VALUE Channels_s_resume(VALUE self, VALUE channel)
{
    check_channel(channel, 1);
    Mix_Resume(NUM2INT(channel));
    return Qnil;
}

/*
 * @overload halt(channel)
 *   Halt playing of a specified channel.
 *   
 *   @param channel [Integer] the channel to be halted, or -1 for all channels.
 *   @return [nil]
 *
 *   @see .expire
 *   @see .fade_out
 *   @see .play?
 */
static VALUE Channels_s_halt(VALUE self, VALUE channel)
{
    check_channel(channel, 1);
    Mix_HaltChannel(NUM2INT(channel));
    return Qnil;
}

/*
 * @overload expire(channel, ticks)
 *   Halt playing of a specified channel after **ticks** milliseconds.
 *
 *   @param channel [Integer] the channel to be halted, or -1 for all channels.
 *   @param ticks [Integer] milliseconds untils the channel halts playback.
 *   @return [nil]
 *
 *   @see .halt
 *   @see .fade_out
 *   @see .play?
 */
static VALUE Channels_s_expire(VALUE self, VALUE channel, VALUE ticks)
{
    check_channel(channel, 1);
    Mix_ExpireChannel(NUM2INT(channel), NUM2INT(ticks));
    return Qnil;
}

/*
 * @overload fade_out(channel, ms)
 *   Halt playing of a specified channel with fade-out effect.
 *
 *   @param channel [Integer] the channel to be halted, or -1 for all channels.
 *   @param ms [Integer] milliseconds of fade-out effect
 *   @return [nil]
 *
 *   @see .halt
 *   @see .expire
 *   @see .play?
 *   @see .fade_in
 */
static VALUE Channels_s_fade_out(VALUE self, VALUE channel, VALUE ms)
{
    check_channel(channel, 1);
    Mix_FadeOutChannel(NUM2INT(channel), NUM2INT(ms));
    return Qnil;
}

/*
 * @overload play?(channel)
 *   Return true if a specified channel is playing.
 *   
 *   @param channel [Integer] channel to test
 *   @see .pause?
 *   @see .fading
 */
static VALUE Channels_s_play_p(VALUE self, VALUE channel)
{
    check_channel(channel, 0);
    return INT2BOOL(Mix_Playing(NUM2INT(channel)));
}

/*
 * @overload pause?(channel)
 *   Return true if a specified channel is paused.
 *
 *   @note This method returns true if a paused channel is halted by {.halt}, or any
 *     other halting methods.
 *   
 *   @param channel [Integer] channel to test
 *
 *   @see .play?
 *   @see .fading
 */
static VALUE Channels_s_pause_p(VALUE self, VALUE channel)
{
    check_channel(channel, 0);
    return INT2BOOL(Mix_Paused(NUM2INT(channel)));
}

/*
 * @overload fading(channel)
 *   Return the fading state of a specified channel.
 *
 *   The return value is one of the following:
 *
 *   * {SDL2::Mixer::NO_FADING} - **channel** is not fading in, and fading out
 *   * {SDL2::Mixer::FADING_IN} - **channel** is fading in
 *   * {SDL2::Mixer::FADING_OUT} - **channel** is fading out
 *
 *   @param channel [Integer] channel to test
 *   
 *   @return [Integer]
 *
 *   @see .play?
 *   @see .pause?
 *   @see .fade_in
 *   @see .fade_out
 */
static VALUE Channels_s_fading(VALUE self, VALUE which)
{
    check_channel(which, 0);
    return INT2FIX(Mix_FadingChannel(NUM2INT(which)));
}

/*
 * @overload playing_chunk(channel)
 *   Get the {SDL2::Mixer::Chunk} object most recently playing on **channel**.
 *
 *   If **channel** is out of allocated channels, or
 *   no chunk is played yet on **channel**, this method returns nil.
 *   
 *   @param channel [Integer] the channel to get the chunk object
 *   @return [SDL2::Mixer::Chunk,nil]
 */
static VALUE Channels_s_playing_chunk(VALUE self, VALUE channel)
{
    check_channel(channel, 0);
    return rb_ary_entry(playing_chunks, NUM2INT(channel));
}

/*
 * Document-class: SDL2::Mixer::Channels::Group
 *
 * This class represents a channel group. A channel group is
 * a set of channels and you can stop playing and fade out playing
 * channels of an group at the same time.
 *
 * Each channel group is identified by an integer called tag.
 */

/*
 * Initialize the channel with given **tag**.
 *
 * Groups with a common tag are identified.
 */
static VALUE Group_initialize(VALUE self, VALUE tag)
{
    rb_iv_set(self, "@tag", tag);
    return Qnil;
}

/*
 * Get the default channel group.
 *
 * The default channel group refers all channels in the mixer system.
 * 
 * @return [SDL2::Mixer::Channels::Group]
 */
static VALUE Group_s_default(VALUE self)
{
    VALUE tag = INT2FIX(-1);
    return rb_class_new_instance(1, &tag, self);
}

/*
 * Get the tag of the group.
 *
 * @return [Integer]
 */
inline static int Group_tag(VALUE group)
{
    return NUM2INT(rb_iv_get(group, "@tag"));
}

/*
 * @overload ==(other) 
 *   Return true if **self** and **other** are same.
 *   
 *   **self** and **other** are considered to be same
 *   if they have the same tag.
 *   
 *   @param other [SDL2::Mixer::Channel::Group] a compared object
 *   @return [Boolean]
 */
static VALUE Group_eq(VALUE self, VALUE other)
{
    return INT2BOOL(rb_obj_is_instance_of(other, cGroup) &&
                    Group_tag(self) == Group_tag(other));
}

/*
 * @overload add(which) 
 *   Add a channel to the group.
 *   
 *   @param which [Integer] a channel id
 *   @return [nil]
 */
static VALUE Group_add(VALUE self, VALUE which)
{
    if (!Mix_GroupChannel(NUM2INT(which), Group_tag(self))) {
        SDL_SetError("Cannot add channel %d", NUM2INT(which));
        SDL_ERROR();
    }
    return Qnil;
}

/*
 * Get the number of channels belong to the group.
 *
 * @return [Integer]
 */
static VALUE Group_count(VALUE self)
{
    return INT2NUM(Mix_GroupCount(Group_tag(self)));
}

/*
 * Return the first available channel in the group.
 *
 * Return -1 if no channel is available.
 * 
 * @return [Integer]
 */
static VALUE Group_available(VALUE self)
{
    return INT2NUM(Mix_GroupAvailable(Group_tag(self)));
}

/*
 * Return the oldest cahnnel in the group.
 * 
 * Return -1 if no channel is available.
 * 
 * @return [Integer]
 */
static VALUE Group_oldest(VALUE self)
{
    return INT2NUM(Mix_GroupOldest(Group_tag(self)));
}

/*
 * Return the newer cahnnel in the group.
 * 
 * Return -1 if no channel is available.
 * 
 * @return [Integer]
 */
static VALUE Group_newer(VALUE self)
{
    return INT2NUM(Mix_GroupNewer(Group_tag(self)));
}

/*
 * @overload fade_out(ms)
 *   Halt playing of all channels in the group with fade-out effect.
 *   
 *   @param ms [Integer] milliseconds of fade-out effect
 *   @return [Integer] the number of channels affected by this method
 *   @see Channels.fade_out
 *   @see .halt
 */
static VALUE Group_fade_out(VALUE self, VALUE ms)
{
    return INT2NUM(Mix_FadeOutGroup(Group_tag(self), NUM2INT(ms)));
}

/*
 * Halt playing of all channels in the group.
 *
 * @return [nil]
 * @see Channels.halt
 * @see .fade_out
 */
static VALUE Group_halt(VALUE self)
{
    Mix_HaltGroup(Group_tag(self));
    return Qnil;
}

/*
 * Document-module: SDL2::Mixer::MusicChannel
 *
 * This module provides the functions to play {SDL2::Mixer::Music}.
 */

/*
 * @overload play(music, loops)
 *    Play **music** **loops** times.
 *
 *    @note the meaning of **loop** is different from {SDL2::Mixer::Channels.play}.
 *
 *    @param music [SDL2::Mixer::Music] music to play
 *    @param loops [Integer] number of times to play the music.
 *      0 plays the music zero times.
 *      -1 plays the music forever.
 *
 *    @return [nil]
 *    
 *    @see .fade_in
 *    
 */
static VALUE MusicChannel_s_play(VALUE self, VALUE music, VALUE loops)
{
    HANDLE_MIX_ERROR(Mix_PlayMusic(Get_Mix_Music(music), NUM2INT(loops)));
    playing_music = music;
    return Qnil;
}

/*
 * @overload fade_in(music, loops, ms, pos=0)
 *    Play **music** **loops** times with fade-in effect.
 *
 *    @note the meaning of **loop** is different from {SDL2::Mixer::Channels.play}.
 *
 *    @param music [SDL2::Mixer::Music] music to play
 *    @param loops [Integer] number of times to play the music.
 *      0 plays the music zero times.
 *      -1 plays the music forever.
 *    @param ms [Integer] milliseconds for the fade-in effect
 *    @param pos [Float] the position to play from.
 *      The meaning of "position" is different for the type of music sources.
 *
 *    @return [nil]
 *    
 *    @see .play
 */
static VALUE MusicChannel_s_fade_in(int argc, VALUE* argv, VALUE self)
{
    VALUE music, loops, fade_in_ms, pos;
    rb_scan_args(argc, argv, "31", &music, &loops, &fade_in_ms, &pos);
    HANDLE_MIX_ERROR(Mix_FadeInMusicPos(Get_Mix_Music(music), NUM2INT(loops), 
                                        NUM2INT(fade_in_ms),
                                        pos == Qnil ? 0 : NUM2DBL(pos)));
    playing_music = music;
    return Qnil;
}

/*
 * Get the volume of the music channel.
 * 
 * @return [Integer]
 *
 * @see .volume=
 */
static VALUE MusicChannel_s_volume(VALUE self)
{
    return INT2FIX(Mix_VolumeMusic(-1));
}

/*
 * @overload volume=(vol)
 *   Set the volume of the music channel.
 *
 *   @param vol [Integer] the volume for mixing,
 *     from 0 to {SDL2::Mixer::MAX_VOLUME}(128).
 *   @return [vol]
 *   
 *   @see .volume
 */
static VALUE MusicChannel_s_set_volume(VALUE self, VALUE volume)
{
    Mix_VolumeMusic(NUM2INT(volume));
    return volume;
}

/*
 * Pause the playback of the music channel.
 *
 * @return [nil]
 * 
 * @see .resume
 * @see .pause?
 */
static VALUE MusicChannel_s_pause(VALUE self)
{
    Mix_PauseMusic(); return Qnil;
}

/*
 * Resume the playback of the music channel.
 *
 * @return [nil]
 * 
 * @see .pause
 * @see .pause?
 */
static VALUE MusicChannel_s_resume(VALUE self)
{
    Mix_ResumeMusic(); return Qnil;
}

/*
 * Rewind the music to the start.
 *
 * @return [nil]
 */
static VALUE MusicChannel_s_rewind(VALUE self)
{
    Mix_RewindMusic(); return Qnil;
}

/*
 * @overload set_position(position)
 *   Set the position of the currently playing music.
 *
 *   @param position [Float] the position to play from.
 *   @return [nil]
 */
static VALUE MusicChannel_s_set_position(VALUE self, VALUE position)
{
    HANDLE_MIX_ERROR(Mix_SetMusicPosition(NUM2DBL(position)));
    return Qnil;
}

/*
 * Halt the music playback.
 *
 * @return [nil]
 */
static VALUE MusicChannel_s_halt(VALUE self)
{
    Mix_HaltMusic(); return Qnil;
}

/*
 * @overload fade_out(ms)
 *   Halt the music playback with fade-out effect.
 *
 *   @return [nil]
 */
static VALUE MusicChannel_s_fade_out(VALUE self, VALUE fade_out_ms)
{
    Mix_FadeOutMusic(NUM2INT(fade_out_ms)); return Qnil;
}

/*
 * Return true if a music is playing.
 */
static VALUE MusicChannel_s_play_p(VALUE self)
{
    return INT2BOOL(Mix_PlayingMusic());
}

/*
 * Return true if a music playback is paused.
 */
static VALUE MusicChannel_s_pause_p(VALUE self)
{
    return INT2BOOL(Mix_PausedMusic());
}

/*
 * Get the fading state of the music playback.
 *
 * The return value is one of the following:
 *
 * * {SDL2::Mixer::NO_FADING} - not fading in, and fading out
 * * {SDL2::Mixer::FADING_IN} - fading in
 * * {SDL2::Mixer::FADING_OUT} - fading out
 *
 * @return [Integer]
 * 
 * @see .fade_in
 * @see .fade_out
 * 
 */
static VALUE MusicChannel_s_fading(VALUE self)
{
    return INT2NUM(Mix_FadingMusic());
}

/*
 * Get the {SDL2::Mixer::Music} object that most recently played.
 *
 * Return nil if no music object is played yet.
 *
 * @return [SDL2::Mixer::Music,nil]
 */
static VALUE MusicChannel_s_playing_music(VALUE self)
{
    return playing_music;
}

/*
 * Document-class: SDL2::Mixer::Chunk
 *
 * This class represents a sound sample, a kind of sound sources.
 * 
 * Chunk objects is playable on {SDL2::Mixer::Channels}.
 *
 * @!method destroy?
 *   Return true if the memory is deallocated by {#destroy}.
 */

/*
 * @overload load(path)
 *   Load a sample from file.
 *
 *   This can load WAVE, AIFF, RIFF, OGG, and VOC files.
 *
 *   @note {SDL2::Mixer.open} must be called before calling this method.
 *
 *   @param path [String] the fine name
 *   @return [SDL2::Mixer::Chunk]
 *
 *   @raise [SDL2::Error] raised when failing to load
 */
static VALUE Chunk_s_load(VALUE self, VALUE fname)
{
    Mix_Chunk* chunk = Mix_LoadWAV(StringValueCStr(fname));
    VALUE c;
    if (!chunk)
        MIX_ERROR();
    c = Chunk_new(chunk);
    rb_iv_set(c, "@filename", fname);
    return c;
}

/*
 * Get the names of the sample decoders.
 *
 * @return [Array<String>] the names of decoders, such as: "WAVE", "OGG", etc.
 */
static VALUE Chunk_s_decoders(VALUE self)
{
    int i;
    int num_decoders = Mix_GetNumChunkDecoders();
    VALUE ary = rb_ary_new();
    for (i=0; i < num_decoders; ++i)
        rb_ary_push(ary, rb_usascii_str_new_cstr(Mix_GetChunkDecoder(i)));
    return ary;
}

/*
 * Deallocate the sample memory.
 *
 * Normally, the memory is deallocated by ruby's GC, but 
 * you can surely deallocate the memory with this method at any time.
 * 
 * @return [nil]
 */
static VALUE Chunk_destroy(VALUE self)
{
    Chunk* c = Get_Chunk(self);
    if (c->chunk) Mix_FreeChunk(c->chunk);
    c->chunk = NULL;
    return Qnil;
}

/*
 * Get the volume of the sample.
 *
 * @return [Integer] the volume from 0 to {SDL2::Mixer::MAX_VOLUME}.
 *
 * @see #volume=
 */
static VALUE Chunk_volume(VALUE self)
{
    return INT2NUM(Mix_VolumeChunk(Get_Mix_Chunk(self), -1));
}

/*
 * @overload volume=(vol)
 *   Set the volume of the sample.
 *
 *   @param vol [Integer] the new volume
 *   @return [vol]
 *
 *   @see #volume
 */
static VALUE Chunk_set_volume(VALUE self, VALUE vol)
{
    return INT2NUM(Mix_VolumeChunk(Get_Mix_Chunk(self), NUM2INT(vol)));
}

/* @return [String] inspection string */
static VALUE Chunk_inspect(VALUE self)
{
    VALUE filename = rb_iv_get(self, "@filename");
    if (RTEST(Chunk_destroy_p(self)))
        return rb_sprintf("<%s: destroyed>", rb_obj_classname(self));
    
    return rb_sprintf("<%s: filename=\"%s\" volume=%d>",
                      rb_obj_classname(self),
                      StringValueCStr(filename),
                      Mix_VolumeChunk(Get_Mix_Chunk(self), -1));
}

/*
 * Document-class: SDL2::Mixer::Music
 *
 * This class represents music, a kind of sound sources.
 *
 * Music is playable on {SDL2::Mixer::MusicChannel}, not on {SDL2::Mixer::Channels}.
 *
 * @!method destroy?
 *   Return true if the memory is deallocated by {#destroy}.
 */

/*
 * Get the names of music decoders.
 *
 * @return [Array<String>] the names of decorders (supported sound formats),
 *   such as: "OGG", "WAVE", "MP3"
 */
static VALUE Music_s_decoders(VALUE self)
{
    int num_decoders = Mix_GetNumMusicDecoders();
    int i;
    VALUE decoders = rb_ary_new2(num_decoders);
    for (i=0; i<num_decoders; ++i)
        rb_ary_push(decoders, utf8str_new_cstr(Mix_GetMusicDecoder(i)));
    return decoders;
}

/*
 * @overload load(path)
 *   Load a music from file.
 *
 *   @param path [String] the file path
 *   @return [SDL2::Mixer::Music]
 *
 *   @raise [SDL2::Error] raised when failing to load.
 */
static VALUE Music_s_load(VALUE self, VALUE fname)
{
    Mix_Music* music = Mix_LoadMUS(StringValueCStr(fname));
    VALUE mus;
    if (!music) MIX_ERROR();
    mus = Music_new(music);
    rb_iv_set(mus, "@filename", fname);
    return mus;
}

/*
 * Deallocate the music memory.
 *
 * Normally, the memory is deallocated by ruby's GC, but 
 * you can surely deallocate the memory with this method at any time.
 * 
 * @return [nil]
 */
static VALUE Music_destroy(VALUE self)
{
    Music* c = Get_Music(self);
    if (c) Mix_FreeMusic(c->music);
    c->music = NULL;
    return Qnil;
}

/* @return [String] inspection string */
static VALUE Music_inspect(VALUE self)
{
    VALUE filename = rb_iv_get(self, "@filename");
    if (RTEST(Music_destroy_p(self)))
        return rb_sprintf("<%s: destroyed>", rb_obj_classname(self));
    
    return rb_sprintf("<%s: filename=\"%s\" type=%d>",
                      rb_obj_classname(self), StringValueCStr(filename),
                      Mix_GetMusicType(Get_Mix_Music(self)));
}


void rubysdl2_init_mixer(void)
{
    mMixer = rb_define_module_under(mSDL2, "Mixer");

    rb_define_module_function(mMixer, "init", Mixer_s_init, 1);
    rb_define_module_function(mMixer, "open", Mixer_s_open, -1);
    rb_define_module_function(mMixer, "close", Mixer_s_close, 0);
    rb_define_module_function(mMixer, "query", Mixer_s_query, 0);
    
    /* define(`DEFINE_MIX_INIT',`rb_define_const(mMixer, "INIT_$1", UINT2NUM(MIX_INIT_$1))') */
    /* Initialize Ogg flac loader */
    DEFINE_MIX_INIT(FLAC);
    /* Initialize MOD loader */
    DEFINE_MIX_INIT(MOD);
    /* Initialize MP3 loader */
    DEFINE_MIX_INIT(MP3);
    /* Initialize Ogg vorbis loader */
    DEFINE_MIX_INIT(OGG);

#if HAVE_CONST_MIX_INIT_MODPLUG
    /* Initialize libmodplug */
    DEFINE_MIX_INIT(MODPLUG);
#endif
#if HAVE_CONST_MIX_INIT_FLUIDSYNTH
    /* Initialize fluidsynth */
    DEFINE_MIX_INIT(FLUIDSYNTH);
#endif
#if HAVE_CONST_MIX_INIT_MID
    /* Initialize mid */
    DEFINE_MIX_INIT(MID);
#endif

    /* define(`DEFINE_MIX_FORMAT',`rb_define_const(mMixer, "FORMAT_$1", UINT2NUM(AUDIO_$1))') */
    /* Unsiged 8-bit sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(U8);
    /* Siged 8-bit sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(S8);
    /* Unsiged 16-bit little-endian sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(U16LSB);
    /* Siged 16-bit little-endian sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(S16LSB);
    /* Unsiged 16-bit big-endian sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(U16MSB);
    /* Unsiged 16-bit big-endian sample format. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(S16MSB);
    /* Unsiged 16-bit sample format. Endian is same as system byte order. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(U16SYS);
    /* Siged 16-bit sample format. Endian is same as system byte order. Used by {Mixer.open} */
    DEFINE_MIX_FORMAT(S16SYS);
    /* Default frequency. 22050 (Hz) */
    rb_define_const(mMixer, "DEFAULT_FREQUENCY", UINT2NUM(MIX_DEFAULT_FREQUENCY));
    /* Default sample format. Same as {Mixer::FORMAT_S16SYS}. */
    rb_define_const(mMixer, "DEFAULT_FORMAT", UINT2NUM(MIX_DEFAULT_FORMAT));
    /* Default number of channels. 2. */
    rb_define_const(mMixer, "DEFAULT_CHANNELS", INT2FIX(MIX_DEFAULT_CHANNELS));
    /* Max volume value. 128. */
    rb_define_const(mMixer, "MAX_VOLUME", INT2FIX(MIX_MAX_VOLUME));
    /* This constants represents that the channel is not fading in and fading out. */
    rb_define_const(mMixer, "NO_FADING", INT2FIX(MIX_NO_FADING));
    /* This constants represents that the channel is fading out. */
    rb_define_const(mMixer, "FADING_OUT", INT2FIX(MIX_FADING_OUT));
    /* This constants represents that the channel is fading in. */
    rb_define_const(mMixer, "FADING_IN", INT2FIX(MIX_FADING_IN));

    
    cChunk = rb_define_class_under(mMixer, "Chunk", rb_cObject);
    rb_undef_alloc_func(cChunk);
    rb_define_singleton_method(cChunk, "load", Chunk_s_load, 1);
    rb_define_singleton_method(cChunk, "decoders", Chunk_s_decoders, 0);
    rb_define_method(cChunk, "destroy", Chunk_destroy, 0);
    rb_define_method(cChunk, "destroy?", Chunk_destroy_p, 0);
    rb_define_method(cChunk, "volume", Chunk_volume, 0);
    rb_define_method(cChunk, "volume=", Chunk_set_volume, 1);
    rb_define_method(cChunk, "inspect", Chunk_inspect, 0);
    /* @return [String] The file name of the file from which the sound is loaded. */
    rb_define_attr(cChunk, "filename", 1, 0);

    
    cMusic = rb_define_class_under(mMixer, "Music", rb_cObject);
    rb_undef_alloc_func(cMusic);
    rb_define_singleton_method(cMusic, "decoders", Music_s_decoders, 0);
    rb_define_singleton_method(cMusic, "load", Music_s_load, 1);
    rb_define_method(cMusic, "destroy", Music_destroy, 0);
    rb_define_method(cMusic, "destroy?", Music_destroy_p, 0);
    rb_define_method(cMusic, "inspect", Music_inspect, 0);

    
    mChannels = rb_define_module_under(mMixer, "Channels");
    rb_define_module_function(mChannels, "allocate", Channels_s_allocate, 1);
    rb_define_module_function(mChannels, "reserve", Channels_s_reserve, 1);
    rb_define_module_function(mChannels, "volume", Channels_s_volume, 1);
    rb_define_module_function(mChannels, "set_volume", Channels_s_set_volume, 2);
    rb_define_module_function(mChannels, "play", Channels_s_play, -1);
    rb_define_module_function(mChannels, "fade_in", Channels_s_fade_in, -1);
    rb_define_module_function(mChannels, "pause", Channels_s_pause, 1);
    rb_define_module_function(mChannels, "resume", Channels_s_resume, 1);
    rb_define_module_function(mChannels, "halt", Channels_s_halt, 1);
    rb_define_module_function(mChannels, "expire", Channels_s_expire, 1);
    rb_define_module_function(mChannels, "fade_out", Channels_s_fade_out, 2);
    rb_define_module_function(mChannels, "play?", Channels_s_play_p, 1);
    rb_define_module_function(mChannels, "pause?", Channels_s_pause_p, 1);
    rb_define_module_function(mChannels, "fading", Channels_s_fading, 1);
    rb_define_module_function(mChannels, "playing_chunk", Channels_s_playing_chunk, 1);

    
    cGroup = rb_define_class_under(mChannels, "Group", rb_cObject);
    rb_define_method(cGroup, "initialize", Group_initialize, 1);
    rb_define_singleton_method(cGroup, "default", Group_s_default, 0);
    /* @return [Integer] tag id */
    rb_define_attr(cGroup, "tag", 1, 0);
    rb_define_method(cGroup, "==", Group_eq, 1);
    rb_define_method(cGroup, "add", Group_add, 1);
    rb_define_method(cGroup, "count", Group_count, 0);
    rb_define_method(cGroup, "available", Group_available, 0);
    rb_define_method(cGroup, "newer", Group_newer, 0);
    rb_define_method(cGroup, "oldest", Group_oldest, 0);
    rb_define_method(cGroup, "fade_out", Group_fade_out, 1);
    rb_define_method(cGroup, "halt", Group_halt, 0);
    
    
    mMusicChannel = rb_define_module_under(mMixer, "MusicChannel");
    rb_define_module_function(mMusicChannel, "play", MusicChannel_s_play, 2);
    rb_define_module_function(mMusicChannel, "fade_in", MusicChannel_s_fade_in, -1);
    rb_define_module_function(mMusicChannel, "volume", MusicChannel_s_volume, 0);
    rb_define_module_function(mMusicChannel, "volume=", MusicChannel_s_set_volume, 1);
    rb_define_module_function(mMusicChannel, "pause", MusicChannel_s_pause, 0);
    rb_define_module_function(mMusicChannel, "resume", MusicChannel_s_resume, 0);
    rb_define_module_function(mMusicChannel, "rewind", MusicChannel_s_rewind, 0);
    rb_define_module_function(mMusicChannel, "set_position", MusicChannel_s_set_position, 1);
    rb_define_module_function(mMusicChannel, "halt", MusicChannel_s_halt, 0);
    rb_define_module_function(mMusicChannel, "fade_out", MusicChannel_s_fade_out, 1);
    rb_define_module_function(mMusicChannel, "play?", MusicChannel_s_play_p, 0);
    rb_define_module_function(mMusicChannel, "pause?", MusicChannel_s_pause_p, 0);
    rb_define_module_function(mMusicChannel, "fading", MusicChannel_s_fading, 0);
    rb_define_module_function(mMusicChannel, "playing_music", MusicChannel_s_playing_music, 0);

    
    rb_gc_register_address(&playing_chunks);
    rb_gc_register_address(&playing_music);
}

#else /* HAVE_SDL_MIXER_H */
void rubysdl2_init_mixer(void)
{
}
#endif
