#ifndef __ALSA_PLAYER_H__
#define __ALSA_PLAYER_H__

#include <cassert>
#include <alsa/asoundlib.h>

class Player {
    constexpr static int FRAME_RATE = 44100;

    snd_pcm_t* handle;
    std::vector<int32_t> buf;
    int32_t* cur;

    void filter(std::vector<int32_t>& buf, std::vector<int>& mx) {
        int k = 0;
        for (int i = 1; i < (int)mx.size()-1; i++) {
            int16_t a, b, c;
            // The least significant 16 bits, aka the left channel.
            a = buf[mx[i-1]];
            b = buf[mx[i]];
            c = buf[mx[i+1]];
            if (a < b && b > c) {
                mx[k++] = mx[i];
            }
        }
        mx.resize(k);
    }

public:
    Player(const char* name = "default") {
        snd_pcm_hw_params_t* hw_params;
        assert(snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) >= 0);
        assert(snd_pcm_nonblock(handle, 1) >= 0);
        assert(snd_pcm_hw_params_malloc(&hw_params) >= 0);
        assert(snd_pcm_hw_params_any(handle, hw_params) >= 0);
        assert(snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) >= 0);
        assert(snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE) >= 0);
        unsigned int rate = FRAME_RATE;
        int dir = 0;
        assert(snd_pcm_hw_params_set_rate_near (handle, hw_params, &rate, &dir) >= 0);
        assert(snd_pcm_hw_params_set_channels (handle, hw_params, 2) >= 0);
        assert(snd_pcm_hw_params(handle, hw_params) >= 0);
        snd_pcm_hw_params_free(hw_params);
    }

    ~Player() {
        snd_pcm_close(handle);
    }

    void playWAV(const char* file_name) {
        prepare(file_name);
        while (avail()) {
            play();
        }
        drain();
    }

    void prepare(const char* file_name) {
        auto f = fopen(file_name, "r");
        fseek(f, 0, SEEK_END);
        assert((ftell(f) - 44) % sizeof(int32_t) == 0);
        buf.resize((ftell(f) - 44)/sizeof(int32_t));
        fseek(f, 44, SEEK_SET);
        assert(fread(buf.data(), buf.size()*sizeof(int32_t), 1, f) == 1);
        fclose(f);

        assert(snd_pcm_prepare(handle) >= 0);
        cur = buf.data();
    }

    bool avail() {
        return cur != buf.data() + buf.size();
    }

    void play() {
        auto nframes = snd_pcm_avail_update(handle) - 1;
        if (cur + nframes >= buf.data() + buf.size()) {
            nframes = buf.data() + buf.size() - cur;
        }
        snd_pcm_writei(handle, cur, nframes);
        cur += nframes;
    }

    void drain() {
        snd_pcm_drain(handle);
    }

    std::vector<int> genRhythm() {
        std::vector<int> mx(buf.size());
        for (int i = 0; i < (int)mx.size(); i++) {
            mx[i] = i;
        }
        while (mx.size() > 500) {
            filter(buf, mx);
        }
        for (auto& it : mx) {
            it /= (double)FRAME_RATE / 1000;
        }
        return mx;
    };

};

#endif
