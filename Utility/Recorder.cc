#include "Recorder.h"
#include "../Graphs/GL_Image.h"
#include "../Point.h"
#include <filesystem>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using std::cerr;
using std::endl;
typedef std::string str;

Recorder::Recorder()
: i(-1), f(NULL)
, codec(NULL), ctx(NULL), pkt(NULL), frame(NULL)
{}

Recorder::~Recorder()
{
	if (ctx) avcodec_free_context(&ctx); ctx = NULL;
	if (frame) av_frame_free(&frame); frame = NULL;
	if (pkt) av_packet_free(&pkt); pkt = NULL;
	if (f) fclose(f); f = NULL;
	codec = NULL;
}

static bool encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
	//if (frame) cerr << "Send frame " << frame->pts << endl;

	int ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0)
	{
		cerr << "Error in " << ret << " in avcodec_send_frame" << endl;
		return false;
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return true;
		if (ret < 0)
		{
			cerr << "Error " << ret << " in avcodec_receive_packet" << endl;
			return false;
		}

		//cerr << "Write packet " << pkt->pts << " (size=" << pkt->size << ")" << endl;
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
	return true;
}

#define CODEC   AV_CODEC_ID_MPEG2VIDEO
//#define CODEC   AV_CODEC_ID_H264
#define PIX_FMT AV_PIX_FMT_YUV420P
#define FPS     30
#define BITRATE 400000

void Recorder::add(const GL_Image &im)
{
	if (im.empty()) return;

	int w = im.w(), h = im.h();
	
	#if CODEC == AV_CODEC_ID_H264
	if (h&1) --h;
	if (w&1) --w;
	#endif

	if (i < 0)
	{
		codec = avcodec_find_encoder(CODEC);
		//codec = avcodec_find_encoder_by_name("libx264");
		if (!codec) {
			cerr << "Codec not found" << endl;
			return;
		}

		ctx = avcodec_alloc_context3(codec);
		if (!ctx) {
			cerr << "Could not allocate video codec context" << endl;
			return;
		}
		ctx->bit_rate  = BITRATE;
		ctx->width     = w;
		ctx->height    = h;
		ctx->time_base = (AVRational){1, FPS};
		ctx->framerate = (AVRational){FPS, 1};
		ctx->gop_size  = 10;
		ctx->max_b_frames = 1;
		ctx->pix_fmt   = PIX_FMT;
		if (codec->id == AV_CODEC_ID_H264) av_opt_set(ctx->priv_data, "preset", "slow", 0);

		pkt = av_packet_alloc();
		if (!pkt)
		{
			cerr << "Could not allocate video packet" << endl;
			avcodec_free_context(&ctx); ctx = NULL;
			codec = NULL;
			return;
		}

		int ret = avcodec_open2(ctx, codec, NULL);
		if (ret < 0) {
			char err[AV_ERROR_MAX_STRING_SIZE+1] = {0};
			av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
			cerr << "Could not open codec: " << err << endl;
			avcodec_free_context(&ctx); ctx = NULL;
			av_packet_free(&pkt); pkt = NULL;
			codec = NULL;
			return;
		}

		frame = av_frame_alloc();
		if (!frame) {
			cerr << "Could not allocate video frame" << endl;
			avcodec_free_context(&ctx); ctx = NULL;
			av_packet_free(&pkt); pkt = NULL;
			codec = NULL;
			return;
		}
		frame->format = ctx->pix_fmt;
		frame->width  = ctx->width;
		frame->height = ctx->height;
		ret = av_frame_get_buffer(frame, 32);
		if (ret < 0)
		{
			cerr << "Could not allocate the video frame data" << endl;
			avcodec_free_context(&ctx); ctx = NULL;
			av_frame_free(&frame); frame = NULL;
			av_packet_free(&pkt); pkt = NULL;
			codec = NULL;
			return;
		}

		#if EQUATION==DIRAC
		str f1 = "wp_dirac";
		#elif EQUATION==MAXWELL
		str f1 = "wp_maxwell";
		#elif EQUATION==KLEINGORDON
		str f1 = "wp_kgordon";
		#endif
		int j = 0;
		str filename = f1 + ".mpg";
		while (std::filesystem::exists(filename))
		{
			std::ostringstream os; os << f1 << "_" << ++j << ".mpg";
			filename = os.str();
		}

		f = fopen(filename.c_str(), "wb");
		if (!f)
		{
			cerr << "Could not open " << filename << endl;
			avcodec_free_context(&ctx); ctx = NULL;
			av_frame_free(&frame); frame = NULL;
			av_packet_free(&pkt); pkt = NULL;
			codec = NULL;
			return;
		}

		i = 0;
	}

	if (w != ctx->width || h != ctx->height)
	{
		cerr << "Image size mismatch - skipping frame" << endl;
	}

	struct SwsContext *sws_context = NULL;
	sws_context = sws_getCachedContext(sws_context,
		ctx->width, ctx->height, AV_PIX_FMT_RGBA,
		ctx->width, ctx->height, PIX_FMT,
		0, 0, 0, 0);
	int linesize = 4 * im.w();
	const uint8_t *rgb = (const uint8_t *)im.data().data();
	sws_scale(sws_context, &rgb, &linesize, 0, ctx->height, frame->data, frame->linesize);

	int ret = av_frame_make_writable(frame);
	if (ret < 0)
	{
		cerr << "Error " << ret << " in av_frame_make_writable" << endl;
		return;
	}

	frame->pts = i;
	if (encode(ctx, frame, pkt, f)) ++i;
}

void Recorder::finish()
{
	if (i < 0) return;

	encode(ctx, NULL, pkt, f);

	if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
	{
		uint8_t endcode[] = { 0, 0, 1, 0xb7 };
		fwrite(endcode, 1, sizeof(endcode), f);
	}

	fclose(f); f = NULL;

	avcodec_free_context(&ctx); ctx = NULL;
	av_frame_free(&frame); frame = NULL;
	av_packet_free(&pkt); pkt = NULL;
	codec = NULL;
	i = -1;
}
