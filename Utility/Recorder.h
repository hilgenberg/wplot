#pragma once
class GL_Image;

class Recorder
{
public:
	Recorder();
	~Recorder();
	
	void add(const GL_Image &im);
	void finish();

private:
	int i; // next frame index or -1 before first add()
	const struct AVCodec *codec;
	struct AVCodecContext *ctx;
	struct AVPacket *pkt;
	struct AVFrame *frame;
	FILE *f;
};
