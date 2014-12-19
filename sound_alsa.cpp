
#include <cstdlib>
#include <cstdio>
#include <assert.h>
#include <endian.h>
#include <thread>
#include <iostream>
#include <memory>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <alsa/output.h>

extern const char _binary_tweet_wav_start[];
extern const char _binary_tweet_wav_end[];
#pragma pack(1)

struct RIFF {
	uint32_t signature;
	uint32_t chunksize;
	uint32_t type;
	char chunks[1];
};

struct Chunk {
	char id[4];                                                // "fmt "
	uint32_t data_size;                                        // 16 + extra format bytes
	char Chunkdata[1];
};

struct Chunk_fmt {
	uint32_t id;                                               // "fmt "
	uint32_t data_size;                                        // 16 + extra format bytes
	uint16_t compression_code;                                 // 1-65535
	uint16_t channels;
	uint32_t samplerate;
	uint32_t byterate;
	uint16_t blockalign;
	uint16_t samplebits;
	uint16_t extra_format_bytes;
	char extra[1];
};

enum compression_code_t
{
	compression_code_unknow,
	compression_code_pcm,
	compression_code_adpcm,
	compression_code_ITU_a = 6,
	compression_code_ITU_Au = 7,
	compression_code_IMA = 17,
	compression_code_ITU_adpcm = 20,
	compression_code_GSM = 49,
	compression_code_ITU_G_adpcm = 64,
	compression_code_MPEG = 80,
};

static void play_thread(const Chunk_fmt* fmt_chunk = NULL, const Chunk * pcm_chunk = NULL)
{
	std::cout << "playing sound..." << std::endl;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_hw_params_alloca(&hwparams);


	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

	if (le16toh(fmt_chunk->samplebits) == 16)
	{
		format = SND_PCM_FORMAT_S16_LE;
	}
	if (le16toh(fmt_chunk->samplebits) == 8)
	{
		format = SND_PCM_FORMAT_U8;
	}

	snd_pcm_uframes_t frames = pcm_chunk->data_size / ( le16toh(fmt_chunk->channels) * le16toh(fmt_chunk->samplebits)/8);

	int ret;
	std::shared_ptr<snd_pcm_t> pcm;
	snd_pcm_t * _pcm = NULL;
	ret = snd_pcm_open(&_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
	pcm.reset(_pcm, snd_pcm_close);

	if (ret)
	{
		std::cerr << "unable to open sound card, you won't be able to hear any sound!" << std::endl;
		return;
	}

	ret = snd_pcm_set_params(pcm.get(), format, SND_PCM_ACCESS_RW_INTERLEAVED, le16toh(fmt_chunk->channels), le16toh(fmt_chunk->samplebits), 1, 1000000);
	ret = snd_pcm_wait(pcm.get(), 1000);
	if (ret )
	{
		ret = snd_pcm_writei(pcm.get(), pcm_chunk->Chunkdata,  frames);
		ret = snd_pcm_drain(pcm.get());
	}
}

extern "C" int playsound()
{
	const RIFF* riff = reinterpret_cast<const RIFF*>(_binary_tweet_wav_start);

	assert(le32toh(reinterpret_cast<const RIFF*>(_binary_tweet_wav_start)->signature) == 0x46464952);
	assert(le32toh(reinterpret_cast<const RIFF*>(_binary_tweet_wav_start)->type) == 0x45564157);

	// 遍历 Chunk

	const Chunk_fmt* fmt_chunk = NULL;
	const Chunk * pcm_chunk = NULL;


	for (
		const Chunk* p_chunk = reinterpret_cast<const Chunk*>(riff->chunks);
		reinterpret_cast<const char*>(p_chunk) < _binary_tweet_wav_end;
		p_chunk = reinterpret_cast<const Chunk*>(p_chunk->Chunkdata + le32toh(p_chunk->data_size))
	)
	{
		if (std::string(p_chunk->id, 4) == "fmt ")
		{
			fmt_chunk = reinterpret_cast<const Chunk_fmt*>(p_chunk);
		}

		if (std::string(p_chunk->id, 4) == "data")
		{
			pcm_chunk = p_chunk;                                     //->Chunkdata;
		}
	}

	if ((!pcm_chunk) || (!fmt_chunk))
		return 0;

	std::thread(std::bind(play_thread, fmt_chunk, pcm_chunk)).detach();
	return 0;
}
